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
#include <core/video_channel.h>
#include <core/video_format.h>

#include <modules/image/consumer/image_consumer.h>

#include <protocol/amcp/AMCPCommandsImpl.h>
#include <protocol/amcp/AMCPProtocolStrategy.h>
#include <protocol/amcp/amcp_command_repository.h>
#include <protocol/amcp/amcp_shared.h>
#include <protocol/osc/client.h>
#include <protocol/util/AsyncEventServer.h>
#include <protocol/util/strategy_adapters.h>
#include <protocol/util/tokenize.h>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>

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
    std::vector<spl::shared_ptr<IO::AsyncEventServer>>     async_servers_;
    std::shared_ptr<IO::AsyncEventServer>                  primary_amcp_server_;
    std::shared_ptr<osc::client>                           osc_client_ = std::make_shared<osc::client>(io_service_);
    std::vector<std::shared_ptr<void>>                     predefined_osc_subscriptions_;
    spl::shared_ptr<std::vector<protocol::amcp::channel_context>> channels_;
    spl::shared_ptr<core::cg_producer_registry>                   cg_registry_;
    spl::shared_ptr<core::frame_producer_registry>                producer_registry_;
    spl::shared_ptr<core::frame_consumer_registry>                consumer_registry_;
    std::function<void(bool)>                                     shutdown_server_now_;

    impl(const impl&)            = delete;
    impl& operator=(const impl&) = delete;

    explicit impl(std::function<void(bool)> shutdown_server_now)
        : video_format_repository_()
        , accelerator_(video_format_repository_)
        , producer_registry_(spl::make_shared<core::frame_producer_registry>())
        , consumer_registry_(spl::make_shared<core::frame_consumer_registry>())
        , shutdown_server_now_(std::move(shutdown_server_now))
    {
        caspar::core::diagnostics::osd::register_sink();
    }

    void start()
    {
        setup_video_modes(env::properties());
        CASPAR_LOG(info) << L"Initialized video modes.";

        auto xml_channels = setup_channels(env::properties());
        CASPAR_LOG(info) << L"Initialized channels.";

        setup_amcp_command_repo();
        CASPAR_LOG(info) << L"Initialized command repository.";

        module_dependencies dependencies(
            cg_registry_, producer_registry_, consumer_registry_, amcp_command_repo_wrapper_);
        initialize_modules(dependencies);
        CASPAR_LOG(info) << L"Initialized modules.";

        setup_channel_producers_and_consumers(xml_channels);
        CASPAR_LOG(info) << L"Initialized startup producers.";

        setup_controllers(env::properties());
        CASPAR_LOG(info) << L"Initialized controllers.";

        setup_osc(env::properties());
        CASPAR_LOG(info) << L"Initialized osc.";
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

        primary_amcp_server_.reset();
        async_servers_.clear();

        destroy_producers_synchronously();
        destroy_consumers_synchronously();
        channels_->clear();

        while (weak_io_service.lock())
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        uninitialize_modules();
        core::diagnostics::osd::shutdown();
    }

    void setup_video_modes(const boost::property_tree::wptree& pt)
    {
        using boost::property_tree::wptree;

        auto videomodes_config = pt.get_child_optional(L"configuration.video-modes");
        if (videomodes_config) {
            for (auto& xml_channel :
                 pt | witerate_children(L"configuration.video-modes") | welement_context_iteration) {
                ptree_verify_element_name(xml_channel, L"video-mode");

                const std::wstring id = xml_channel.second.get(L"id", L"");
                if (id == L"")
                    CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Invalid video-mode id: " + id));

                const int width  = xml_channel.second.get<int>(L"width", 0);
                const int height = xml_channel.second.get<int>(L"height", 0);
                if (width == 0 || height == 0)
                    CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Invalid dimensions: " +
                                                                    boost::lexical_cast<std::wstring>(width) + L"x" +
                                                                    boost::lexical_cast<std::wstring>(height)));

                const int field_count = xml_channel.second.get<int>(L"field-count", 1);
                if (field_count != 1 && field_count != 2)
                    CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Invalid field-count: " +
                                                                    boost::lexical_cast<std::wstring>(field_count)));

                const int timescale = xml_channel.second.get<int>(L"time-scale", 60000);
                const int duration  = xml_channel.second.get<int>(L"duration", 1000);
                if (timescale == 0 || duration == 0)
                    CASPAR_THROW_EXCEPTION(
                        user_error() << msg_info(L"Invalid framerate: " + boost::lexical_cast<std::wstring>(timescale) +
                                                 L"/" + boost::lexical_cast<std::wstring>(duration)));

                std::vector<int> cadence;
                int              cadence_sum = 0;

                const std::wstring     cadence_str = xml_channel.second.get(L"cadence", L"");
                std::set<std::wstring> cadence_parts;
                boost::split(cadence_parts, cadence_str, boost::is_any_of(L", "));

                for (auto& cad : cadence_parts) {
                    if (cad == L"")
                        continue;

                    const int c = std::stoi(cad);
                    cadence.push_back(c);
                    cadence_sum += c;
                }

                if (cadence.size() == 0) {
                    const int c = static_cast<int>(48000 / (static_cast<double>(timescale) / duration) + 0.5);
                    cadence.push_back(c);
                    cadence_sum += c;
                }

                const auto new_format = video_format_desc(
                    video_format::custom, field_count, width, height, width, height, timescale, duration, id, cadence);
                // TODO: verify cadence_sum to look correct

                const auto existing = video_format_repository_.find(id);
                if (existing.format != video_format::invalid)
                    CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Video-mode already exists: " + id));

                video_format_repository_.store(new_format);
            }
        }
    }

    std::vector<boost::property_tree::wptree> setup_channels(const boost::property_tree::wptree& pt)
    {
        using boost::property_tree::wptree;

        std::vector<wptree> xml_channels;

        for (auto& xml_channel : pt | witerate_children(L"configuration.channels") | welement_context_iteration) {
            xml_channels.push_back(xml_channel.second);
            ptree_verify_element_name(xml_channel, L"channel");

            auto format_desc_str = xml_channel.second.get(L"video-mode", L"PAL");
            auto format_desc     = video_format_repository_.find(format_desc_str);
            if (format_desc.format == video_format::invalid)
                CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Invalid video-mode: " + format_desc_str));

            auto weak_client = std::weak_ptr<osc::client>(osc_client_);
            auto channel_id  = static_cast<int>(channels_->size() + 1);
            auto channel =
                spl::make_shared<video_channel>(channel_id,
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
        }

        return xml_channels;
    }

    void setup_osc(const boost::property_tree::wptree& pt)
    {
        using boost::property_tree::wptree;
        using namespace boost::asio::ip;

        auto default_port                 = pt.get<unsigned short>(L"configuration.osc.default-port", 6250);
        auto disable_send_to_amcp_clients = pt.get(L"configuration.osc.disable-send-to-amcp-clients", false);
        auto predefined_clients           = pt.get_child_optional(L"configuration.osc.predefined-clients");

        if (predefined_clients) {
            for (auto& predefined_client :
                 pt | witerate_children(L"configuration.osc.predefined-clients") | welement_context_iteration) {
                ptree_verify_element_name(predefined_client, L"predefined-client");

                const auto address = ptree_get<std::wstring>(predefined_client.second, L"address");
                const auto port    = ptree_get<unsigned short>(predefined_client.second, L"port");

                boost::system::error_code ec;
                auto                      ipaddr = address_v4::from_string(u8(address), ec);
                if (!ec)
                    predefined_osc_subscriptions_.push_back(
                        osc_client_->get_subscription_token(udp::endpoint(ipaddr, port)));
                else
                    CASPAR_LOG(warning) << "Invalid OSC client. Must be valid ipv4 address: " << address;
            }
        }

        if (!disable_send_to_amcp_clients && primary_amcp_server_)
            primary_amcp_server_->add_client_lifecycle_object_factory(
                [=](const std::string& ipv4_address) -> std::pair<std::wstring, std::shared_ptr<void>> {
                    using namespace boost::asio::ip;

                    return std::make_pair(std::wstring(L"osc_subscribe"),
                                          osc_client_->get_subscription_token(
                                              udp::endpoint(address_v4::from_string(ipv4_address), default_port)));
                });
    }

    void setup_channel_producers_and_consumers(const std::vector<boost::property_tree::wptree>& xml_channels)
    {
        auto console_client = spl::make_shared<IO::ConsoleClientInfo>();

        std::vector<spl::shared_ptr<core::video_channel>> channels_vec;
        for (auto& cc : *channels_) {
            channels_vec.emplace_back(cc.raw_channel);
        }

        for (auto& channel : *channels_) {
            core::diagnostics::scoped_call_context save;
            core::diagnostics::call_context::for_thread().video_channel = channel.raw_channel->index();

            auto xml_channel = xml_channels.at(channel.raw_channel->index() - 1);

            // Consumers
            if (xml_channel.get_child_optional(L"consumers")) {
                for (auto& xml_consumer : xml_channel | witerate_children(L"consumers") | welement_context_iteration) {
                    auto name = xml_consumer.first;

                    try {
                        if (name != L"<xmlcomment>")
                            channel.raw_channel->output().add(consumer_registry_->create_consumer(
                                name, xml_consumer.second, video_format_repository_, channels_vec));
                    } catch (...) {
                        CASPAR_LOG_CURRENT_EXCEPTION();
                    }
                }
            }

            // Producers
            if (xml_channel.get_child_optional(L"producers")) {
                for (auto& xml_producer : xml_channel | witerate_children(L"producers") | welement_context_iteration) {
                    ptree_verify_element_name(xml_producer, L"producer");

                    const std::wstring command = xml_producer.second.get_value(L"");
                    const auto         attrs   = xml_producer.second.get_child(L"<xmlattr>");
                    const int          id      = attrs.get(L"id", -1);

                    try {
                        std::list<std::wstring> tokens{
                            L"PLAY", (boost::wformat(L"%i-%i") % channel.raw_channel->index() % id).str()};
                        IO::tokenize(command, tokens);
                        auto cmd = amcp_command_repo_->parse_command(console_client, tokens, L"");

                        if (cmd) {
                            std::wstring res = cmd->Execute(channels_).get();
                            console_client->send(std::move(res), false);
                        }
                    } catch (const user_error&) {
                        CASPAR_LOG(error) << "Failed to parse command: " << command;
                    } catch (...) {
                        CASPAR_LOG_CURRENT_EXCEPTION();
                    }
                }
            }
        }
    }

    void setup_amcp_command_repo()
    {
        amcp_command_repo_ = std::make_shared<amcp::amcp_command_repository>(channels_);

        auto ogl_device = accelerator_.get_device();
        auto ctx        = std::make_shared<amcp::amcp_command_static_context>(
            video_format_repository_,
            cg_registry_,
            producer_registry_,
            consumer_registry_,
            amcp_command_repo_,
            shutdown_server_now_,
            u8(caspar::env::properties().get(L"configuration.amcp.media-server.host", L"127.0.0.1")),
            u8(caspar::env::properties().get(L"configuration.amcp.media-server.port", L"8000")),
            ogl_device,
            spl::make_shared_ptr(osc_client_));

        amcp_context_factory_ = std::make_shared<amcp::command_context_factory>(ctx);

        amcp_command_repo_wrapper_ =
            std::make_shared<amcp::amcp_command_repository_wrapper>(amcp_command_repo_, amcp_context_factory_);

        amcp::register_commands(amcp_command_repo_wrapper_);
    }

    void setup_controllers(const boost::property_tree::wptree& pt)
    {
        using boost::property_tree::wptree;
        for (auto& xml_controller : pt | witerate_children(L"configuration.controllers") | welement_context_iteration) {
            auto name     = xml_controller.first;
            auto protocol = ptree_get<std::wstring>(xml_controller.second, L"protocol");

            if (name == L"tcp") {
                auto port = ptree_get<unsigned int>(xml_controller.second, L"port");

                try {
                    auto asyncbootstrapper = spl::make_shared<IO::AsyncEventServer>(
                        io_service_,
                        create_protocol(protocol, L"TCP Port " + std::to_wstring(port)),
                        static_cast<short>(port));
                    async_servers_.push_back(asyncbootstrapper);

                    if (!primary_amcp_server_ && boost::iequals(protocol, L"AMCP"))
                        primary_amcp_server_ = asyncbootstrapper;
                } catch (...) {
                    CASPAR_LOG(fatal) << L"Failed to setup " << protocol << L" controller on port "
                                      << boost::lexical_cast<std::wstring>(port) << L". It is likely already in use";
                    throw;
                    // CASPAR_LOG_CURRENT_EXCEPTION();
                }
            } else
                CASPAR_LOG(warning) << "Invalid controller: " << name;
        }
    }

    IO::protocol_strategy_factory<char>::ptr create_protocol(const std::wstring& name,
                                                             const std::wstring& port_description) const
    {
        using namespace IO;

        if (boost::iequals(name, L"AMCP"))
            return amcp::create_char_amcp_strategy_factory(port_description, spl::make_shared_ptr(amcp_command_repo_));

        CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Invalid protocol: " + name));
    }
};

server::server(std::function<void(bool)> shutdown_server_now)
    : impl_(new impl(std::move(shutdown_server_now)))
{
}
void                                                     server::start() { impl_->start(); }
spl::shared_ptr<protocol::amcp::amcp_command_repository> server::get_amcp_command_repository() const
{
    return spl::make_shared_ptr(impl_->amcp_command_repo_);
}

} // namespace caspar
