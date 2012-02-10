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

#include <accelerator/factory.h>
#include <accelerator/ogl/factory.h>
#include <accelerator/cpu/factory.h>

#include <common/env.h>
#include <common/except.h>
#include <common/utf.h>

#include <core/video_channel.h>
#include <core/video_format.h>
#include <core/producer/stage.h>
#include <core/consumer/output.h>

#include <modules/bluefish/bluefish.h>
#include <modules/decklink/decklink.h>
#include <modules/ffmpeg/ffmpeg.h>
#include <modules/flash/flash.h>
#include <modules/oal/oal.h>
#include <modules/screen/screen.h>
#include <modules/image/image.h>

#include <modules/oal/consumer/oal_consumer.h>
#include <modules/bluefish/consumer/bluefish_consumer.h>
#include <modules/decklink/consumer/decklink_consumer.h>
#include <modules/screen/consumer/screen_consumer.h>
#include <modules/ffmpeg/consumer/ffmpeg_consumer.h>

#include <protocol/amcp/AMCPProtocolStrategy.h>
#include <protocol/cii/CIIProtocolStrategy.h>
#include <protocol/CLK/CLKProtocolStrategy.h>
#include <protocol/util/AsyncEventServer.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace caspar {

using namespace core;
using namespace protocol;

struct server::impl : boost::noncopyable
{
	spl::shared_ptr<monitor::subject>					event_subject_;
	std::unique_ptr<accelerator::factory>				accel_factory_;
	std::vector<spl::shared_ptr<IO::AsyncEventServer>>	async_servers_;	
	std::vector<spl::shared_ptr<video_channel>>			channels_;

	impl()		
	{	
		auto accel_str = env::properties().get(L"configuration.accelerator", L"auto");

		if(accel_str == L"cpu")
			accel_factory_.reset(new accelerator::cpu::factory());
		else
		{
			try
			{
				accel_factory_.reset(new accelerator::ogl::factory());
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				accel_factory_.reset(new accelerator::cpu::factory());
				CASPAR_LOG(warning) << L"Using fallback CPU mixer.";
			}
		}

		ffmpeg::init();
		CASPAR_LOG(info) << L"Initialized ffmpeg module.";
							  
		bluefish::init();	  
		CASPAR_LOG(info) << L"Initialized bluefish module.";
							  
		decklink::init();	  
		CASPAR_LOG(info) << L"Initialized decklink module.";
							  							  
		oal::init();		  
		CASPAR_LOG(info) << L"Initialized oal module.";
							  
		screen::init();		  
		CASPAR_LOG(info) << L"Initialized ogl module.";

		image::init();		  
		CASPAR_LOG(info) << L"Initialized image module.";

		flash::init();		  
		CASPAR_LOG(info) << L"Initialized flash module.";

		setup_channels(env::properties());
		CASPAR_LOG(info) << L"Initialized channels.";

		setup_controllers(env::properties());
		CASPAR_LOG(info) << L"Initialized controllers.";
	}

	~impl()
	{		
		image::uninit();
		ffmpeg::uninit();

		async_servers_.clear();
		channels_.clear();
	}
				
	void setup_channels(const boost::property_tree::wptree& pt)
	{   
		using boost::property_tree::wptree;
		BOOST_FOREACH(auto& xml_channel, pt.get_child(L"configuration.channels"))
		{		
			auto format_desc = video_format_desc(xml_channel.second.get(L"video-mode", L"PAL"));		
			if(format_desc.format == video_format::invalid)
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Invalid video-mode."));
			
			auto channel = spl::make_shared<video_channel>(static_cast<int>(channels_.size()+1), format_desc, accel_factory_->create_image_mixer());
			
			BOOST_FOREACH(auto& xml_consumer, xml_channel.second.get_child(L"consumers"))
			{
				try
				{
					auto name = xml_consumer.first;
					if(name == L"screen")
						channel->output()->add(caspar::screen::create_consumer(xml_consumer.second));					
					else if(name == L"bluefish")					
						channel->output()->add(bluefish::create_consumer(xml_consumer.second));					
					else if(name == L"decklink")					
						channel->output()->add(decklink::create_consumer(xml_consumer.second));				
					else if(name == L"file")					
						channel->output()->add(ffmpeg::create_consumer(xml_consumer.second));						
					else if(name == L"system-audio")
						channel->output()->add(oal::create_consumer());		
					else if(name != L"<xmlcomment>")
						CASPAR_LOG(warning) << "Invalid consumer: " << name;	
				}
				catch(...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
				}
			}		

		    channel->subscribe(monitor::observable::observer_ptr(event_subject_));
			channels_.push_back(channel);
		}

		// Dummy diagnostics channel
		if(env::properties().get(L"configuration.channel-grid", false))
			channels_.push_back(spl::make_shared<video_channel>(static_cast<int>(channels_.size()+1), core::video_format_desc(core::video_format::x576p2500), accel_factory_->create_image_mixer()));
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
					auto asyncbootstrapper = spl::make_shared<IO::AsyncEventServer>(create_protocol(protocol), port);
					asyncbootstrapper->Start();
					async_servers_.push_back(asyncbootstrapper);
				}
				else
					CASPAR_LOG(warning) << "Invalid controller: " << name;	
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		}
	}

	spl::shared_ptr<IO::IProtocolStrategy> create_protocol(const std::wstring& name) const
	{
		if(boost::iequals(name, L"AMCP"))
			return spl::make_shared<amcp::AMCPProtocolStrategy>(channels_);
		else if(boost::iequals(name, L"CII"))
			return spl::make_shared<cii::CIIProtocolStrategy>(channels_);
		else if(boost::iequals(name, L"CLOCK"))
			return spl::make_shared<CLK::CLKProtocolStrategy>(channels_);
		
		BOOST_THROW_EXCEPTION(caspar_exception() << arg_name_info(L"name") << arg_value_info(name) << msg_info(L"Invalid protocol"));
	}
};

server::server() : impl_(new impl()){}

const std::vector<spl::shared_ptr<video_channel>> server::get_channels() const
{
	return impl_->channels_;
}
void server::subscribe(const monitor::observable::observer_ptr& o){impl_->event_subject_->subscribe(o);}
void server::unsubscribe(const monitor::observable::observer_ptr& o){impl_->event_subject_->unsubscribe(o);}

}