#include "../../StdAfx.h"

#include "channel_command.h"

#include "exception.h"

#include "../../producer/media.h"

#include "../../frame/frame_format.h"

#include "../../renderer/render_device.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range.hpp>

namespace caspar { namespace controller { namespace amcp {

channel_info channel_info::parse(boost::wsmatch& what, const std::vector<renderer::render_device_ptr>& channels)
{
	channel_info info;
				
	int channel_index = boost::lexical_cast<int>(what["CHANNEL"].str())-1;
	if(channel_index < 0 || static_cast<size_t>(channel_index) >= channels.size())
		BOOST_THROW_EXCEPTION(invalid_channel() << arg_value_info(common::narrow(what["CHANNEL"].str())));

	info.channel_index	= channel_index;
	info.channel		= channels[info.channel_index];
	info.layer_index	= what["LAYER"].matched ? boost::lexical_cast<int>(what["LAYER"].str()) : DEFAULT_CHANNEL_LAYER;

	return info;
}

std::function<std::wstring()> channel_info_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^INFO");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;

	auto info = channel_info::parse(what, channels);
	
	std::wstring index_str = boost::lexical_cast<std::wstring>(info.channel_index);
	if(what["LAYER"].matched)
		index_str += L"-" + boost::lexical_cast<std::wstring>(info.layer_index);

	return [=]() -> std::wstring
	{
		std::wstringstream str;
		str << index_str << TEXT(" ") << info.channel->frame_format_desc().name  << TEXT("\r\n") << (info.channel->active(info.layer_index) != nullptr ? TEXT(" PLAYING") : TEXT(" STOPPED"));;	
		
		CASPAR_LOG(info) << L"Executed [amcp_channel_info]";		
		return str.str();
	};
}

std::function<std::wstring()> channel_load_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^LOAD(?<NOPREVIEW>BG)?\\s(?<CHANNEL>\\d+)-?(?<LAYER>\\d+)?\\s?(?<PLAY_ON_LOAD>\\d\\s)?(?<PARAMS>.*)");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;
		
	auto info = channel_info::parse(what, channels);
	
	std::vector<std::wstring> params;
	boost::split(params, what["PARAMS"].str(), boost::is_any_of(" "));

	auto load_option = renderer::load_option::none;
	if(!what["NOPREVIEW"].matched)
		load_option = renderer::load_option::preview;
	else if(what["PLAY_ON_LOAD"].matched && what["PLAY_ON_LOAD"].str() != L"0")
		load_option = renderer::load_option::auto_play;

	for(size_t n = 0; n < params.size(); ++n)
		boost::replace_all(params[n], "\"", "");
	
	return [=]() -> std::wstring
	{
		if(auto producer = load_media(params, info.channel->frame_format_desc()))
			info.channel->load(info.layer_index, producer, load_option);	
		else
			BOOST_THROW_EXCEPTION(file_not_found());

		CASPAR_LOG(info) << L"Executed [amcp_channel_load]";
		return L"";
	};
}

std::function<std::wstring()> channel_play_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^PLAY\\s(?<CHANNEL>\\d+)-?(?<LAYER>\\d+)?");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;

	auto info = channel_info::parse(what, channels);

	return [=]() -> std::wstring
	{
		info.channel->play(info.layer_index);		
		CASPAR_LOG(info) << L"Executed [amcp_channel_play]";
		return L"";
	};
}

std::function<std::wstring()> channel_stop_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^STOP\\s(?<CHANNEL>\\d+)-?(?<LAYER>\\d+)?");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;

	auto info = channel_info::parse(what, channels);	

	return [=]() -> std::wstring
	{
		info.channel->stop(info.layer_index);
		CASPAR_LOG(info) << L"Executed [amcp_channel_stop]";
		return L"";
	};
}

std::function<std::wstring()> channel_clear_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^CLEAR\\s(?<CHANNEL>\\d+)-?(?<LAYER>\\d+)?");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;
	
	auto info = channel_info::parse(what, channels);

	return [=]() -> std::wstring
	{
		what["LAYER"].matched ? info.channel->clear(info.layer_index) : info.channel->clear();
		CASPAR_LOG(info) << L"Executed [amcp_channel_clear]";
		return L"";	
	};
}

}}}
