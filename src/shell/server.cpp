/*
 * Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
 *
 * This file is part of CasparCG (www.casparcg.com).
 *
 * CasparCG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CasparCG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Robert Nagy, ronag89@gmail.com
 */
#include "included_modules.h"

#include "server.h"

#include <accelerator/accelerator.h>

#include <common/env.h>
#include <common/except.h>
#include <common/memory.h>
#include <common/ptree.h>
#include <common/utf.h>

#include <core/consumer/output.h>
#include <core/diagnostics/call_context.h>
#include <core/diagnostics/osd_graph.h>
#include <core/mixer/image/image_mixer.h>
#include <core/producer/cg_proxy.h>
#include <core/producer/color/color_producer.h>
#include <core/producer/frame_producer.h>
#include <core/producer/transition/sting_producer.h>
#include <core/producer/transition/transition_producer.h>
#include <core/video_channel.h>
#include <core/video_format.h>

#include <modules/image/consumer/image_consumer.h>

#include <protocol/amcp/AMCPCommandsImpl.h>
#include <protocol/amcp/amcp_command_repository.h>
#include <protocol/amcp/amcp_shared.h>

#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>

#include <iostream>
#include <thread>
#include <utility>

namespace caspar {
using namespace core;
using namespace protocol;

struct server::impl
{
    video_format_repository                                       video_format_repository_;
    accelerator::accelerator                                      accelerator_;
    std::shared_ptr<amcp::amcp_command_repository>                amcp_command_repo_;
    std::shared_ptr<amcp::amcp_command_repository_wrapper>        amcp_command_repo_wrapper_;
    std::shared_ptr<amcp::command_context_factory>                amcp_context_factory_;
    spl::shared_ptr<std::vector<protocol::amcp::channel_context>> channels_;
    spl::shared_ptr<core::cg_producer_registry>                   cg_registry_;
    spl::shared_ptr<core::frame_producer_registry>                producer_registry_;
    spl::shared_ptr<core::frame_consumer_registry>                consumer_registry_;

    impl(const impl&)            = delete;
    impl& operator=(const impl&) = delete;

    explicit impl()
        : video_format_repository_()
        , accelerator_(video_format_repository_)
        , producer_registry_(spl::make_shared<core::frame_producer_registry>())
        , consumer_registry_(spl::make_shared<core::frame_consumer_registry>())
    {
        caspar::core::diagnostics::osd::register_sink();
    }

    void start()
    {
        setup_amcp_command_repo();
        CASPAR_LOG(info) << L"Initialized command repository.";

        module_dependencies dependencies(
            cg_registry_, producer_registry_, consumer_registry_, amcp_command_repo_wrapper_);
        initialize_modules(dependencies);
        CASPAR_LOG(info) << L"Initialized modules.";
    }

    ~impl()
    {
        amcp_command_repo_wrapper_.reset();
        amcp_command_repo_.reset();
        amcp_context_factory_.reset();

        destroy_producers_synchronously();
        destroy_consumers_synchronously();
        channels_->clear();

        uninitialize_modules();
        core::diagnostics::osd::shutdown();
    }

    void add_video_format_desc(std::wstring id, core::video_format_desc format)
    {
        if (id == L"")
            CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Invalid video-mode id: " + id));

        if (format.width == 0 || format.height == 0)
            CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Invalid dimensions: " +
                                                            boost::lexical_cast<std::wstring>(format.width) + L"x" +
                                                            boost::lexical_cast<std::wstring>(format.height)));

        if (format.field_count != 1 && format.field_count != 2)
            CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Invalid field-count: " +
                                                            boost::lexical_cast<std::wstring>(format.field_count)));

        if (format.time_scale == 0 || format.duration == 0)
            CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Invalid framerate: " +
                                                            boost::lexical_cast<std::wstring>(format.time_scale) +
                                                            L"/" + boost::lexical_cast<std::wstring>(format.duration)));

        // TODO: sum and verify cadence looks correct

        const auto existing = video_format_repository_.find(id);
        if (existing.format != video_format::invalid)
            CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Video-mode already exists: " + id));

        video_format_repository_.store(format);
    }

    int add_channel(std::wstring format_desc_str, std::weak_ptr<channel_osc_sender> weak_osc_sender)
    {
        auto format_desc = video_format_repository_.find(format_desc_str);
        if (format_desc.format == video_format::invalid)
            return -1;

        auto channel_id = static_cast<int>(channels_->size() + 1);
        auto channel =
            spl::make_shared<video_channel>(channel_id,
                                            format_desc,
                                            accelerator_.create_image_mixer(channel_id),
                                            [channel_id, weak_osc_sender](core::monitor::state channel_state) {
                                                auto osc_sender = weak_osc_sender.lock();
                                                if (osc_sender) {
                                                    osc_sender->send_state(channel_id, channel_state);
                                                }
                                            });

        const std::wstring lifecycle_key = L"lock" + std::to_wstring(channel_id);
        channels_->emplace_back(channel, channel->stage(), lifecycle_key);

        return channel_id;
    }

    int add_consumer_from_xml(int channel_index, const boost::property_tree::wptree& config)
    {
        auto type = config.get(L"type", L"");
        if (type.empty())
            return -1;

        auto channel = channels_->at(channel_index);

        core::diagnostics::scoped_call_context save;
        core::diagnostics::call_context::for_thread().video_channel = channel.raw_channel->index();

        std::vector<spl::shared_ptr<core::video_channel>> channels_vec;
        for (auto& cc : *channels_) {
            channels_vec.emplace_back(cc.raw_channel);
        }

        auto consumer = consumer_registry_->create_consumer(type, config, video_format_repository_, channels_vec);
        if (consumer == core::frame_consumer::empty()) {
            return -1;
        }

        channel.raw_channel->output().add(consumer);

        return consumer->index();
    }

    int add_consumer_from_tokens(const std::shared_ptr<core::video_channel>& channel,
                                 int                                         layer_index,
                                 std::vector<std::wstring>                   amcp_params)
    {
        core::diagnostics::scoped_call_context save;
        core::diagnostics::call_context::for_thread().video_channel = channel->index();

        std::vector<spl::shared_ptr<core::video_channel>> channels_vec;
        for (auto& cc : *channels_) {
            channels_vec.emplace_back(cc.raw_channel);
        }

        auto consumer = consumer_registry_->create_consumer(amcp_params, video_format_repository_, channels_vec);
        if (consumer == core::frame_consumer::empty()) {
            return -1;
        }

        channel->output().add(consumer);

        return consumer->index();
    }

    bool remove_consumer_by_port(int channel_index, int layer_index)
    {
        if (channel_index < 0 || channel_index >= channels_->size()) {
            return false;
        }
        auto channel = channels_->at(channel_index);

        return channel.raw_channel->output().remove(layer_index);
    }
    bool remove_consumer_by_params(int channel_index, std::vector<std::wstring> amcp_params)
    {
        if (channel_index < 0 || channel_index >= channels_->size()) {
            return false;
        }
        auto channel = channels_->at(channel_index);

        std::vector<spl::shared_ptr<core::video_channel>> channels_vec;
        for (auto& cc : *channels_) {
            channels_vec.emplace_back(cc.raw_channel);
        }

        auto index = consumer_registry_->create_consumer(amcp_params, video_format_repository_, channels_vec)->index();

        return channel.raw_channel->output().remove(index);
    }

    inline core::frame_producer_dependencies
    get_producer_dependencies(const std::shared_ptr<core::video_channel>& channel)
    {
        std::vector<spl::shared_ptr<core::video_channel>> channels;
        for (auto& cc : *channels_) {
            channels.emplace_back(cc.raw_channel);
        }

        return core::frame_producer_dependencies(channel->frame_factory(),
                                                 channels,
                                                 video_format_repository_,
                                                 channel->stage()->video_format_desc(),
                                                 producer_registry_);
    }

    spl::shared_ptr<core::frame_producer> create_producer(const std::shared_ptr<core::video_channel>& channel,
                                                          int                                         layer_index,
                                                          std::vector<std::wstring>                   amcp_params)
    {
        core::diagnostics::scoped_call_context save;
        core::diagnostics::call_context::for_thread().video_channel = channel->index();
        core::diagnostics::call_context::for_thread().layer         = layer_index;

        return producer_registry_->create_producer(get_producer_dependencies(channel), amcp_params);
    }

    spl::shared_ptr<core::frame_producer>
    create_sting_transition(const std::shared_ptr<core::video_channel>&  channel,
                            int                                          layer_index,
                            const spl::shared_ptr<core::frame_producer>& destination,
                            core::sting_info&                            info)
    {
        core::diagnostics::scoped_call_context save;
        core::diagnostics::call_context::for_thread().video_channel = channel->index();
        core::diagnostics::call_context::for_thread().layer         = layer_index;

        return core::create_sting_producer(get_producer_dependencies(channel), destination, info);
    }

    spl::shared_ptr<core::frame_producer>
    create_basic_transition(const std::shared_ptr<core::video_channel>&  channel,
                            int                                          layer_index,
                            const spl::shared_ptr<core::frame_producer>& destination,
                            core::transition_info&                       info)
    {
        core::diagnostics::scoped_call_context save;
        core::diagnostics::call_context::for_thread().video_channel = channel->index();
        core::diagnostics::call_context::for_thread().layer         = layer_index;

        return core::create_transition_producer(destination, info);
    }

    void setup_amcp_command_repo()
    {
        amcp_command_repo_ = std::make_shared<amcp::amcp_command_repository>(channels_);

        auto ogl_device = accelerator_.get_device();
        auto ctx        = std::make_shared<amcp::amcp_command_static_context>(video_format_repository_,
                                                                       cg_registry_,
                                                                       producer_registry_,
                                                                       consumer_registry_,
                                                                       amcp_command_repo_,
                                                                       ogl_device);

        amcp_context_factory_ = std::make_shared<amcp::command_context_factory>(ctx);

        amcp_command_repo_wrapper_ =
            std::make_shared<amcp::amcp_command_repository_wrapper>(amcp_command_repo_, amcp_context_factory_);

        amcp::register_commands(amcp_command_repo_wrapper_);
    }
};

server::server()
    : impl_(new impl())
{
}
void                                                     server::start() { impl_->start(); }
spl::shared_ptr<protocol::amcp::amcp_command_repository> server::get_amcp_command_repository() const
{
    return spl::make_shared_ptr(impl_->amcp_command_repo_);
}
spl::shared_ptr<std::vector<protocol::amcp::channel_context>>& server::get_channels() const { return impl_->channels_; }

int server::add_consumer_from_xml(int channel_index, const boost::property_tree::wptree& config)
{
    return impl_->add_consumer_from_xml(channel_index, config);
}

int server::add_consumer_from_tokens(const std::shared_ptr<core::video_channel>& channel,
                                     int                                         layer_index,
                                     std::vector<std::wstring>                   amcp_params)
{
    return impl_->add_consumer_from_tokens(channel, layer_index, amcp_params);
}
bool server::remove_consumer_by_port(int channel_index, int layer_index)
{
    return impl_->remove_consumer_by_port(channel_index, layer_index);
}
bool server::remove_consumer_by_params(int channel_index, std::vector<std::wstring> amcp_params)
{
    return impl_->remove_consumer_by_params(channel_index, amcp_params);
}

int server::add_channel(std::wstring format_desc_str, std::weak_ptr<channel_osc_sender> weak_osc_sender)
{
    return impl_->add_channel(format_desc_str, weak_osc_sender);
}

void server::add_video_format_desc(std::wstring id, core::video_format_desc format)
{
    impl_->add_video_format_desc(id, format);
}

spl::shared_ptr<core::frame_producer> server::create_producer(const std::shared_ptr<core::video_channel>& channel,
                                                              int                                         layer_index,
                                                              std::vector<std::wstring>                   amcp_params)
{
    return impl_->create_producer(channel, layer_index, amcp_params);
}

spl::shared_ptr<core::frame_producer>
server::create_sting_transition(const std::shared_ptr<core::video_channel>&  channel,
                                int                                          layer_index,
                                const spl::shared_ptr<core::frame_producer>& destination,
                                core::sting_info&                            info)
{
    return impl_->create_sting_transition(channel, layer_index, destination, info);
}

spl::shared_ptr<core::frame_producer>
server::create_basic_transition(const std::shared_ptr<core::video_channel>&  channel,
                                int                                          layer_index,
                                const spl::shared_ptr<core::frame_producer>& destination,
                                core::transition_info&                       info)
{
    return impl_->create_basic_transition(channel, layer_index, destination, info);
}

} // namespace caspar
