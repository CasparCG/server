#include "bootstrapper.h"

#include <core/channel.h>

#include <core/consumer/oal/oal_consumer.h>
#include <core/consumer/bluefish/bluefish_consumer.h>
#include <core/consumer/decklink/decklink_consumer.h>
#include <core/consumer/ogl/ogl_consumer.h>
#include <core/consumer/ffmpeg/ffmpeg_consumer.h>

#include <core/producer/flash/FlashAxContainer.h>

#include <protocol/amcp/AMCPProtocolStrategy.h>
#include <protocol/cii/CIIProtocolStrategy.h>
#include <protocol/CLK/CLKProtocolStrategy.h>
#include <protocol/util/AsyncEventServer.h>

#include <common/env.h>
#include <common/exception/exceptions.h>
#include <common/utility/string_convert.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace caspar {

using namespace core;
using namespace protocol;

struct bootstrapper::implementation : boost::noncopyable
{
	std::vector<safe_ptr<IO::AsyncEventServer>> async_servers_;	
	std::vector<safe_ptr<channel>> channels_;

	implementation()												
	{			
		setup_channels(env::properties());
		setup_controllers(env::properties());
	
		if(!flash::FlashAxContainer::CheckForFlashSupport())
			CASPAR_LOG(error) << "No flashplayer activex-control installed. Flash support will be disabled";
	}

	~implementation()
	{				
		async_servers_.clear();
		channels_.clear();
	}
				
	void setup_channels(const boost::property_tree::ptree& pt)
	{   
		using boost::property_tree::ptree;
		BOOST_FOREACH(auto& xml_channel, pt.get_child("configuration.channels"))
		{		
			auto format_desc = video_format_desc::get(widen(xml_channel.second.get("videomode", "PAL")));		
			if(format_desc.format == video_format::invalid)
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Invalid videomode."));
			std::vector<safe_ptr<frame_consumer>> consumers;

			BOOST_FOREACH(auto& xml_consumer, xml_channel.second.get_child("consumers"))
			{
				try
				{
					std::string name = xml_consumer.first;
					if(name == "ogl")
					{			
						int device = xml_consumer.second.get("device", 0);
			
						ogl::stretch stretch = ogl::stretch::fill;
						std::string stretchStr = xml_consumer.second.get("stretch", "");
						if(stretchStr == "none")
							stretch = ogl::stretch::none;
						else if(stretchStr == "uniform")
							stretch = ogl::stretch::uniform;
						else if(stretchStr == "uniformtofill")
							stretch = ogl::stretch::uniform_to_fill;

						bool windowed = xml_consumer.second.get("windowed", false);
						consumers.push_back(ogl::consumer(format_desc, device, stretch, windowed));
					}
					else if(name == "bluefish")					
						consumers.push_back(bluefish::consumer(format_desc, xml_consumer.second.get("device", 0), xml_consumer.second.get("embedded-audio", false)));					
					else if(name == "decklink")
						consumers.push_back(make_safe<decklink::decklink_consumer>(format_desc, xml_consumer.second.get("device", 0), xml_consumer.second.get("internalkey", false)));
					else if(name == "audio")
						consumers.push_back(oal::consumer(format_desc));			
				}
				catch(...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
				}
			}
							
			channels_.push_back(channel(format_desc, consumers));
		}
	}
		
	void setup_controllers(const boost::property_tree::ptree& pt)
	{		
		using boost::property_tree::ptree;
		BOOST_FOREACH(auto& xml_controller, pt.get_child("configuration.controllers"))
		{
			try
			{
				std::string name = xml_controller.first;
				std::string protocol = xml_controller.second.get<std::string>("protocol");	

				if(name == "tcpcontroller")
				{					
					unsigned int port = xml_controller.second.get<unsigned int>("port");
					port = port != 0 ? port : 5250;
					auto asyncbootstrapper = make_safe<IO::AsyncEventServer>(create_protocol(protocol), port);
					asyncbootstrapper->Start();
					async_servers_.push_back(asyncbootstrapper);
				}
				else
					BOOST_THROW_EXCEPTION(invalid_bootstrapper() << arg_name_info(name) << msg_info("Invalid controller"));
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		}
	}

	safe_ptr<IO::IProtocolStrategy> create_protocol(const std::string& name) const
	{
		if(name == "AMCP")
			return make_safe<amcp::AMCPProtocolStrategy>(channels_);
		else if(name == "CII")
			return make_safe<cii::CIIProtocolStrategy>(channels_);
		else if(name == "CLOCK")
			return make_safe<CLK::CLKProtocolStrategy>(channels_);
		
		BOOST_THROW_EXCEPTION(invalid_bootstrapper() << arg_name_info("name") << arg_value_info(name) << msg_info("Invalid protocol"));
	}
};

bootstrapper::bootstrapper() : impl_(new implementation()){}

const std::vector<safe_ptr<channel>> bootstrapper::get_channels() const
{
	return impl_->channels_;
}

}