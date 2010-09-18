#include "../../StdAfx.h"

#include "command.h"
#include "exception.h"

#include "../../producer/media.h"

#include "../../renderer/render_device.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range.hpp>

namespace caspar { namespace controller { namespace amcp {
			
unsigned int get_channel_index(const std::wstring& index_str, const std::vector<renderer::render_device_ptr>& channels)
{	
	int channel_index = boost::lexical_cast<int>(index_str)-1;
	if(channel_index < 0 || static_cast<size_t>(channel_index) >= channels.size())
		BOOST_THROW_EXCEPTION(invalid_channel() << arg_value_info(common::narrow(index_str)));

	return static_cast<unsigned int>(channel_index);
}

std::function<std::wstring()> channel_info_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	return nullptr;
}

std::function<std::wstring()> load_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"\\bLOAD(?<NOPREVIEW>BG)?\\s(?<CHANNEL>\\d)?-?(?<LAYER>\\d)?\\s(?<PARAMS>.*)");

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
	static boost::wregex expr(L"\\bPLAY\\s(?<CHANNEL>\\d)?-?(?<LAYER>\\d)?\\s*");

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
	static boost::wregex expr(L"\\bSTOP\\s(?<CHANNEL>\\d)?-?(?<LAYER>\\d)?\\s*");

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
	static boost::wregex expr(L"\\bCLEAR\\s(?<CHANNEL>\\d)?-?(?<LAYER>\\d)?\\s*");

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
	return nullptr;
}

std::function<std::wstring()> list_template_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	return nullptr;
}

std::function<std::wstring()> media_info_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	return nullptr;
}

std::function<std::wstring()> template_info_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	return nullptr;
}

std::function<std::wstring()> version_info_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	return nullptr;
}

}}}
