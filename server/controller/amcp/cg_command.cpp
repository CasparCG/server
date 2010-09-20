#include "../../StdAfx.h"

#include "cg_command.h"
#include "channel_command.h"

#include "exception.h"

#include "../../renderer/render_device.h"

#include "../../producer/flash/cg_producer.h"
#include "../../producer/flash/flash_producer.h"

#include "../../server.h"

#include "../../../common/utility/scope_exit.h"

#include <boost/algorithm/string.hpp>

namespace caspar { namespace controller { namespace amcp {
	
std::wstring get_data(const std::wstring& data)
{
	if(data.empty())
		return L"";

	if(data[0] == L'<')
		return data;

	//The data is not an XML-string, it must be a filename	
	std::wstring filename = server::data_folder() + data + L".ftd";

	std::wifstream datafile(filename.c_str());
	if(datafile) 
	{ 
		CASPAR_SCOPE_EXIT([&]{datafile.close();});
		
		std::wstringstream file_data;
		file_data << datafile.rdbuf();			

		auto result = file_data.str();
		if(result.empty() || result[0] != L'<')
			return result;
	}		
	
	CASPAR_LOG(warning) << "[cg_command] Invalid Data";
	return L"";
}

std::function<std::wstring()> channel_cg_add_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^CG\\s(?<CHANNEL>\\d+)-?(?<LAYER>\\d+)?\\sADD\\s(?<FLASH_LAYER>\\d+)\\s(?<TEMPLATE>\\S+)\\s?(?<START_LABEL>\\S\\S+)?\\s?(?<PLAY_ON_LOAD>\\d)?\\s?(?<DATA>.*)?");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;

	auto info = channel_info::parse(what, channels);

	int flash_layer_index = boost::lexical_cast<int>(what["FLASH_LAYER"].str());

	std::wstring templatename = what["TEMPLATE"].str();
	bool play_on_load = what["PLAY_ON_LOAD"].matched ? what["PLAY_ON_LOAD"].str() != L"0" : 0;
	std::wstring start_label = what["START_LABEL"].str();	
	std::wstring data = get_data(what["DATA"].str());
	
	boost::replace_all(templatename, "\"", "");

	return [=]() -> std::wstring
	{	
		std::wstring fullFilename = flash::flash_producer::find_template(server::template_folder() + templatename);
		if(fullFilename.empty())
			BOOST_THROW_EXCEPTION(file_not_found());
	
		std::wstring extension = boost::filesystem::wpath(fullFilename).extension();
		std::wstring filename = templatename;
		filename.append(extension);

		flash::get_default_cg_producer(info.channel, std::max<int>(DEFAULT_CHANNEL_LAYER+1, info.layer_index))
			->add(flash_layer_index, filename, play_on_load, start_label, data);

		CASPAR_LOG(info) << L"Executed [amcp_channel_cg_add]";
		return L"";
	};
}

std::function<std::wstring()> channel_cg_remove_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^CG\\s(?<CHANNEL>\\d+)-?(?<LAYER>\\d+)?\\sREMOVE\\s(?<FLASH_LAYER>\\d+)");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;

	auto info = channel_info::parse(what, channels);

	int flash_layer_index = boost::lexical_cast<int>(what["FLASH_LAYER"].str());
	
	return [=]() -> std::wstring
	{	
		flash::get_default_cg_producer(info.channel, std::max<int>(DEFAULT_CHANNEL_LAYER+1, info.layer_index))
			->remove(flash_layer_index);

		CASPAR_LOG(info) << L"Executed [amcp_channel_cg_remove]";
		return L"";
	};
}

std::function<std::wstring()> channel_cg_clear_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^CG\\s(?<CHANNEL>\\d+)-?(?<LAYER>\\d+)?\\sCLEAR");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;

	auto info = channel_info::parse(what, channels);
	
	return [=]() -> std::wstring
	{	
		flash::get_default_cg_producer(info.channel, std::max<int>(DEFAULT_CHANNEL_LAYER+1, info.layer_index))
			->clear();

		CASPAR_LOG(info) << L"Executed [amcp_channel_cg_clear]";
		return L"";
	};
}

std::function<std::wstring()> channel_cg_play_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^CG\\s(?<CHANNEL>\\d+)-?(?<LAYER>\\d+)?\\sPLAY\\s(?<FLASH_LAYER>\\d+)");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;

	auto info = channel_info::parse(what, channels);

	int flash_layer_index = boost::lexical_cast<int>(what["FLASH_LAYER"].str());
	
	return [=]() -> std::wstring
	{	
		flash::get_default_cg_producer(info.channel, std::max<int>(DEFAULT_CHANNEL_LAYER+1, info.layer_index))
			->play(flash_layer_index);

		CASPAR_LOG(info) << L"Executed [amcp_channel_cg_play]";
		return L"";
	};
}

std::function<std::wstring()> channel_cg_stop_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^CG\\s(?<CHANNEL>\\d+)-?(?<LAYER>\\d+)?\\sSTOP\\s(?<FLASH_LAYER>\\d+)");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;

	auto info = channel_info::parse(what, channels);

	int flash_layer_index = boost::lexical_cast<int>(what["FLASH_LAYER"].str());
	
	return [=]() -> std::wstring
	{	
		flash::get_default_cg_producer(info.channel, std::max<int>(DEFAULT_CHANNEL_LAYER+1, info.layer_index))
			->stop(flash_layer_index, 0);

		CASPAR_LOG(info) << L"Executed [amcp_channel_cg_stop]";
		return L"";
	};
}

std::function<std::wstring()> channel_cg_next_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^CG\\s(?<CHANNEL>\\d+)-?(?<LAYER>\\d+)?\\sNEXT\\s(?<FLASH_LAYER>\\d+)");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;

	auto info = channel_info::parse(what, channels);

	int flash_layer_index = boost::lexical_cast<int>(what["FLASH_LAYER"].str());
	
	return [=]() -> std::wstring
	{	
		flash::get_default_cg_producer(info.channel, std::max<int>(DEFAULT_CHANNEL_LAYER+1, info.layer_index))
			->next(flash_layer_index);

		CASPAR_LOG(info) << L"Executed [amcp_channel_cg_next]";
		return L"";
	};
}

std::function<std::wstring()> channel_cg_goto_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^CG\\s(?<CHANNEL>\\d+)-?(?<LAYER>\\d+)?\\sREMOVE\\s(?<FLASH_LAYER>\\d+)\\s(?<LABEL>[^\\s]+)");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;

	auto info = channel_info::parse(what, channels);

	int flash_layer_index = boost::lexical_cast<int>(what["FLASH_LAYER"].str());
	
	BOOST_THROW_EXCEPTION(not_implemented());
}

std::function<std::wstring()> channel_cg_update_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^CG\\s(?<CHANNEL>\\d+)-?(?<LAYER>\\d+)?\\sUPDATE\\s(?<FLASH_LAYER>\\d+)\\s(?<DATA>[^\\s]+)");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;

	auto info = channel_info::parse(what, channels);

	int flash_layer_index = boost::lexical_cast<int>(what["FLASH_LAYER"].str());
	std::wstring data = get_data(what["DATA"].str());
	
	return [=]() -> std::wstring
	{	
		flash::get_default_cg_producer(info.channel, std::max<int>(DEFAULT_CHANNEL_LAYER+1, info.layer_index))
			->update(flash_layer_index, data);

		CASPAR_LOG(info) << L"Executed [amcp_channel_cg_update]";
		return L"";
	};
}

std::function<std::wstring()> channel_cg_invoke_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^CG\\s(?<CHANNEL>\\d+)-?(?<LAYER>\\d+)?\\sUPDATE\\s(?<FLASH_LAYER>\\d+)\\s(?<METHOD>[^\\s]+)");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;

	auto info = channel_info::parse(what, channels);

	int flash_layer_index = boost::lexical_cast<int>(what["FLASH_LAYER"].str());
	std::wstring method = get_data(what["METHOD"].str());
	
	return [=]() -> std::wstring
	{	
		flash::get_default_cg_producer(info.channel, std::max<int>(DEFAULT_CHANNEL_LAYER+1, info.layer_index))
			->invoke(flash_layer_index, method);

		CASPAR_LOG(info) << L"Executed [amcp_channel_cg_invoke]";
		return L"";
	};
}

}}}
