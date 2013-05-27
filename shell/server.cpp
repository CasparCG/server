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


#include "server.h"

#include <memory>

#include <common/env.h>
#include <common/exception/exceptions.h>
#include <common/utility/string.h>
#include <common/filesystem/polling_filesystem_monitor.h>

#include <core/mixer/gpu/ogl_device.h>
#include <core/mixer/audio/audio_util.h>
#include <core/video_channel.h>
#include <core/producer/stage.h>
#include <core/consumer/output.h>
#include <core/consumer/synchronizing/synchronizing_consumer.h>
#include <core/thumbnail_generator.h>

#include <modules/bluefish/bluefish.h>
#include <modules/decklink/decklink.h>
#include <modules/ffmpeg/ffmpeg.h>
#include <modules/flash/flash.h>
#include <modules/oal/oal.h>
#include <modules/ogl/ogl.h>
#include <modules/silverlight/silverlight.h>
#include <modules/image/image.h>
#include <modules/image/consumer/image_consumer.h>

#include <modules/oal/consumer/oal_consumer.h>
#include <modules/bluefish/consumer/bluefish_consumer.h>
#include <modules/decklink/consumer/decklink_consumer.h>
#include <modules/ogl/consumer/ogl_consumer.h>
#include <modules/ffmpeg/consumer/ffmpeg_consumer.h>

#include <protocol/amcp/AMCPProtocolStrategy.h>
#include <protocol/cii/CIIProtocolStrategy.h>
#include <protocol/CLK/CLKProtocolStrategy.h>
#include <protocol/util/AsyncEventServer.h>
#include <protocol/util/stateful_protocol_strategy_wrapper.h>
#include <protocol/osc/client.h>
#include <protocol/asio/io_service_manager.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace caspar {

using namespace core;
using namespace protocol;

struct server::implementation : boost::noncopyable
{
	protocol::asio::io_service_manager			io_service_manager_;
	core::monitor::subject						monitor_subject_;
	core::monitor::multi_target					multi_target_;
	boost::promise<bool>&						shutdown_server_now_;
	safe_ptr<ogl_device>						ogl_;
	std::vector<safe_ptr<IO::AsyncEventServer>> async_servers_;	
	std::shared_ptr<IO::AsyncEventServer>		primary_amcp_server_;
	std::vector<osc::client>					osc_clients_;
	std::vector<safe_ptr<video_channel>>		channels_;
	std::shared_ptr<thumbnail_generator>		thumbnail_generator_;

	implementation(boost::promise<bool>& shutdown_server_now)
		: shutdown_server_now_(shutdown_server_now)
		, ogl_(ogl_device::create())
	{
		monitor_subject_.link_target(&multi_target_);
		setup_audio(env::properties());

		ffmpeg::init();
		CASPAR_LOG(info) << L"Initialized ffmpeg module.";
							  
		bluefish::init();	  
		CASPAR_LOG(info) << L"Initialized bluefish module.";
							  
		decklink::init();	  
		CASPAR_LOG(info) << L"Initialized decklink module.";
							  							  
		oal::init();		  
		CASPAR_LOG(info) << L"Initialized oal module.";
							  
		ogl::init();		  
		CASPAR_LOG(info) << L"Initialized ogl module.";

		image::init();		  
		CASPAR_LOG(info) << L"Initialized image module.";

		flash::init();		  
		CASPAR_LOG(info) << L"Initialized flash module.";

		setup_channels(env::properties());
		CASPAR_LOG(info) << L"Initialized channels.";

		setup_thumbnail_generation(env::properties());

		setup_controllers(env::properties());
		CASPAR_LOG(info) << L"Initialized controllers.";

		setup_osc(env::properties());
		CASPAR_LOG(info) << L"Initialized osc.";
	}

	~implementation()
	{		
		ffmpeg::uninit();

		async_servers_.clear();
		channels_.clear();
	}

	void setup_audio(const boost::property_tree::wptree& pt)
	{
		register_default_channel_layouts(default_channel_layout_repository());
		register_default_mix_configs(default_mix_config_repository());
		parse_channel_layouts(
				default_channel_layout_repository(),
				pt.get_child(L"configuration.audio.channel-layouts"));
		parse_mix_configs(
				default_mix_config_repository(),
				pt.get_child(L"configuration.audio.mix-configs"));
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
			
			channels_.back()->monitor_output().link_target(&monitor_subject_);

			create_consumers(
				xml_channel.second.get_child(L"consumers"),
				[&] (const safe_ptr<core::frame_consumer>& consumer)
				{
					channels_.back()->output()->add(consumer);
				});

			// Add all consumers before starting channel.
			channels_.back()->start_channel();
		}

		// Dummy diagnostics channel
		if(env::properties().get(L"configuration.channel-grid", false))
		{
			channels_.push_back(make_safe<video_channel>(channels_.size()+1, core::video_format_desc::get(core::video_format::x576p2500), ogl_, default_channel_layout_repository().get_by_name(L"STEREO")));
			channels_.back()->start_channel();
		}
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
				else if (name == L"file")					
					on_consumer(ffmpeg::create_consumer(xml_consumer.second));						
				else if (name == L"system-audio")
					on_consumer(oal::create_consumer());
				else if (name == L"synchronizing")
					on_consumer(make_safe<core::synchronizing_consumer>(create_consumers<core::synchronizable_consumer>(xml_consumer.second)));
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
				osc_clients_.push_back(osc::client(
						io_service_manager_.service(),
						udp::endpoint(
								address_v4::from_string(narrow(address)),
								port),
						multi_target_));
			}
		}

		if (primary_amcp_server_)
			primary_amcp_server_->add_lifecycle_factory(
					[=] (const std::string& ipv4_address)
							-> std::shared_ptr<void>
					{
						using namespace boost::asio::ip;

						return std::make_shared<osc::client>(
								io_service_manager_.service(),
								udp::endpoint(
										address_v4::from_string(ipv4_address),
										default_port),
								multi_target_);
					});
	}

	void setup_thumbnail_generation(const boost::property_tree::wptree& pt)
	{
		if (!pt.get(L"configuration.thumbnails.generate-thumbnails", true))
			return;

		auto scan_interval_millis = pt.get(L"configuration.thumbnails.scan-interval-millis", 5000);

		polling_filesystem_monitor_factory monitor_factory(scan_interval_millis);
		thumbnail_generator_.reset(new thumbnail_generator(
				monitor_factory, 
				env::media_folder(),
				env::thumbnails_folder(),
				pt.get(L"configuration.thumbnails.width", 256),
				pt.get(L"configuration.thumbnails.height", 144),
				core::video_format_desc::get(pt.get(L"configuration.thumbnails.video-mode", L"720p2500")),
				ogl_,
				pt.get(L"configuration.thumbnails.generate-delay-millis", 2000),
				&image::write_cropped_png));

		CASPAR_LOG(info) << L"Initialized thumbnail generator.";
	}

	safe_ptr<IO::IProtocolStrategy> create_protocol(const std::wstring& name) const
	{
		if(boost::iequals(name, L"AMCP"))
			return make_safe<amcp::AMCPProtocolStrategy>(channels_, thumbnail_generator_, shutdown_server_now_);
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

core::monitor::source& server::monitor_output()
{
	return impl_->monitor_subject_;
}

}