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
#include <protocol/osc/client.h>
#include <protocol/util/tokenize.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>

#include <iostream>
#include <thread>
#include <utility>

namespace caspar {
using namespace core;
using namespace protocol;

std::shared_ptr<boost::asio::io_service> create_running_io_service()
{
    auto service = std::make_shared<boost::asio::io_service>();
    // To keep the io_service::run() running although no pending async
    // operations are posted.
    auto work      = std::make_shared<boost::asio::io_service::work>(*service);
    auto weak_work = std::weak_ptr<boost::asio::io_service::work>(work);
    auto thread    = std::make_shared<std::thread>([service, weak_work] {
        while (auto strong = weak_work.lock()) {
            try {
                service->run();
            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
            }
        }

        CASPAR_LOG(info) << "[asio] Global io_service uninitialized.";
    });

    return std::shared_ptr<boost::asio::io_service>(service.get(), [service, work, thread](void*) mutable {
        CASPAR_LOG(info) << "[asio] Shutting down global io_service.";
        work.reset();
        service->stop();
        if (thread->get_id() != std::this_thread::get_id())
            thread->join();
        else
            thread->detach();
    });
}

struct server::impl
{
    std::shared_ptr<boost::asio::io_service>               io_service_ = create_running_io_service();
    video_format_repository                                video_format_repository_;
    accelerator::accelerator                               accelerator_;
    std::shared_ptr<amcp::amcp_command_repository>         amcp_command_repo_;
    std::shared_ptr<amcp::amcp_command_repository_wrapper> amcp_command_repo_wrapper_;
    std::shared_ptr<amcp::command_context_factory>         amcp_context_factory_;
    std::shared_ptr<osc::client>                           osc_client_ = std::make_shared<osc::client>(io_service_);
    std::vector<std::shared_ptr<void>>                     predefined_osc_subscriptions_;
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
        std::weak_ptr<boost::asio::io_service> weak_io_service = io_service_;
        io_service_.reset();
        predefined_osc_subscriptions_.clear();
        osc_client_.reset();

        amcp_command_repo_wrapper_.reset();
        amcp_command_repo_.reset();
        amcp_context_factory_.reset();

        destroy_producers_synchronously();
        destroy_consumers_synchronously();
        channels_->clear();

        while (weak_io_service.lock())
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

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

    int add_channel(std::wstring format_desc_str)
    {
        auto format_desc = video_format_repository_.find(format_desc_str);
        if (format_desc.format == video_format::invalid)
            return -1;

        auto weak_client = std::weak_ptr<osc::client>(osc_client_);
        auto channel_id  = static_cast<int>(channels_->size() + 1);
        auto channel     = spl::make_shared<video_channel>(channel_id,
                                                       format_desc,
                                                       accelerator_.create_image_mixer(channel_id),
                                                       [channel_id, weak_client](core::monitor::state channel_state) {
                                                           monitor::state state;
                                                           state[""]["channel"][channel_id] = channel_state;
                                                           auto client                      = weak_client.lock();
                                                           if (client) {
                                                               client->send(std::move(state));
                                                           }
                                                       });

        const std::wstring lifecycle_key = L"lock" + std::to_wstring(channel_id);
        channels_->emplace_back(channel, channel->stage(), lifecycle_key);

        return channel_id;
    }

    void setup_osc(const boost::property_tree::wptree& pt)
    {
        using boost::property_tree::wptree;
        using namespace boost::asio::ip;

        auto default_port                 = pt.get<unsigned short>(L"configuration.osc.default-port", 6250);
        auto disable_send_to_amcp_clients = pt.get(L"configuration.osc.disable-send-to-amcp-clients", false);

        // if (!disable_send_to_amcp_clients && primary_amcp_server_)
        //     primary_amcp_server_->add_client_lifecycle_object_factory(
        //         [=](const std::string& ipv4_address) -> std::pair<std::wstring, std::shared_ptr<void>> {
        //             using namespace boost::asio::ip;

        //             return std::make_pair(std::wstring(L"osc_subscribe"),
        //                                   osc_client_->get_subscription_token(
        //                                       udp::endpoint(address_v4::from_string(ipv4_address), default_port)));
        //         });
    }

    bool add_osc_predefined_client(std::string address, unsigned short port)
    {
        using namespace boost::asio::ip;

        boost::system::error_code ec;
        auto                      ipaddr = address_v4::from_string(address, ec);
        if (!ec) {
            predefined_osc_subscriptions_.push_back(osc_client_->get_subscription_token(udp::endpoint(ipaddr, port)));
            return true;
        } else {
            return false;
        }
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
                                                                       ogl_device,
                                                                       spl::make_shared_ptr(osc_client_));

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

bool server::add_osc_predefined_client(std::string address, unsigned short port)
{
    return impl_->add_osc_predefined_client(address, port);
}

int server::add_consumer_from_xml(int channel_index, const boost::property_tree::wptree& config)
{
    return impl_->add_consumer_from_xml(channel_index, config);
}
int server::add_channel(std::wstring format_desc_str) { return impl_->add_channel(format_desc_str); }

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
