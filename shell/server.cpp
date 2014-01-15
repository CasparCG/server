/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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


#include "server.h"

#include <memory>

#include <common/env.h>
#include <common/exception/exceptions.h>
#include <common/utility/string.h>
#include <common/filesystem/polling_filesystem_monitor.h>

#include <core/mixer/gpu/ogl_device.h>
#include <core/mixer/audio/audio_util.h>
#include <core/mixer/mixer.h>
#include <core/video_channel.h>
#include <core/producer/stage.h>
#include <core/consumer/output.h>
#include <core/consumer/synchronizing/synchronizing_consumer.h>
#include <core/thumbnail_generator.h>
#include <core/producer/media_info/media_info.h>
#include <core/producer/media_info/media_info_repository.h>
#include <core/producer/media_info/in_memory_media_info_repository.h>

#include <modules/bluefish/bluefish.h>
#include <modules/decklink/decklink.h>
#include <modules/ffmpeg/ffmpeg.h>
#include <modules/flash/flash.h>
#include <modules/html/html.h>
#include <modules/oal/oal.h>
#include <modules/ogl/ogl.h>
#include <modules/newtek/newtek.h>
#include <modules/image/image.h>
#include <modules/image/consumer/image_consumer.h>

#include <modules/oal/consumer/oal_consumer.h>
#include <modules/bluefish/consumer/bluefish_consumer.h>
#include <modules/newtek/consumer/newtek_ivga_consumer.h>
#include <modules/decklink/consumer/decklink_consumer.h>
#include <modules/decklink/consumer/blocking_decklink_consumer.h>
#include <modules/ogl/consumer/ogl_consumer.h>
#include <modules/ffmpeg/consumer/ffmpeg_consumer.h>

#include <protocol/amcp/AMCPProtocolStrategy.h>
#include <protocol/cii/CIIProtocolStrategy.h>
#include <protocol/CLK/CLKProtocolStrategy.h>
#include <protocol/util/AsyncEventServer.h>
#include <protocol/util/stateful_protocol_strategy_wrapper.h>
#include <protocol/osc/client.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/asio.hpp>

#include <tbb/atomic.h>

namespace caspar {

using namespace core;
using namespace protocol;

std::shared_ptr<boost::asio::io_service> create_running_io_service()
{
	auto service = std::make_shared<boost::asio::io_service>();
	// To keep the io_service::run() running although no pending async
	// operations are posted.
	auto work = std::make_shared<boost::asio::io_service::work>(*service);
	auto thread = std::make_shared<boost::thread>([service]
	{
		win32_exception::ensure_handler_installed_for_thread("asio-thread");

		service->run();
	});

	return std::shared_ptr<boost::asio::io_service>(
			service.get(),
			[service, work, thread] (void*) mutable
			{
				work.reset();
				service->stop();
				thread->join();
			});
}

struct server::implementation : boost::noncopyable
{
	std::shared_ptr<boost::asio::io_service>	io_service_;
	safe_ptr<core::monitor::subject>			monitor_subject_;
	boost::promise<bool>&						shutdown_server_now_;
	safe_ptr<ogl_device>						ogl_;
	std::vector<safe_ptr<IO::AsyncEventServer>> async_servers_;	
	std::shared_ptr<IO::AsyncEventServer>		primary_amcp_server_;
	osc::client									osc_client_;
	std::vector<std::shared_ptr<void>>			predefined_osc_subscriptions_;
	std::vector<safe_ptr<video_channel>>		channels_;
	safe_ptr<media_info_repository>				media_info_repo_;
	boost::thread								initial_media_info_thread_;
	tbb::atomic<bool>							running_;
	std::shared_ptr<thumbnail_generator>		thumbnail_generator_;

	implementation(boost::promise<bool>& shutdown_server_now)
		: io_service_(create_running_io_service())
		, shutdown_server_now_(shutdown_server_now)
		, ogl_(ogl_device::create())
		, osc_client_(io_service_)
		, media_info_repo_(create_in_memory_media_info_repository())
	{
		running_ = true;
		setup_audio(env::properties());
		
		html::init();
		CASPAR_LOG(info) << L"Initialized html module.";

		ffmpeg::init(media_info_repo_);
		CASPAR_LOG(info) << L"Initialized ffmpeg module.";
							  
		bluefish::init();	  
		CASPAR_LOG(info) << L"Initialized bluefish module.";
							  
		decklink::init();	  
		CASPAR_LOG(info) << L"Initialized decklink module.";

		oal::init();
		CASPAR_LOG(info) << L"Initialized oal module.";
							  
		newtek::init();
		CASPAR_LOG(info) << L"Initialized newtek module.";

		ogl::init();		  
		CASPAR_LOG(info) << L"Initialized ogl module.";

		flash::init();		  
		CASPAR_LOG(info) << L"Initialized flash module.";

		image::init();		  
		CASPAR_LOG(info) << L"Initialized image module.";

		setup_channels(env::properties());
		CASPAR_LOG(info) << L"Initialized channels.";

		setup_thumbnail_generation(env::properties());

		setup_controllers(env::properties());
		CASPAR_LOG(info) << L"Initialized controllers.";

		setup_osc(env::properties());
		CASPAR_LOG(info) << L"Initialized osc.";

		start_initial_media_info_scan();
		CASPAR_LOG(info) << L"Started initial media information retrieval.";
	}

	~implementation()
	{
		running_ = false;
		initial_media_info_thread_.join();
		thumbnail_generator_.reset();
		primary_amcp_server_.reset();
		async_servers_.clear();
		destroy_producers_synchronously();
		channels_.clear();

		html::uninit();
		ffmpeg::uninit();
	}

	void setup_audio(const boost::property_tree::wptree& pt)
	{
		register_default_channel_layouts(default_channel_layout_repository());
		register_default_mix_configs(default_mix_config_repository());

		auto channel_layouts =
			pt.get_child_optional(L"configuration.audio.channel-layouts");
		auto mix_configs =
			pt.get_child_optional(L"configuration.audio.mix-configs");

		if (channel_layouts)
			parse_channel_layouts(
					default_channel_layout_repository(), *channel_layouts);

		if (mix_configs)
			parse_mix_configs(
					default_mix_config_repository(), *mix_configs);
	}
				
	void setup_channels(const boost::property_tree::wptree& pt)
	{   
		using boost::property_tree::wptree;
		BOOST_FOREACH(auto& xml_channel, pt.get_child(L"configuration.channels"))
		{		
			auto format_desc = video_format_desc::get(widen(xml_channel.second.get(L"video-mode", L"PAL")));		
			if(format_desc.format == video_format::invalid)
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Invalid video-mode."));
			auto audio_channel_layout = default_channel_layout_repository().get_by_name(
					boost::to_upper_copy(xml_channel.second.get(L"channel-layout", L"STEREO")));
			
			channels_.push_back(make_safe<video_channel>(channels_.size()+1, format_desc, ogl_, audio_channel_layout));
			
			channels_.back()->monitor_output().attach_parent(monitor_subject_);
			channels_.back()->mixer()->set_straight_alpha_output(
					xml_channel.second.get(L"straight-alpha-output", false));

			create_consumers(
				xml_channel.second.get_child(L"consumers"),
				[&] (const safe_ptr<core::frame_consumer>& consumer)
				{
					channels_.back()->output()->add(consumer);
				});
		}

		// Dummy diagnostics channel
		if(env::properties().get(L"configuration.channel-grid", false))
			channels_.push_back(make_safe<video_channel>(channels_.size()+1, core::video_format_desc::get(core::video_format::x576p2500), ogl_, default_channel_layout_repository().get_by_name(L"STEREO")));
	}

	template<typename Base>
	std::vector<safe_ptr<Base>> create_consumers(const boost::property_tree::wptree& pt)
	{
		std::vector<safe_ptr<Base>> consumers;

		create_consumers(pt, [&] (const safe_ptr<core::frame_consumer>& consumer)
		{
			consumers.push_back(dynamic_pointer_cast<Base>(consumer));
		});

		return consumers;
	}

	template<class Func>
	void create_consumers(const boost::property_tree::wptree& pt, const Func& on_consumer)
	{
		BOOST_FOREACH(auto& xml_consumer, pt)
		{
			try
			{
				auto name = xml_consumer.first;

				if (name == L"screen")
					on_consumer(ogl::create_consumer(xml_consumer.second));
				else if (name == L"bluefish")					
					on_consumer(bluefish::create_consumer(xml_consumer.second));					
				else if (name == L"decklink")					
					on_consumer(decklink::create_consumer(xml_consumer.second));				
				else if (name == L"newtek-ivga")					
					on_consumer(newtek::create_ivga_consumer(xml_consumer.second));			
				else if (name == L"blocking-decklink")
					on_consumer(decklink::create_blocking_consumer(xml_consumer.second));				
				else if (name == L"file" || name == L"stream")					
					on_consumer(ffmpeg::create_consumer(xml_consumer.second));						
				else if (name == L"system-audio")
					on_consumer(oal::create_consumer());
				else if (name == L"synchronizing")
					on_consumer(make_safe<core::synchronizing_consumer>(
							create_consumers<core::frame_consumer>(
									xml_consumer.second)));
				else if (name != L"<xmlcomment>")
					CASPAR_LOG(warning) << "Invalid consumer: " << widen(name);	
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		}
	}
		
	void setup_controllers(const boost::property_tree::wptree& pt)
	{		
		using boost::property_tree::wptree;
		BOOST_FOREACH(auto& xml_controller, pt.get_child(L"configuration.controllers"))
		{
			try
			{
				auto name = xml_controller.first;
				auto protocol = xml_controller.second.get<std::wstring>(L"protocol");	

				if(name == L"tcp")
				{					
					unsigned int port = xml_controller.second.get(L"port", 5250);
					auto asyncbootstrapper = make_safe<IO::AsyncEventServer>(create_protocol(protocol), port);
					asyncbootstrapper->Start();
					async_servers_.push_back(asyncbootstrapper);

					if (!primary_amcp_server_ && boost::iequals(protocol, L"AMCP"))
						primary_amcp_server_ = asyncbootstrapper;
				}
				else
					CASPAR_LOG(warning) << "Invalid controller: " << widen(name);	
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		}
	}

	void setup_osc(const boost::property_tree::wptree& pt)
	{		
		using boost::property_tree::wptree;
		using namespace boost::asio::ip;

		monitor_subject_->attach_parent(osc_client_.sink());
		
		auto default_port =
				pt.get<unsigned short>(L"configuration.osc.default-port", 6250);
		auto predefined_clients =
				pt.get_child_optional(L"configuration.osc.predefined-clients");

		if (predefined_clients)
		{
			BOOST_FOREACH(auto& predefined_client, *predefined_clients)
			{
				const auto address =
						predefined_client.second.get<std::wstring>(L"address");
				const auto port =
						predefined_client.second.get<unsigned short>(L"port");
				predefined_osc_subscriptions_.push_back(
						osc_client_.get_subscription_token(udp::endpoint(
								address_v4::from_string(narrow(address)),
								port)));
			}
		}

		if (primary_amcp_server_)
			primary_amcp_server_->add_lifecycle_factory(
					[=] (const std::string& ipv4_address)
							-> std::shared_ptr<void>
					{
						using namespace boost::asio::ip;

						return osc_client_.get_subscription_token(
								udp::endpoint(
										address_v4::from_string(ipv4_address),
										default_port));
					});
	}

	void setup_thumbnail_generation(const boost::property_tree::wptree& pt)
	{
		if (!pt.get(L"configuration.thumbnails.generate-thumbnails", true))
			return;

		auto scan_interval_millis = pt.get(L"configuration.thumbnails.scan-interval-millis", 5000);

		polling_filesystem_monitor_factory monitor_factory(
				io_service_, scan_interval_millis);
		thumbnail_generator_.reset(new thumbnail_generator(
				monitor_factory, 
				env::media_folder(),
				env::thumbnails_folder(),
				pt.get(L"configuration.thumbnails.width", 256),
				pt.get(L"configuration.thumbnails.height", 144),
				core::video_format_desc::get(pt.get(L"configuration.thumbnails.video-mode", L"720p2500")),
				ogl_,
				pt.get(L"configuration.thumbnails.generate-delay-millis", 2000),
				&image::write_cropped_png,
				media_info_repo_));

		CASPAR_LOG(info) << L"Initialized thumbnail generator.";
	}

	safe_ptr<IO::IProtocolStrategy> create_protocol(const std::wstring& name) const
	{
		if(boost::iequals(name, L"AMCP"))
			return make_safe<amcp::AMCPProtocolStrategy>(channels_, thumbnail_generator_, media_info_repo_, shutdown_server_now_);
		else if(boost::iequals(name, L"CII"))
			return make_safe<cii::CIIProtocolStrategy>(channels_);
		else if(boost::iequals(name, L"CLOCK"))
			//return make_safe<CLK::CLKProtocolStrategy>(channels_);
			return make_safe<IO::stateful_protocol_strategy_wrapper>([=]
			{
				return std::make_shared<CLK::CLKProtocolStrategy>(channels_);
			});
		
		BOOST_THROW_EXCEPTION(caspar_exception() << arg_name_info("name") << arg_value_info(narrow(name)) << msg_info("Invalid protocol"));
	}

	void start_initial_media_info_scan()
	{
		initial_media_info_thread_ = boost::thread([this]
		{
			for (boost::filesystem::wrecursive_directory_iterator iter(env::media_folder()), end; iter != end; ++iter)
			{
				if (running_)
					media_info_repo_->get(iter->path().file_string());
				else
				{
					CASPAR_LOG(info) << L"Initial media information retrieval aborted.";
					return;
				}
			}

			CASPAR_LOG(info) << L"Initial media information retrieval finished.";
		});
	}
};

server::server(boost::promise<bool>& shutdown_server_now) : impl_(new implementation(shutdown_server_now)){}

const std::vector<safe_ptr<video_channel>> server::get_channels() const
{
	return impl_->channels_;
}

std::shared_ptr<thumbnail_generator> server::get_thumbnail_generator() const
{
	return impl_->thumbnail_generator_;
}

safe_ptr<media_info_repository> server::get_media_info_repo() const
{
	return impl_->media_info_repo_;
}

core::monitor::subject& server::monitor_output()
{
	return *impl_->monitor_subject_;
}

}