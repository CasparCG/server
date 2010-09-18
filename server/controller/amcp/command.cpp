#include "../../StdAfx.h"

#include "command.h"
#include "exception.h"

#include "../../producer/media.h"

#include "../../renderer/render_device.h"
#include "../../frame/frame_format.h"

#include "../../server.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range.hpp>
#include <boost/date_time.hpp>

namespace caspar { namespace controller { namespace amcp {
		
std::wstring list_media()
{	
	std::wstringstream replyString;
	for (boost::filesystem::wrecursive_directory_iterator itr(server::media_folder()), end; itr != end; ++itr)
    {			
		if(boost::filesystem::is_regular_file(itr->path()))
		{
			std::wstring clipttype = TEXT(" N/A ");
			std::wstring extension = boost::to_upper_copy(itr->path().extension());
			if(extension == TEXT(".TGA") || extension == TEXT(".COL"))
				clipttype = TEXT(" STILL ");
			else if(extension == TEXT(".SWF") || extension == TEXT(".DV") || extension == TEXT(".MOV") || extension == TEXT(".MPG") || 
					extension == TEXT(".AVI") || extension == TEXT(".FLV") || extension == TEXT(".F4V") || extension == TEXT(".MP4"))
				clipttype = TEXT(" MOVIE ");
			else if(extension == TEXT(".WAV") || extension == TEXT(".MP3"))
				clipttype = TEXT(" STILL ");

			if(clipttype != TEXT(" N/A "))
			{		
				auto is_not_digit = [](char c){ return std::isdigit(c) == 0; };

				auto relativePath = boost::filesystem::wpath(itr->path().file_string().substr(server::media_folder().size(), itr->path().file_string().size()));

				auto writeTimeStr = boost::posix_time::to_iso_string(boost::posix_time::from_time_t(boost::filesystem::last_write_time(itr->path())));
				writeTimeStr.erase(std::remove_if(writeTimeStr.begin(), writeTimeStr.end(), is_not_digit), writeTimeStr.end());
				auto writeTimeWStr = std::wstring(writeTimeStr.begin(), writeTimeStr.end());

				auto sizeStr = boost::lexical_cast<std::wstring>(boost::filesystem::file_size(itr->path()));
				sizeStr.erase(std::remove_if(sizeStr.begin(), sizeStr.end(), is_not_digit), sizeStr.end());
				auto sizeWStr = std::wstring(sizeStr.begin(), sizeStr.end());

				replyString << TEXT("\"") << relativePath.replace_extension(TEXT(""))
							<< TEXT("\" ") << clipttype 
							<< TEXT(" ") << sizeStr
							<< TEXT(" ") << writeTimeWStr
							<< TEXT("\r\n");
			}	
		}
	}
	return boost::to_upper_copy(replyString.str());
}

std::wstring list_templates() 
{
	std::wstringstream replyString;

	for (boost::filesystem::wrecursive_directory_iterator itr(server::template_folder()), end; itr != end; ++itr)
    {		
		if(boost::filesystem::is_regular_file(itr->path()) && itr->path().extension() == L".ft")
		{
			auto relativePath = boost::filesystem::wpath(itr->path().file_string().substr(server::template_folder().size(), itr->path().file_string().size()));

			auto writeTimeStr = boost::posix_time::to_iso_string(boost::posix_time::from_time_t(boost::filesystem::last_write_time(itr->path())));
			writeTimeStr.erase(std::remove_if(writeTimeStr.begin(), writeTimeStr.end(), [](char c){ return std::isdigit(c) == 0;}), writeTimeStr.end());
			auto writeTimeWStr = std::wstring(writeTimeStr.begin(), writeTimeStr.end());

			auto sizeStr = boost::lexical_cast<std::string>(boost::filesystem::file_size(itr->path()));
			sizeStr.erase(std::remove_if(sizeStr.begin(), sizeStr.end(), [](char c){ return std::isdigit(c) == 0;}), sizeStr.end());

			auto sizeWStr = std::wstring(sizeStr.begin(), sizeStr.end());
			
			replyString << TEXT("\"") << relativePath.replace_extension(TEXT(""))
						<< TEXT("\" ") << sizeWStr
						<< TEXT(" ") << writeTimeWStr
						<< TEXT("\r\n");		
		}
	}
	return boost::to_upper_copy(replyString.str());
}

unsigned int get_channel_index(const std::wstring& index_str, const std::vector<renderer::render_device_ptr>& channels)
{	
	int channel_index = boost::lexical_cast<int>(index_str)-1;
	if(channel_index < 0 || static_cast<size_t>(channel_index) >= channels.size())
		BOOST_THROW_EXCEPTION(invalid_channel() << arg_value_info(common::narrow(index_str)));

	return static_cast<unsigned int>(channel_index);
}

std::function<std::wstring()> channel_info_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"\\bINFO\\b\\s?(?<CHANNEL>\\d)?-?(?<LAYER>\\d)?\\s*");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;

	if(!what["CHANNEL"].matched)
		BOOST_THROW_EXCEPTION(missing_parameter() << arg_name_info("CHANNEL"));
	
	int  channel_index	= get_channel_index(what["CHANNEL"].str(), channels);
	auto channel		= channels[channel_index];
	int	 layer_index	= what["LAYER"].matched ? boost::lexical_cast<int>(what["LAYER"].str()) : DEFAULT_CHANNEL_LAYER;

	std::wstring index_str = boost::lexical_cast<std::wstring>(channel_index);
	if(what["LAYER"].matched)
		index_str += L"-" + boost::lexical_cast<std::wstring>(layer_index);

	return [=]() -> std::wstring
	{
		std::wstringstream str;
		str << index_str << TEXT(" ") << channel->frame_format_desc().name  << TEXT("\r\n") << (channel->active(layer_index) != nullptr ? TEXT(" PLAYING") : TEXT(" STOPPED"));;	
		
		CASPAR_LOG(info) << L"Executed [amcp_info]";
		
		return str.str();
	};
}

std::function<std::wstring()> load_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"\\bLOAD(?<NOPREVIEW>BG)?\\b\\s?(?<CHANNEL>\\d)?-?(?<LAYER>\\d)?\\s(?<PARAMS>.*)");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;

	if(!what["CHANNEL"].matched)
		BOOST_THROW_EXCEPTION(missing_parameter() << arg_name_info("CHANNEL"));

	if(!what["PARAMS"].matched)
		BOOST_THROW_EXCEPTION(missing_parameter() << arg_name_info("FILENAME"));
			
	int  channel_index	= get_channel_index(what["CHANNEL"].str(), channels);
	auto channel		= channels[channel_index];
	int	 layer_index	= what["LAYER"].matched ? boost::lexical_cast<int>(what["LAYER"].str()) : DEFAULT_CHANNEL_LAYER;
	
	bool preview		= !what["NOPREVIEW"].matched;
	std::vector<std::wstring> params;
	boost::split(params, what["PARAMS"].str(), boost::is_any_of(" "));

	for(size_t n = 0; n < params.size(); ++n)
		boost::replace_all(params[n], "\"", "");
	
	return [=]() -> std::wstring
	{
		if(auto producer = load_media(params, channel->frame_format_desc()))
			channel->load(layer_index, producer, preview ? renderer::load_option::preview : renderer::load_option::auto_play);	
		else
			BOOST_THROW_EXCEPTION(file_not_found());
		
		CASPAR_LOG(info) << L"Executed [amcp_load]";

		return L"";
	};
}

std::function<std::wstring()> play_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"\\bPLAY\\b\\s?(?<CHANNEL>\\d)?-?(?<LAYER>\\d)?\\s*");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;

	if(!what["CHANNEL"].matched)
		BOOST_THROW_EXCEPTION(missing_parameter() << arg_name_info("CHANNEL"));
	
	int  channel_index	= get_channel_index(what["CHANNEL"].str(), channels);
	auto channel		= channels[channel_index];
	int	 layer_index	= what["LAYER"].matched ? boost::lexical_cast<int>(what["LAYER"].str()) : DEFAULT_CHANNEL_LAYER;

	return [=]() -> std::wstring
	{
		channel->play(layer_index);
		
		CASPAR_LOG(info) << L"Executed [amcp_play]";

		return L"";
	};
}

std::function<std::wstring()> stop_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"\\bSTOP\\b\\s?(?<CHANNEL>\\d)?-?(?<LAYER>\\d)?\\s*");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;

	if(!what["CHANNEL"].matched)
		BOOST_THROW_EXCEPTION(missing_parameter() << arg_name_info("CHANNEL"));
	
	int  channel_index	= get_channel_index(what["CHANNEL"].str(), channels);
	auto channel		= channels[channel_index];
	int	 layer_index	= what["LAYER"].matched ? boost::lexical_cast<int>(what["LAYER"].str()) : DEFAULT_CHANNEL_LAYER;
	
	return [=]() -> std::wstring
	{
		channel->stop(layer_index);

		CASPAR_LOG(info) << L"Executed [amcp_stop]";

		return L"";
	};
}

std::function<std::wstring()> clear_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"\\bCLEAR\\b\\s?(?<CHANNEL>\\d)?-?(?<LAYER>\\d)?\\s*");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;

	if(!what["CHANNEL"].matched)
		BOOST_THROW_EXCEPTION(missing_parameter() << arg_name_info("CHANNEL"));
	
	int  channel_index	= get_channel_index(what["CHANNEL"].str(), channels);
	auto channel		= channels[channel_index];
	int	 layer_index	= what["LAYER"].matched ? boost::lexical_cast<int>(what["LAYER"].str()) : -1;

	return [=]() -> std::wstring
	{
		layer_index != -1 ? channel->clear(layer_index) : channel->clear();

		CASPAR_LOG(info) << L"Executed [amcp_clear]";

		return L"";	
	};
}

std::function<std::wstring()> list_media_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"\\bCLS\\s*");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;
	
	return [=]() -> std::wstring
	{
		CASPAR_LOG(info) << L"Executed [amcp_list_media]";

		return list_media();	
	};
}

std::function<std::wstring()> list_template_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"\\bTLS\\s*");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;
	
	return [=]() -> std::wstring
	{
		CASPAR_LOG(info) << L"Executed [amcp_list_template]";

		return list_templates();	
	};
}

std::function<std::wstring()> media_info_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"\\bCINF\\s*");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;
	
	BOOST_THROW_EXCEPTION(not_implemented());
}

std::function<std::wstring()> template_info_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"\\bTINF\\s*");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;
	
	BOOST_THROW_EXCEPTION(not_implemented());
}

std::function<std::wstring()> version_info_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"\\bVERSION\\s*");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;
	
	return [=]() -> std::wstring
	{
		CASPAR_LOG(info) << L"Executed [amcp_version_info]";

		return std::wstring(TEXT(CASPAR_VERSION_STR)) + TEXT("\r\n");	
	};	
}

}}}
