#include "StdAfx.h"

#include "server.h"

#include "consumer/oal/oal_frame_consumer.h"
#ifndef DISABLE_BLUEFISH
#include "consumer/bluefish/BlueFishVideoConsumer.h"
#endif
#include "consumer/decklink/DecklinkVideoConsumer.h"
#include "consumer/ogl/ogl_frame_consumer.h"
#include "consumer/file/file_frame_consumer.h"

#include <FreeImage.h>

#include "producer/flash/FlashAxContainer.h"

#include "../common/utility/string_convert.h"

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar{

struct server::implementation : boost::noncopyable
{
	implementation()												
	{
		FreeImage_Initialise(true);	
		av_register_all();
				
		boost::property_tree::ptree pt;
		boost::property_tree::read_xml(boost::filesystem::initial_path().file_string() + "\\caspar.config", pt);

		setup_paths();
		setup_channels(pt);
	
		if(!flash::FlashAxContainer::CheckForFlashSupport())
			CASPAR_LOG(error) << "No flashplayer activex-control installed. Flash support will be disabled";
	}

	~implementation()
	{		
		FreeImage_DeInitialise();
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
					frame_consumer_ptr consumer;
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
						consumer = std::make_shared<ogl::ogl_frame_consumer>(format_desc, device, stretch, windowed);
					}
				#ifndef DISABLE_BLUEFISH
					else if(name == "bluefish")
						consumer = caspar::bluefish::BlueFishVideoConsumer::Create(format_desc, xml_consumer.second.get("device", 0));
				#endif
					else if(name == "decklink")
						consumer = std::make_shared<decklink::DecklinkVideoConsumer>(format_desc, xml_consumer.second.get("internalkey", false));
					else if(name == "audio")
						consumer = std::make_shared<audio::oal_frame_consumer>(format_desc);

					if(consumer)					
						consumers.push_back(consumer);					
				}
				catch(...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
				}
			}
			//consumers.push_back(std::make_shared<file::file_frame_consumer>(common::narrow(media_folder_) + "test.avi", format_desc));
			channels_.push_back(std::make_shared<renderer::render_device>(format_desc, channels_.size() + 1, consumers));
		}
	}
		
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