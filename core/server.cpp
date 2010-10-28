#include "StdAfx.h"

#include "server.h"

#include "consumer/oal/oal_consumer.h"
#ifndef DISABLE_BLUEFISH
#include "consumer/bluefish/bluefish_consumer.h"
#endif
#include "consumer/decklink/DecklinkVideoConsumer.h"
#include "consumer/ogl/ogl_consumer.h"

#include <FreeImage.h>

#include "protocol/amcp/AMCPProtocolStrategy.h"
#include "protocol/cii/CIIProtocolStrategy.h"
#include "protocol/CLK/CLKProtocolStrategy.h"
#include "producer/flash/FlashAxContainer.h"

#include "../common/io/AsyncEventServer.h"
#include "../common/utility/string_convert.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace caspar{

struct server::implementation : boost::noncopyable
{
	implementation()												
	{
		FreeImage_Initialise(true);	
				
		boost::property_tree::ptree pt;
		boost::property_tree::read_xml(boost::filesystem::initial_path().file_string() + "\\caspar.config", pt);

		setup_paths();
		setup_channels(pt);
		setup_controllers(pt);
	
		//if(!flash::FlashAxContainer::CheckForFlashSupport())
		//	CASPAR_LOG(error) << "No flashplayer activex-control installed. Flash support will be disabled";
	}

	~implementation()
	{		
		FreeImage_DeInitialise();
		async_servers_.clear();
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
		media_folder_ = common::widen(paths.get("media-path", initialPath + "\\media\\"));
		log_folder_ = common::widen(paths.get("log-path", initialPath + "\\log\\"));
		template_folder_ = common::widen(paths.get("template-path", initialPath + "\\template\\"));
		data_folder_ = common::widen(paths.get("data-path", initialPath + "\\data\\"));
	}
			
	void setup_channels(boost::property_tree::ptree& pt)
	{   
		using boost::property_tree::ptree;
		BOOST_FOREACH(auto& xml_channel, pt.get_child("configuration.channels"))
		{		
			auto format_desc = get_video_format_desc(common::widen(xml_channel.second.get("videomode", "PAL")));			
			std::vector<frame_consumer_ptr> consumers;

			BOOST_FOREACH(auto& xml_consumer, xml_channel.second.get_child("consumers"))
			{
				try
				{
					frame_consumer_ptr pConsumer;
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
						pConsumer = std::make_shared<ogl::consumer>(format_desc, device, stretch, windowed);
					}
				#ifndef DISABLE_BLUEFISH
					else if(name == "bluefish")					
						pConsumer = std::make_shared<bluefish::consumer>(format_desc, xml_consumer.second.get("device", 0), xml_consumer.second.get("embedded-audio", false));					
				#endif
					else if(name == "decklink")
						pConsumer = std::make_shared<decklink::DecklinkVideoConsumer>(format_desc, xml_consumer.second.get("internalkey", false));
					else if(name == "audio")
						pConsumer = std::make_shared<audio::consumer>(format_desc);

					if(pConsumer)					
						consumers.push_back(pConsumer);					
				}
				catch(...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
				}
			}
			
			channels_.push_back(std::make_shared<renderer::render_device>(format_desc, channels_.size() + 1, consumers));
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
					auto asyncserver = std::make_shared<caspar::IO::AsyncEventServer>(create_protocol(protocol), port);
					asyncserver->Start();
					async_servers_.push_back(asyncserver);
				}
				else
					BOOST_THROW_EXCEPTION(invalid_configuration() << arg_name_info(name) << msg_info("Invalid controller"));
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				throw;
			}
		}
	}

	IO::ProtocolStrategyPtr create_protocol(const std::string& name) const
	{
		if(name == "AMCP")
			return std::make_shared<amcp::AMCPProtocolStrategy>(channels_);
		else if(name == "CII")
			return std::make_shared<cii::CIIProtocolStrategy>(channels_);
		else if(name == "CLOCK")
			return std::make_shared<CLK::CLKProtocolStrategy>(channels_);
		
		BOOST_THROW_EXCEPTION(invalid_configuration() << arg_name_info("name") << arg_value_info(name) << msg_info("Invalid protocol"));
	}

	std::vector<IO::AsyncEventServerPtr> async_servers_;

	std::vector<renderer::render_device_ptr> channels_;

	int logLevel_;

	static std::wstring media_folder_;
	static std::wstring log_folder_;
	static std::wstring template_folder_;
	static std::wstring data_folder_;
};

std::wstring server::implementation::media_folder_ = L"";
std::wstring server::implementation::log_folder_ = L"";
std::wstring server::implementation::template_folder_ = L"";
std::wstring server::implementation::data_folder_ = L"";

server::server() : impl_(new implementation()){}

const std::wstring& server::media_folder()
{
	server::implementation::setup_paths();
	return server::implementation::media_folder_;
}

const std::wstring& server::log_folder()
{
	server::implementation::setup_paths();
	return server::implementation::log_folder_;
}

const std::wstring& server::template_folder()
{
	server::implementation::setup_paths();
	return server::implementation::template_folder_;
}

const std::wstring& server::data_folder()
{
	server::implementation::setup_paths();
	return server::implementation::data_folder_;
}

const std::vector<renderer::render_device_ptr>& server::get_channels() const{ return impl_->channels_; }

}