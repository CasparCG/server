#include "StdAfx.h"

#include "configuration.h"
#include "channel.h"

#include "consumer/oal/oal_consumer.h"
#ifndef DISABLE_BLUEFISH
#include "consumer/bluefish/bluefish_consumer.h"
#endif
#include "consumer/decklink/decklink_consumer.h"
#include "consumer/ogl/ogl_consumer.h"

#include "protocol/amcp/AMCPProtocolStrategy.h"
#include "protocol/cii/CIIProtocolStrategy.h"
#include "protocol/CLK/CLKProtocolStrategy.h"
#include "producer/flash/FlashAxContainer.h"

#include "../common/exception/exceptions.h"
#include "../common/io/AsyncEventServer.h"
#include "../common/utility/string_convert.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "consumer/frame_consumer_device.h"
#include "processor/frame_processor_device.h"

namespace caspar { namespace core {

struct configuration::implementation : boost::noncopyable
{
	implementation()												
	{			
		boost::property_tree::ptree pt;
		boost::property_tree::read_xml(boost::filesystem::initial_path().file_string() + "\\caspar.config", pt);

		setup_paths();
		setup_channels(pt);
		setup_controllers(pt);
	
		if(!flash::FlashAxContainer::CheckForFlashSupport())
			CASPAR_LOG(error) << "No flashplayer activex-control installed. Flash support will be disabled";
	}

	~implementation()
	{				
		async_configurations_.clear();
		channels_.clear();
	}

	static void setup_paths()
	{
		if(!media_folder_.empty())
			return;

		std::string initialPath = boost::filesystem::initial_path().file_string();
		boost::property_tree::ptree pt;
		boost::property_tree::read_xml(initialPath + "\\caspar.config", pt);

		auto paths = pt.get_child("configuration.paths");
		media_folder_ = widen(paths.get("media-path", initialPath + "\\media\\"));
		log_folder_ = widen(paths.get("log-path", initialPath + "\\log\\"));
		template_folder_ = widen(paths.get("template-path", initialPath + "\\template\\"));
		data_folder_ = widen(paths.get("data-path", initialPath + "\\data\\"));
	}
			
	void setup_channels(boost::property_tree::ptree& pt)
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
				#ifndef DISABLE_BLUEFISH
					else if(name == "bluefish")					
						consumers.push_back(bluefish::consumer(format_desc, xml_consumer.second.get("device", 0), xml_consumer.second.get("embedded-audio", false)));					
				#endif
					else if(name == "decklink")
						consumers.push_back(make_safe<decklink::decklink_consumer>(format_desc, xml_consumer.second.get("internalkey", false)));
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
		
	void setup_controllers(boost::property_tree::ptree& pt)
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
					auto asyncconfiguration = make_safe<IO::AsyncEventServer>(create_protocol(protocol), port);
					asyncconfiguration->Start();
					async_configurations_.push_back(asyncconfiguration);
				}
				else
					BOOST_THROW_EXCEPTION(invalid_configuration() << arg_name_info(name) << msg_info("Invalid controller"));
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
		
		BOOST_THROW_EXCEPTION(invalid_configuration() << arg_name_info("name") << arg_value_info(name) << msg_info("Invalid protocol"));
	}

	std::vector<safe_ptr<IO::AsyncEventServer>> async_configurations_;
	
	std::vector<safe_ptr<channel>> channels_;

	int logLevel_;

	static std::wstring media_folder_;
	static std::wstring log_folder_;
	static std::wstring template_folder_;
	static std::wstring data_folder_;
};

std::wstring configuration::implementation::media_folder_ = L"";
std::wstring configuration::implementation::log_folder_ = L"";
std::wstring configuration::implementation::template_folder_ = L"";
std::wstring configuration::implementation::data_folder_ = L"";

configuration::configuration() : impl_(new implementation()){}

const std::wstring& configuration::media_folder()
{
	configuration::implementation::setup_paths();
	return configuration::implementation::media_folder_;
}

const std::wstring& configuration::log_folder()
{
	configuration::implementation::setup_paths();
	return configuration::implementation::log_folder_;
}

const std::wstring& configuration::template_folder()
{
	configuration::implementation::setup_paths();
	return configuration::implementation::template_folder_;
}

const std::wstring& configuration::data_folder()
{
	configuration::implementation::setup_paths();
	return configuration::implementation::data_folder_;
}

const std::vector<safe_ptr<channel>> configuration::get_channels() const
{
	return impl_->channels_;
}
}}