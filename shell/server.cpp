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

#include "stdafx.h"

#include "server.h"
#include "included_modules.h"
#include "default_audio_config.h"

#include <accelerator/accelerator.h>

#include <common/env.h>
#include <common/except.h>
#include <common/utf.h>
#include <common/memory.h>
#include <common/ptree.h>

#include <core/video_channel.h>
#include <core/video_format.h>
#include <core/frame/audio_channel_layout.h>
#include <core/producer/stage.h>
#include <core/producer/frame_producer.h>
#include <core/producer/scene/scene_producer.h>
#include <core/producer/scene/xml_scene_producer.h>
#include <core/producer/text/text_producer.h>
#include <core/producer/color/color_producer.h>
#include <core/consumer/output.h>
#include <core/consumer/syncto/syncto_consumer.h>
#include <core/mixer/mixer.h>
#include <core/mixer/image/image_mixer.h>
#include <core/producer/cg_proxy.h>
#include <core/diagnostics/subject_diagnostics.h>
#include <core/diagnostics/call_context.h>
#include <core/diagnostics/osd_graph.h>
#include <core/diagnostics/graph_to_log_sink.h>

#include <modules/image/consumer/image_consumer.h>

#include <protocol/amcp/AMCPProtocolStrategy.h>
#include <protocol/amcp/amcp_command_repository.h>
#include <protocol/amcp/AMCPCommandsImpl.h>
#include <protocol/cii/CIIProtocolStrategy.h>
#include <protocol/clk/CLKProtocolStrategy.h>
#include <protocol/util/AsyncEventServer.h>
#include <protocol/util/strategy_adapters.h>
#include <protocol/osc/client.h>
#include <protocol/log/tcp_logger_protocol_strategy.h>

#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/asio.hpp>

#include <future>

namespace caspar {

using namespace core;
using namespace protocol;

std::shared_ptr<boost::asio::io_service> create_running_io_service()
{
	auto service = std::make_shared<boost::asio::io_service>();
	// To keep the io_service::run() running although no pending async
	// operations are posted.
	auto work = std::make_shared<boost::asio::io_service::work>(*service);
	auto weak_work = std::weak_ptr<boost::asio::io_service::work>(work);
	auto thread = std::make_shared<boost::thread>([service, weak_work]
	{
		ensure_gpf_handler_installed_for_thread("asio-thread");

		while (auto strong = weak_work.lock())
		{
			try
			{
				service->run();
			}
			catch (...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		}

		CASPAR_LOG(info) << "[asio] Global io_service uninitialized.";
	});

	return std::shared_ptr<boost::asio::io_service>(
			service.get(),
			[service, work, thread](void*) mutable
			{
				CASPAR_LOG(info) << "[asio] Shutting down global io_service.";
				work.reset();
				service->stop();
				if (thread->get_id() != boost::this_thread::get_id())
					thread->join();
				else
					thread->detach();
			});
}

struct server::impl : boost::noncopyable
{
	std::shared_ptr<boost::asio::io_service>			io_service_						= create_running_io_service();
	spl::shared_ptr<monitor::subject>					monitor_subject_;
	spl::shared_ptr<monitor::subject>					diag_subject_					= core::diagnostics::get_or_create_subject();
	accelerator::accelerator							accelerator_;
	std::shared_ptr<amcp::amcp_command_repository>		amcp_command_repo_;
	std::vector<spl::shared_ptr<IO::AsyncEventServer>>	async_servers_;
	std::shared_ptr<IO::AsyncEventServer>				primary_amcp_server_;
	std::shared_ptr<osc::client>						osc_client_						= std::make_shared<osc::client>(io_service_);
	std::vector<std::shared_ptr<void>>					predefined_osc_subscriptions_;
	std::vector<spl::shared_ptr<video_channel>>			channels_;
	spl::shared_ptr<core::cg_producer_registry>			cg_registry_;
	spl::shared_ptr<core::frame_producer_registry>		producer_registry_;
	spl::shared_ptr<core::frame_consumer_registry>		consumer_registry_;
	std::promise<bool>&									shutdown_server_now_;

	explicit impl(std::promise<bool>& shutdown_server_now)
		: accelerator_(env::properties().get(L"configuration.accelerator", L"auto"))
		, producer_registry_(spl::make_shared<core::frame_producer_registry>())
		, consumer_registry_(spl::make_shared<core::frame_consumer_registry>())
		, shutdown_server_now_(shutdown_server_now)
	{
		core::diagnostics::register_graph_to_log_sink();
		caspar::core::diagnostics::osd::register_sink();
		diag_subject_->attach_parent(monitor_subject_);

		module_dependencies dependencies(
				cg_registry_,
				producer_registry_,
				consumer_registry_);

		initialize_modules(dependencies);
		core::text::init(dependencies);
		core::init_cg_proxy_as_producer(dependencies);
		core::scene::init(dependencies);
		core::syncto::init(dependencies);
	}

	void start()
	{
		setup_audio_config(env::properties());
		CASPAR_LOG(info) << L"Initialized audio config.";

		setup_channels(env::properties());
		CASPAR_LOG(info) << L"Initialized channels.";

		setup_controllers(env::properties());
		CASPAR_LOG(info) << L"Initialized controllers.";

		setup_osc(env::properties());
		CASPAR_LOG(info) << L"Initialized osc.";
	}

	~impl()
	{
		std::weak_ptr<boost::asio::io_service> weak_io_service = io_service_;
		io_service_.reset();
		osc_client_.reset();
		amcp_command_repo_.reset();
		primary_amcp_server_.reset();
		async_servers_.clear();
		destroy_producers_synchronously();
		destroy_consumers_synchronously();
		channels_.clear();

		while (weak_io_service.lock())
			boost::this_thread::sleep_for(boost::chrono::milliseconds(100));

		uninitialize_modules();
		core::diagnostics::osd::shutdown();
	}

	void setup_audio_config(const boost::property_tree::wptree& pt)
	{
		using boost::property_tree::wptree;

		auto default_config = get_default_audio_config();

		// Start with the defaults
		audio_channel_layout_repository::get_default()->register_all_layouts(default_config.get_child(L"audio.channel-layouts"));
		audio_mix_config_repository::get_default()->register_all_configs(default_config.get_child(L"audio.mix-configs"));

		// Merge with user configuration (adds to or overwrites the defaults)
		auto custom_channel_layouts	= pt.get_child_optional(L"configuration.audio.channel-layouts");
		auto custom_mix_configs		= pt.get_child_optional(L"configuration.audio.mix-configs");

		if (custom_channel_layouts)
		{
			CASPAR_SCOPED_CONTEXT_MSG("/configuration/audio/channel-layouts");
			audio_channel_layout_repository::get_default()->register_all_layouts(*custom_channel_layouts);
		}

		if (custom_mix_configs)
		{
			CASPAR_SCOPED_CONTEXT_MSG("/configuration/audio/mix-configs");
			audio_mix_config_repository::get_default()->register_all_configs(*custom_mix_configs);
		}
	}

	void setup_channels(const boost::property_tree::wptree& pt)
	{
		using boost::property_tree::wptree;

		std::vector<wptree> xml_channels;

		for (auto& xml_channel : pt | witerate_children(L"configuration.channels") | welement_context_iteration)
		{
			xml_channels.push_back(xml_channel.second);
			ptree_verify_element_name(xml_channel, L"channel");

			auto format_desc_str = xml_channel.second.get(L"video-mode", L"PAL");
			auto format_desc = video_format_desc(format_desc_str);
			if(format_desc.format == video_format::invalid)
				CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Invalid video-mode: " + format_desc_str));

			auto channel_layout_str = xml_channel.second.get(L"channel-layout", L"stereo");
			auto channel_layout = core::audio_channel_layout_repository::get_default()->get_layout(channel_layout_str);
			if (!channel_layout)
				CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Unknown channel-layout: " + channel_layout_str));

			auto channel_id = static_cast<int>(channels_.size() + 1);
			auto channel = spl::make_shared<video_channel>(channel_id, format_desc, *channel_layout, accelerator_.create_image_mixer(channel_id));

			channel->monitor_output().attach_parent(monitor_subject_);
			channel->mixer().set_straight_alpha_output(xml_channel.second.get(L"straight-alpha-output", false));
			channels_.push_back(channel);
		}

		for (auto& channel : channels_)
		{
			core::diagnostics::scoped_call_context save;
			core::diagnostics::call_context::for_thread().video_channel = channel->index();

			for (auto& xml_consumer : xml_channels.at(channel->index() - 1) | witerate_children(L"consumers") | welement_context_iteration)
			{
				auto name = xml_consumer.first;

				try
				{
					if (name != L"<xmlcomment>")
						channel->output().add(consumer_registry_->create_consumer(name, xml_consumer.second, &channel->stage(), channels_));
				}
				catch (const user_error& e)
				{
					CASPAR_LOG_CURRENT_EXCEPTION_AT_LEVEL(debug);
					CASPAR_LOG(error) << get_message_and_context(e) << " Turn on log level debug for stacktrace.";
				}
				catch (...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
				}
			}
		}

		// Dummy diagnostics channel
		if (env::properties().get(L"configuration.channel-grid", false))
		{
			auto channel_id = static_cast<int>(channels_.size() + 1);
			channels_.push_back(spl::make_shared<video_channel>(
					channel_id,
					core::video_format_desc(core::video_format::x576p2500),
					*core::audio_channel_layout_repository::get_default()->get_layout(L"stereo"),
					accelerator_.create_image_mixer(channel_id)));
			channels_.back()->monitor_output().attach_parent(monitor_subject_);
		}
	}

	void setup_osc(const boost::property_tree::wptree& pt)
	{
		using boost::property_tree::wptree;
		using namespace boost::asio::ip;

		monitor_subject_->attach_parent(osc_client_->sink());

		auto default_port =
				pt.get<unsigned short>(L"configuration.osc.default-port", 6250);
		auto disable_send_to_amcp_clients =
				pt.get(L"configuration.osc.disable-send-to-amcp-clients", false);
		auto predefined_clients =
				pt.get_child_optional(L"configuration.osc.predefined-clients");

		if (predefined_clients)
		{
			for (auto& predefined_client : pt | witerate_children(L"configuration.osc.predefined-clients") | welement_context_iteration)
			{
				ptree_verify_element_name(predefined_client, L"predefined-client");

				const auto address =
						ptree_get<std::wstring>(predefined_client.second, L"address");
				const auto port =
						ptree_get<unsigned short>(predefined_client.second, L"port");

				boost::system::error_code ec;
				auto ipaddr = address_v4::from_string(u8(address), ec);
				if (!ec)
					predefined_osc_subscriptions_.push_back(
						osc_client_->get_subscription_token(udp::endpoint(
							ipaddr, port)));
				else
					CASPAR_LOG(warning) << "Invalid OSC client. Must be valid ipv4 address: " << address;
			}
		}

		if (!disable_send_to_amcp_clients && primary_amcp_server_)
			primary_amcp_server_->add_client_lifecycle_object_factory(
					[=] (const std::string& ipv4_address)
							-> std::pair<std::wstring, std::shared_ptr<void>>
					{
						using namespace boost::asio::ip;

						return std::make_pair(
								std::wstring(L"osc_subscribe"),
								osc_client_->get_subscription_token(
										udp::endpoint(
												address_v4::from_string(
														ipv4_address),
												default_port)));
					});
	}

	void setup_controllers(const boost::property_tree::wptree& pt)
	{
		amcp_command_repo_ = spl::make_shared<amcp::amcp_command_repository>(
				channels_,
				cg_registry_,
				producer_registry_,
				consumer_registry_,
				accelerator_.get_ogl_device(),
				shutdown_server_now_);
		amcp::register_commands(*amcp_command_repo_);

		using boost::property_tree::wptree;
		for (auto& xml_controller : pt | witerate_children(L"configuration.controllers") | welement_context_iteration)
		{
			auto name = xml_controller.first;
			auto protocol = ptree_get<std::wstring>(xml_controller.second, L"protocol");

			if(name == L"tcp")
			{
				auto port = ptree_get<unsigned int>(xml_controller.second, L"port");
				auto asyncbootstrapper = spl::make_shared<IO::AsyncEventServer>(
						io_service_,
						create_protocol(protocol, L"TCP Port " + boost::lexical_cast<std::wstring>(port)),
						static_cast<short>(port));
				async_servers_.push_back(asyncbootstrapper);

				if (!primary_amcp_server_ && boost::iequals(protocol, L"AMCP"))
					primary_amcp_server_ = asyncbootstrapper;
			}
			else
				CASPAR_LOG(warning) << "Invalid controller: " << name;
		}
	}

	IO::protocol_strategy_factory<char>::ptr create_protocol(const std::wstring& name, const std::wstring& port_description) const
	{
		using namespace IO;

		if(boost::iequals(name, L"AMCP"))
			return wrap_legacy_protocol("\r\n", spl::make_shared<amcp::AMCPProtocolStrategy>(port_description, spl::make_shared_ptr(amcp_command_repo_)));
		else if(boost::iequals(name, L"CII"))
			return wrap_legacy_protocol("\r\n", spl::make_shared<cii::CIIProtocolStrategy>(channels_, cg_registry_, producer_registry_));
		else if(boost::iequals(name, L"CLOCK"))
			return spl::make_shared<to_unicode_adapter_factory>(
					"ISO-8859-1",
					spl::make_shared<CLK::clk_protocol_strategy_factory>(channels_, cg_registry_, producer_registry_));
		else if (boost::iequals(name, L"LOG"))
			return spl::make_shared<protocol::log::tcp_logger_protocol_strategy_factory>();

		CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Invalid protocol: " + name));
	}
};

server::server(std::promise<bool>& shutdown_server_now) : impl_(new impl(shutdown_server_now)){}
void server::start() { impl_->start(); }
spl::shared_ptr<protocol::amcp::amcp_command_repository> server::get_amcp_command_repository() const { return spl::make_shared_ptr(impl_->amcp_command_repo_); }
core::monitor::subject& server::monitor_output() { return *impl_->monitor_subject_; }

}
