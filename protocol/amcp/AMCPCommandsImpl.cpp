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
* Author: Nicklas P Andersson
*/

#include "../StdAfx.h"

#if defined(_MSC_VER)
#pragma warning (push, 1) // TODO: Legacy code, just disable warnings
#endif

#include "AMCPCommandsImpl.h"
#include "AMCPProtocolStrategy.h"

#include <common/env.h>

#include <common/log.h>
#include <common/param.h>
#include <common/os/system_info.h>
#include <common/os/filesystem.h>
#include <common/base64.h>

#include <core/producer/frame_producer.h>
#include <core/video_format.h>
#include <core/producer/transition/transition_producer.h>
#include <core/frame/frame_transform.h>
#include <core/producer/stage.h>
#include <core/producer/layer.h>
#include <core/mixer/mixer.h>
#include <core/consumer/output.h>
#include <core/thumbnail_generator.h>
#include <core/producer/media_info/media_info.h>
#include <core/producer/media_info/media_info_repository.h>
#include <core/diagnostics/call_context.h>
#include <core/diagnostics/osd_graph.h>
#include <core/system_info_provider.h>

#include <modules/reroute/producer/reroute_producer.h>

#include <algorithm>
#include <locale>
#include <fstream>
#include <memory>
#include <cctype>
#include <future>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/regex.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/locale.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include <tbb/concurrent_unordered_map.h>

/* Return codes

102 [action]			Information that [action] has happened
101 [action]			Information that [action] has happened plus one row of data  

202 [command] OK		[command] has been executed
201 [command] OK		[command] has been executed, plus one row of data  
200 [command] OK		[command] has been executed, plus multiple lines of data. ends with an empty line

400 ERROR				the command could not be understood
401 [command] ERROR		invalid/missing channel
402 [command] ERROR		parameter missing
403 [command] ERROR		invalid parameter  
404 [command] ERROR		file not found

500 FAILED				internal error
501 [command] FAILED	internal error
502 [command] FAILED	could not read file
503 [command] FAILED	access denied

600 [command] FAILED	[command] not implemented
*/

namespace caspar { namespace protocol {

using namespace core;

std::wstring read_file_base64(const boost::filesystem::path& file)
{
	using namespace boost::archive::iterators;

	boost::filesystem::ifstream filestream(file, std::ios::binary);

	if (!filestream)
		return L"";

	auto length = boost::filesystem::file_size(file);
	std::vector<char> bytes;
	bytes.resize(length);
	filestream.read(bytes.data(), length);

	std::string result(to_base64(bytes.data(), length));
	return std::wstring(result.begin(), result.end());
}

std::wstring read_utf8_file(const boost::filesystem::path& file)
{
	std::wstringstream result;
	boost::filesystem::wifstream filestream(file);

	if (filestream) 
	{
		// Consume BOM first
		filestream.get();
		// read all data
		result << filestream.rdbuf();
	}

	return result.str();
}

std::wstring read_latin1_file(const boost::filesystem::path& file)
{
	boost::locale::generator gen;
	gen.locale_cache_enabled(true);
	gen.categories(boost::locale::codepage_facet);

	std::stringstream result_stream;
	boost::filesystem::ifstream filestream(file);
	filestream.imbue(gen("en_US.ISO8859-1"));

	if (filestream)
	{
		// read all data
		result_stream << filestream.rdbuf();
	}

	std::string result = result_stream.str();
	std::wstring widened_result;

	// The first 255 codepoints in unicode is the same as in latin1
	boost::copy(
		result | boost::adaptors::transformed(
				[](char c) { return static_cast<unsigned char>(c); }),
		std::back_inserter(widened_result));

	return widened_result;
}

std::wstring read_file(const boost::filesystem::path& file)
{
	static const uint8_t BOM[] = {0xef, 0xbb, 0xbf};

	if (!boost::filesystem::exists(file))
	{
		return L"";
	}

	if (boost::filesystem::file_size(file) >= 3)
	{
		boost::filesystem::ifstream bom_stream(file);

		char header[3];
		bom_stream.read(header, 3);
		bom_stream.close();

		if (std::memcmp(BOM, header, 3) == 0)
			return read_utf8_file(file);
	}

	return read_latin1_file(file);
}

std::wstring MediaInfo(const boost::filesystem::path& path, const spl::shared_ptr<media_info_repository>& media_info_repo)
{
	if (!boost::filesystem::is_regular_file(path))
		return L"";

	auto media_info = media_info_repo->get(path.wstring());

	if (!media_info)
		return L"";

	auto is_not_digit = [](char c){ return std::isdigit(c) == 0; };

	auto relativePath = boost::filesystem::path(path.wstring().substr(env::media_folder().size() - 1, path.wstring().size()));

	auto writeTimeStr = boost::posix_time::to_iso_string(boost::posix_time::from_time_t(boost::filesystem::last_write_time(path)));
	writeTimeStr.erase(std::remove_if(writeTimeStr.begin(), writeTimeStr.end(), is_not_digit), writeTimeStr.end());
	auto writeTimeWStr = std::wstring(writeTimeStr.begin(), writeTimeStr.end());

	auto sizeStr = boost::lexical_cast<std::wstring>(boost::filesystem::file_size(path));
	sizeStr.erase(std::remove_if(sizeStr.begin(), sizeStr.end(), is_not_digit), sizeStr.end());
	auto sizeWStr = std::wstring(sizeStr.begin(), sizeStr.end());

	auto str = relativePath.replace_extension(L"").generic_wstring();
	if (str[0] == '\\' || str[0] == '/')
		str = std::wstring(str.begin() + 1, str.end());

	return std::wstring()
		+ L"\"" + str +
		+ L"\" " + media_info->clip_type +
		+ L" " + sizeStr +
		+ L" " + writeTimeWStr +
		+ L" " + boost::lexical_cast<std::wstring>(media_info->duration) +
		+ L" " + boost::lexical_cast<std::wstring>(media_info->time_base.numerator()) + L"/" + boost::lexical_cast<std::wstring>(media_info->time_base.denominator())
		+ L"\r\n";
}

std::wstring ListMedia(const spl::shared_ptr<media_info_repository>& media_info_repo)
{	
	std::wstringstream replyString;
	for (boost::filesystem::recursive_directory_iterator itr(env::media_folder()), end; itr != end; ++itr)
		replyString << MediaInfo(itr->path(), media_info_repo);
	
	return boost::to_upper_copy(replyString.str());
}

std::wstring ListTemplates(const spl::shared_ptr<core::cg_producer_registry>& cg_registry)
{
	std::wstringstream replyString;

	for (boost::filesystem::recursive_directory_iterator itr(env::template_folder()), end; itr != end; ++itr)
	{		
		if(boost::filesystem::is_regular_file(itr->path()) && cg_registry->is_cg_extension(itr->path().extension().wstring()))
		{
			auto relativePath = boost::filesystem::path(itr->path().wstring().substr(env::template_folder().size()-1, itr->path().wstring().size()));

			auto writeTimeStr = boost::posix_time::to_iso_string(boost::posix_time::from_time_t(boost::filesystem::last_write_time(itr->path())));
			writeTimeStr.erase(std::remove_if(writeTimeStr.begin(), writeTimeStr.end(), [](char c){ return std::isdigit(c) == 0;}), writeTimeStr.end());
			auto writeTimeWStr = std::wstring(writeTimeStr.begin(), writeTimeStr.end());

			auto sizeStr = boost::lexical_cast<std::string>(boost::filesystem::file_size(itr->path()));
			sizeStr.erase(std::remove_if(sizeStr.begin(), sizeStr.end(), [](char c){ return std::isdigit(c) == 0;}), sizeStr.end());

			auto sizeWStr = std::wstring(sizeStr.begin(), sizeStr.end());

			auto dir = relativePath.parent_path();
			auto file = boost::to_upper_copy(relativePath.filename().wstring());
			relativePath = dir / file;
						
			auto str = relativePath.replace_extension(L"").generic_wstring();
			boost::trim_if(str, boost::is_any_of("\\/"));

			replyString << L"\"" << str
						<< L"\" " << sizeWStr
						<< L" " << writeTimeWStr
						<< L"\r\n";
		}
	}
	return replyString.str();
}

namespace amcp {
	
void AMCPCommand::SendReply()
{
	if(replyString_.empty())
		return;

	client_->send(std::move(replyString_));
}

bool DiagnosticsCommand::DoExecute()
{	
	try
	{
		core::diagnostics::osd::show_graphs(true);

		SetReplyString(L"202 DIAG OK\r\n");

		return true;
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"502 DIAG FAILED\r\n");
		return false;
	}
}

bool ChannelGridCommand::DoExecute()
{
	int index = 1;
	auto self = channels().back().channel;
	
	core::diagnostics::scoped_call_context save;
	core::diagnostics::call_context::for_thread().video_channel = channels().size();

	std::vector<std::wstring> params;
	params.push_back(L"SCREEN");
	params.push_back(L"0");
	params.push_back(L"NAME");
	params.push_back(L"Channel Grid Window");
	auto screen = create_consumer(params, &self->stage());

	self->output().add(screen);

	for (auto& channel : channels())
	{
		if(channel.channel != self)
		{
			core::diagnostics::call_context::for_thread().layer = index;
			auto producer = reroute::create_producer(*channel.channel);
			self->stage().load(index, producer, false);
			self->stage().play(index);
			index++;
		}
	}

	int n = channels().size()-1;
	double delta = 1.0/static_cast<double>(n);
	for(int x = 0; x < n; ++x)
	{
		for(int y = 0; y < n; ++y)
		{
			int index = x+y*n+1;
			auto transform = [=](frame_transform transform) -> frame_transform
			{		
				transform.image_transform.fill_translation[0]	= x*delta;
				transform.image_transform.fill_translation[1]	= y*delta;
				transform.image_transform.fill_scale[0]			= delta;
				transform.image_transform.fill_scale[1]			= delta;
				transform.image_transform.clip_translation[0]	= x*delta;
				transform.image_transform.clip_translation[1]	= y*delta;
				transform.image_transform.clip_scale[0]			= delta;
				transform.image_transform.clip_scale[1]			= delta;			
				return transform;
			};
			self->stage().apply_transform(index, transform);
		}
	}

	return true;
}

bool CallCommand::DoExecute()
{	
	//Perform loading of the clip
	try
	{
		auto result = channel()->stage().call(layer_index(), parameters());
		
		// TODO: because of std::async deferred timed waiting does not work

		/*auto wait_res = result.wait_for(std::chrono::seconds(2));
		if (wait_res == std::future_status::timeout)
			CASPAR_THROW_EXCEPTION(timed_out());*/
				
		std::wstringstream replyString;
		if(result.get().empty())
			replyString << L"202 CALL OK\r\n";
		else
			replyString << L"201 CALL OK\r\n" << result.get() << L"\r\n";
		
		SetReplyString(replyString.str());

		return true;
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"502 CALL FAILED\r\n");
		return false;
	}
}

tbb::concurrent_unordered_map<int, std::vector<stage::transform_tuple_t>> deferred_transforms;

bool MixerCommand::DoExecute()
{	
	//Perform loading of the clip
	try
	{	
		bool defer = boost::iequals(parameters().back(), L"DEFER");
		if(defer)
			parameters().pop_back();

		std::vector<stage::transform_tuple_t> transforms;

		if(boost::iequals(parameters()[0], L"KEYER") || boost::iequals(parameters()[0], L"IS_KEY"))
		{
			bool value = boost::lexical_cast<int>(parameters().at(1));
			transforms.push_back(stage::transform_tuple_t(layer_index(), [=](frame_transform transform) -> frame_transform
			{
				transform.image_transform.is_key = value;
				return transform;					
			}, 0, L"linear"));
		}
		else if(boost::iequals(parameters()[0], L"OPACITY"))
		{
			int duration = parameters().size() > 2 ? boost::lexical_cast<int>(parameters()[2]) : 0;
			std::wstring tween = parameters().size() > 3 ? parameters()[3] : L"linear";

			double value = boost::lexical_cast<double>(parameters().at(1));
			
			transforms.push_back(stage::transform_tuple_t(layer_index(), [=](frame_transform transform) -> frame_transform
			{
				transform.image_transform.opacity = value;
				return transform;					
			}, duration, tween));
		}
		else if(boost::iequals(parameters()[0], L"FILL") || boost::iequals(parameters()[0], L"FILL_RECT"))
		{
			int duration = parameters().size() > 5 ? boost::lexical_cast<int>(parameters()[5]) : 0;
			std::wstring tween = parameters().size() > 6 ? parameters()[6] : L"linear";
			double x	= boost::lexical_cast<double>(parameters().at(1));
			double y	= boost::lexical_cast<double>(parameters().at(2));
			double x_s	= boost::lexical_cast<double>(parameters().at(3));
			double y_s	= boost::lexical_cast<double>(parameters().at(4));

			transforms.push_back(stage::transform_tuple_t(layer_index(), [=](frame_transform transform) mutable -> frame_transform
			{
				transform.image_transform.fill_translation[0]	= x;
				transform.image_transform.fill_translation[1]	= y;
				transform.image_transform.fill_scale[0]			= x_s;
				transform.image_transform.fill_scale[1]			= y_s;
				return transform;
			}, duration, tween));
		}
		else if(boost::iequals(parameters()[0], L"CLIP") || boost::iequals(parameters()[0], L"CLIP_RECT"))
		{
			int duration = parameters().size() > 5 ? boost::lexical_cast<int>(parameters()[5]) : 0;
			std::wstring tween = parameters().size() > 6 ? parameters()[6] : L"linear";
			double x	= boost::lexical_cast<double>(parameters().at(1));
			double y	= boost::lexical_cast<double>(parameters().at(2));
			double x_s	= boost::lexical_cast<double>(parameters().at(3));
			double y_s	= boost::lexical_cast<double>(parameters().at(4));

			transforms.push_back(stage::transform_tuple_t(layer_index(), [=](frame_transform transform) -> frame_transform
			{
				transform.image_transform.clip_translation[0]	= x;
				transform.image_transform.clip_translation[1]	= y;
				transform.image_transform.clip_scale[0]			= x_s;
				transform.image_transform.clip_scale[1]			= y_s;
				return transform;
			}, duration, tween));
		}
		else if(boost::iequals(parameters()[0], L"GRID"))
		{
			int duration = parameters().size() > 2 ? boost::lexical_cast<int>(parameters()[2]) : 0;
			std::wstring tween = parameters().size() > 3 ? parameters()[3] : L"linear";
			int n = boost::lexical_cast<int>(parameters().at(1));
			double delta = 1.0/static_cast<double>(n);
			for(int x = 0; x < n; ++x)
			{
				for(int y = 0; y < n; ++y)
				{
					int index = x+y*n+1;
					transforms.push_back(stage::transform_tuple_t(index, [=](frame_transform transform) -> frame_transform
					{		
						transform.image_transform.fill_translation[0]	= x*delta;
						transform.image_transform.fill_translation[1]	= y*delta;
						transform.image_transform.fill_scale[0]			= delta;
						transform.image_transform.fill_scale[1]			= delta;
						transform.image_transform.clip_translation[0]	= x*delta;
						transform.image_transform.clip_translation[1]	= y*delta;
						transform.image_transform.clip_scale[0]			= delta;
						transform.image_transform.clip_scale[1]			= delta;			
						return transform;
					}, duration, tween));
				}
			}
		}
		else if(boost::iequals(parameters()[0], L"BLEND"))
		{
			auto blend_str = parameters().at(1);								
			int layer = layer_index();
			channel()->mixer().set_blend_mode(layer, get_blend_mode(blend_str));	
		}
		else if(boost::iequals(parameters()[0], L"MASTERVOLUME"))
		{
			float master_volume = boost::lexical_cast<float>(parameters().at(1));
			channel()->mixer().set_master_volume(master_volume);
		}
		else if(boost::iequals(parameters()[0], L"BRIGHTNESS"))
		{
			auto value = boost::lexical_cast<double>(parameters().at(1));
			int duration = parameters().size() > 2 ? boost::lexical_cast<int>(parameters()[2]) : 0;
			std::wstring tween = parameters().size() > 3 ? parameters()[3] : L"linear";
			transforms.push_back(stage::transform_tuple_t(layer_index(), [=](frame_transform transform) -> frame_transform
			{
				transform.image_transform.brightness = value;
				return transform;
			}, duration, tween));
		}
		else if(boost::iequals(parameters()[0], L"SATURATION"))
		{
			auto value = boost::lexical_cast<double>(parameters().at(1));
			int duration = parameters().size() > 2 ? boost::lexical_cast<int>(parameters()[2]) : 0;
			std::wstring tween = parameters().size() > 3 ? parameters()[3] : L"linear";
			transforms.push_back(stage::transform_tuple_t(layer_index(), [=](frame_transform transform) -> frame_transform
			{
				transform.image_transform.saturation = value;
				return transform;
			}, duration, tween));	
		}
		else if(parameters()[0] == L"CONTRAST")
		{
			auto value = boost::lexical_cast<double>(parameters().at(1));
			int duration = parameters().size() > 2 ? boost::lexical_cast<int>(parameters()[2]) : 0;
			std::wstring tween = parameters().size() > 3 ? parameters()[3] : L"linear";
			transforms.push_back(stage::transform_tuple_t(layer_index(), [=](frame_transform transform) -> frame_transform
			{
				transform.image_transform.contrast = value;
				return transform;
			}, duration, tween));	
		}
		else if(boost::iequals(parameters()[0], L"LEVELS"))
		{
			levels value;
			value.min_input  = boost::lexical_cast<double>(parameters().at(1));
			value.max_input  = boost::lexical_cast<double>(parameters().at(2));
			value.gamma		 = boost::lexical_cast<double>(parameters().at(3));
			value.min_output = boost::lexical_cast<double>(parameters().at(4));
			value.max_output = boost::lexical_cast<double>(parameters().at(5));
			int duration = parameters().size() > 6 ? boost::lexical_cast<int>(parameters()[6]) : 0;
			std::wstring tween = parameters().size() > 7 ? parameters()[7] : L"linear";

			transforms.push_back(stage::transform_tuple_t(layer_index(), [=](frame_transform transform) -> frame_transform
			{
				transform.image_transform.levels = value;
				return transform;
			}, duration, tween));
		}
		else if(boost::iequals(parameters()[0], L"VOLUME"))
		{
			int duration = parameters().size() > 2 ? boost::lexical_cast<int>(parameters()[2]) : 0;
			std::wstring tween = parameters().size() > 3 ? parameters()[3] : L"linear";
			double value = boost::lexical_cast<double>(parameters()[1]);

			transforms.push_back(stage::transform_tuple_t(layer_index(), [=](frame_transform transform) -> frame_transform
			{
				transform.audio_transform.volume = value;
				return transform;
			}, duration, tween));
		}
		else if(boost::iequals(parameters()[0], L"CLEAR"))
		{
			int layer = layer_index(std::numeric_limits<int>::max());

			if (layer == std::numeric_limits<int>::max())
			{
				channel()->stage().clear_transforms();
				channel()->mixer().clear_blend_modes();
			}
			else
			{
				channel()->stage().clear_transforms(layer);
				channel()->mixer().clear_blend_mode(layer);
			}
		}
		else if(boost::iequals(parameters()[0], L"COMMIT"))
		{
			transforms = std::move(deferred_transforms[channel_index()]);
		}
		else
		{
			SetReplyString(L"404 MIXER ERROR\r\n");
			return false;
		}

		if(defer)
		{
			auto& defer_tranforms = deferred_transforms[channel_index()];
			defer_tranforms.insert(defer_tranforms.end(), transforms.begin(), transforms.end());
		}
		else
			channel()->stage().apply_transforms(transforms);
	
		SetReplyString(L"202 MIXER OK\r\n");

		return true;
	}
	catch(file_not_found&)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"404 MIXER ERROR\r\n");
		return false;
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"502 MIXER FAILED\r\n");
		return false;
	}
}

bool SwapCommand::DoExecute()
{	
	//Perform loading of the clip
	try
	{
		if(layer_index(-1) != -1)
		{
			std::vector<std::string> strs;
			boost::split(strs, parameters()[0], boost::is_any_of("-"));
			
			auto ch1 = channel();
			auto ch2 = channels().at(boost::lexical_cast<int>(strs.at(0))-1);

			int l1 = layer_index();
			int l2 = boost::lexical_cast<int>(strs.at(1));

			ch1->stage().swap_layer(l1, l2, ch2.channel->stage());
		}
		else
		{
			auto ch1 = channel();
			auto ch2 = channels().at(boost::lexical_cast<int>(parameters()[0])-1);
			ch1->stage().swap_layers(ch2.channel->stage());
		}
		
		SetReplyString(L"202 SWAP OK\r\n");

		return true;
	}
	catch(file_not_found&)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"404 SWAP ERROR\r\n");
		return false;
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"502 SWAP FAILED\r\n");
		return false;
	}
}

bool AddCommand::DoExecute()
{	
	//Perform loading of the clip
	try
	{
		//create_consumer still expects all parameters to be uppercase
		for (auto& str : parameters())
		{
			boost::to_upper(str);
		}

		core::diagnostics::scoped_call_context save;
		core::diagnostics::call_context::for_thread().video_channel = channel_index() + 1;

		auto consumer = create_consumer(parameters(), &channel()->stage());
		channel()->output().add(layer_index(consumer->index()), consumer);
	
		SetReplyString(L"202 ADD OK\r\n");

		return true;
	}
	catch(file_not_found&)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"404 ADD ERROR\r\n");
		return false;
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"502 ADD FAILED\r\n");
		return false;
	}
}

bool RemoveCommand::DoExecute()
{	
	//Perform loading of the clip
	try
	{
		auto index = layer_index(std::numeric_limits<int>::min());
		if(index == std::numeric_limits<int>::min())
		{
			//create_consumer still expects all parameters to be uppercase
			for (auto& str : parameters())
			{
				boost::to_upper(str);
			}

			index = create_consumer(parameters(), &channel()->stage())->index();
		}

		channel()->output().remove(index);

		SetReplyString(L"202 REMOVE OK\r\n");

		return true;
	}
	catch(file_not_found&)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"404 REMOVE ERROR\r\n");
		return false;
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"502 REMOVE FAILED\r\n");
		return false;
	}
}

bool LoadCommand::DoExecute()
{	
	//Perform loading of the clip
	try
	{
		core::diagnostics::scoped_call_context save;
		core::diagnostics::call_context::for_thread().video_channel = channel_index() + 1;
		core::diagnostics::call_context::for_thread().layer = layer_index();
		auto pFP = create_producer(channel()->frame_factory(), channel()->video_format_desc(), parameters());
		channel()->stage().load(layer_index(), pFP, true);
	
		SetReplyString(L"202 LOAD OK\r\n");

		return true;
	}
	catch(file_not_found&)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"404 LOAD ERROR\r\n");
		return false;
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"502 LOAD FAILED\r\n");
		return false;
	}
}

bool LoadbgCommand::DoExecute()
{
	transition_info transitionInfo;
	
	// TRANSITION

	std::wstring message;
	for(size_t n = 0; n < parameters().size(); ++n)
		message += boost::to_upper_copy(parameters()[n]) + L" ";
		
	static const boost::wregex expr(LR"(.*(?<TRANSITION>CUT|PUSH|SLIDE|WIPE|MIX)\s*(?<DURATION>\d+)\s*(?<TWEEN>(LINEAR)|(EASE[^\s]*))?\s*(?<DIRECTION>FROMLEFT|FROMRIGHT|LEFT|RIGHT)?.*)");
	boost::wsmatch what;
	if(boost::regex_match(message, what, expr))
	{
		auto transition = what["TRANSITION"].str();
		transitionInfo.duration = boost::lexical_cast<size_t>(what["DURATION"].str());
		auto direction = what["DIRECTION"].matched ? what["DIRECTION"].str() : L"";
		auto tween = what["TWEEN"].matched ? what["TWEEN"].str() : L"";
		transitionInfo.tweener = tween;		

		if(transition == L"CUT")
			transitionInfo.type = transition_type::cut;
		else if(transition == L"MIX")
			transitionInfo.type = transition_type::mix;
		else if(transition == L"PUSH")
			transitionInfo.type = transition_type::push;
		else if(transition == L"SLIDE")
			transitionInfo.type = transition_type::slide;
		else if(transition == L"WIPE")
			transitionInfo.type = transition_type::wipe;
		
		if(direction == L"FROMLEFT")
			transitionInfo.direction = transition_direction::from_left;
		else if(direction == L"FROMRIGHT")
			transitionInfo.direction = transition_direction::from_right;
		else if(direction == L"LEFT")
			transitionInfo.direction = transition_direction::from_right;
		else if(direction == L"RIGHT")
			transitionInfo.direction = transition_direction::from_left;
	}
	
	//Perform loading of the clip
	try
	{
		std::shared_ptr<core::frame_producer> pFP;
		
		static boost::wregex expr(LR"(\[(?<CHANNEL>\d+)\])", boost::regex::icase);
			
		core::diagnostics::scoped_call_context save;
		core::diagnostics::call_context::for_thread().video_channel = channel_index() + 1;
		core::diagnostics::call_context::for_thread().layer = layer_index();

		boost::wsmatch what;
		if(boost::regex_match(parameters().at(0), what, expr))
		{
			auto channel_index = boost::lexical_cast<int>(what["CHANNEL"].str());
			pFP = reroute::create_producer(*channels().at(channel_index-1).channel); 
		}
		else
		{
			pFP = create_producer(channel()->frame_factory(), channel()->video_format_desc(), parameters());
		}
		
		if(pFP == frame_producer::empty())
			CASPAR_THROW_EXCEPTION(file_not_found() << msg_info(parameters().size() > 0 ? parameters()[0] : L""));

		bool auto_play = contains_param(L"AUTO", parameters());

		auto pFP2 = create_transition_producer(channel()->video_format_desc().field_mode, spl::make_shared_ptr(pFP), transitionInfo);
		if(auto_play)
			channel()->stage().load(layer_index(), pFP2, false, transitionInfo.duration); // TODO: LOOP
		else
			channel()->stage().load(layer_index(), pFP2, false); // TODO: LOOP
	
		
		SetReplyString(L"202 LOADBG OK\r\n");

		return true;
	}
	catch(file_not_found&)
	{		
		CASPAR_LOG(error) << L"File not found. No match found for parameters. Check syntax.";
		SetReplyString(L"404 LOADBG ERROR\r\n");
		return false;
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"502 LOADBG FAILED\r\n");
		return false;
	}
}

bool PauseCommand::DoExecute()
{
	try
	{
		channel()->stage().pause(layer_index());
		SetReplyString(L"202 PAUSE OK\r\n");
		return true;
	}
	catch(...)
	{
		SetReplyString(L"501 PAUSE FAILED\r\n");
	}

	return false;
}

bool PlayCommand::DoExecute()
{
	try
	{
		if(!parameters().empty())
		{
			LoadbgCommand lbg(*this);

			if(!lbg.Execute())
				throw std::exception();
		}

		channel()->stage().play(layer_index());
		
		SetReplyString(L"202 PLAY OK\r\n");
		return true;
	}
	catch(...)
	{
		SetReplyString(L"501 PLAY FAILED\r\n");
	}

	return false;
}

bool StopCommand::DoExecute()
{
	try
	{
		channel()->stage().stop(layer_index());
		SetReplyString(L"202 STOP OK\r\n");
		return true;
	}
	catch(...)
	{
		SetReplyString(L"501 STOP FAILED\r\n");
	}

	return false;
}

bool ClearCommand::DoExecute()
{
	int index = layer_index(std::numeric_limits<int>::min());
	if(index != std::numeric_limits<int>::min())
		channel()->stage().clear(index);
	else
		channel()->stage().clear();
		
	SetReplyString(L"202 CLEAR OK\r\n");

	return true;
}

bool PrintCommand::DoExecute()
{
	channel()->output().add(create_consumer({ L"IMAGE" }, &channel()->stage()));
		
	SetReplyString(L"202 PRINT OK\r\n");

	return true;
}

bool LogCommand::DoExecute()
{
	if(boost::iequals(parameters().at(0), L"LEVEL"))
		log::set_log_level(parameters().at(1));

	SetReplyString(L"202 LOG OK\r\n");

	return true;
}

bool CGCommand::DoExecute()
{
	try
	{
		std::wstring command = boost::to_upper_copy(parameters()[0]);
		if(command == L"ADD")
			return DoExecuteAdd();
		else if(command == L"PLAY")
			return DoExecutePlay();
		else if(command == L"STOP")
			return DoExecuteStop();
		else if(command == L"NEXT")
			return DoExecuteNext();
		else if(command == L"REMOVE")
			return DoExecuteRemove();
		else if(command == L"CLEAR")
			return DoExecuteClear();
		else if(command == L"UPDATE")
			return DoExecuteUpdate();
		else if(command == L"INVOKE")
			return DoExecuteInvoke();
		else if(command == L"INFO")
			return DoExecuteInfo();
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
	}

	SetReplyString(L"403 CG ERROR\r\n");
	return false;
}

bool CGCommand::ValidateLayer(const std::wstring& layerstring) {
	int length = layerstring.length();
	for(int i = 0; i < length; ++i) {
		if(!std::isdigit(layerstring[i])) {
			return false;
		}
	}

	return true;
}

bool CGCommand::DoExecuteAdd() {
	//CG 1 ADD 0 "template_folder/templatename" [STARTLABEL] 0/1 [DATA]

	int layer = 0;				//_parameters[1]
//	std::wstring templateName;	//_parameters[2]
	std::wstring label;		//_parameters[3]
	bool bDoStart = false;		//_parameters[3] alt. _parameters[4]
//	std::wstring data;			//_parameters[4] alt. _parameters[5]

	if(parameters().size() < 4) 
	{
		SetReplyString(L"402 CG ERROR\r\n");
		return false;
	}
	unsigned int dataIndex = 4;

	if(!ValidateLayer(parameters()[1])) 
	{
		SetReplyString(L"403 CG ERROR\r\n");
		return false;
	}

	layer = boost::lexical_cast<int>(parameters()[1]);

	if(parameters()[3].length() > 1) 
	{	//read label
		label = parameters()[3];
		++dataIndex;

		if(parameters().size() > 4 && parameters()[4].length() > 0)	//read play-on-load-flag
			bDoStart = (parameters()[4][0]==L'1') ? true : false;
		else 
		{
			SetReplyString(L"402 CG ERROR\r\n");
			return false;
		}
	}
	else if(parameters()[3].length() > 0) {	//read play-on-load-flag
		bDoStart = (parameters()[3][0]==L'1') ? true : false;
	}
	else 
	{
		SetReplyString(L"403 CG ERROR\r\n");
		return false;
	}

	const wchar_t* pDataString = 0;
	std::wstring dataFromFile;
	if(parameters().size() > dataIndex) 
	{	//read data
		const std::wstring& dataString = parameters()[dataIndex];

		if(dataString[0] == L'<') //the data is an XML-string
			pDataString = dataString.c_str();
		else 
		{
			//The data is not an XML-string, it must be a filename
			std::wstring filename = env::data_folder();
			filename.append(dataString);
			filename.append(L".ftd");

			auto found_file = find_case_insensitive(filename);

			if (found_file)
			{
				dataFromFile = read_file(boost::filesystem::path(*found_file));
				pDataString = dataFromFile.c_str();
			}
		}
	}

	auto filename = parameters()[2];
	auto proxy = cg_registry_->get_or_create_proxy(channel(), layer_index(core::cg_proxy::DEFAULT_LAYER), filename);

	if (proxy == core::cg_proxy::empty())
	{
		CASPAR_LOG(warning) << "Could not find template " << parameters()[2];
		SetReplyString(L"404 CG ERROR\r\n");
	}
	else
	{
		proxy->add(layer, filename, bDoStart, label, (pDataString != 0) ? pDataString : L"");
	}

	return true;
}

bool CGCommand::DoExecutePlay()
{
	if(parameters().size() > 1)
	{
		if(!ValidateLayer(parameters()[1])) 
		{
			SetReplyString(L"403 CG ERROR\r\n");
			return false;
		}
		int layer = boost::lexical_cast<int>(parameters()[1]);
		cg_registry_->get_proxy(channel(), layer_index(core::cg_proxy::DEFAULT_LAYER))->play(layer);
	}
	else
	{
		SetReplyString(L"402 CG ERROR\r\n");
		return true;
	}

	SetReplyString(L"202 CG OK\r\n");
	return true;
}

bool CGCommand::DoExecuteStop() 
{
	if(parameters().size() > 1)
	{
		if(!ValidateLayer(parameters()[1])) 
		{
			SetReplyString(L"403 CG ERROR\r\n");
			return false;
		}
		int layer = boost::lexical_cast<int>(parameters()[1]);
		cg_registry_->get_proxy(channel(), layer_index(core::cg_proxy::DEFAULT_LAYER))->stop(layer, 0);
	}
	else 
	{
		SetReplyString(L"402 CG ERROR\r\n");
		return true;
	}

	SetReplyString(L"202 CG OK\r\n");
	return true;
}

bool CGCommand::DoExecuteNext()
{
	if(parameters().size() > 1) 
	{
		if(!ValidateLayer(parameters()[1])) 
		{
			SetReplyString(L"403 CG ERROR\r\n");
			return false;
		}

		int layer = boost::lexical_cast<int>(parameters()[1]);
		cg_registry_->get_proxy(channel(), layer_index(core::cg_proxy::DEFAULT_LAYER))->next(layer);
	}
	else 
	{
		SetReplyString(L"402 CG ERROR\r\n");
		return true;
	}

	SetReplyString(L"202 CG OK\r\n");
	return true;
}

bool CGCommand::DoExecuteRemove() 
{
	if(parameters().size() > 1) 
	{
		if(!ValidateLayer(parameters()[1])) 
		{
			SetReplyString(L"403 CG ERROR\r\n");
			return false;
		}

		int layer = boost::lexical_cast<int>(parameters()[1]);
		cg_registry_->get_proxy(channel(), layer_index(core::cg_proxy::DEFAULT_LAYER))->remove(layer);
	}
	else 
	{
		SetReplyString(L"402 CG ERROR\r\n");
		return true;
	}

	SetReplyString(L"202 CG OK\r\n");
	return true;
}

bool CGCommand::DoExecuteClear() 
{
	channel()->stage().clear(layer_index(core::cg_proxy::DEFAULT_LAYER));
	SetReplyString(L"202 CG OK\r\n");
	return true;
}

bool CGCommand::DoExecuteUpdate() 
{
	try
	{
		if(!ValidateLayer(parameters().at(1)))
		{
			SetReplyString(L"403 CG ERROR\r\n");
			return false;
		}
						
		std::wstring dataString = parameters().at(2);				
		if(dataString.at(0) != L'<')
		{
			//The data is not an XML-string, it must be a filename
			std::wstring filename = env::data_folder();
			filename.append(dataString);
			filename.append(L".ftd");

			dataString = read_file(boost::filesystem::path(filename));
		}		

		int layer = boost::lexical_cast<int>(parameters()[1]);
		cg_registry_->get_proxy(channel(), layer_index(core::cg_proxy::DEFAULT_LAYER))->update(layer, dataString);
	}
	catch(...)
	{
		SetReplyString(L"402 CG ERROR\r\n");
		return true;
	}

	SetReplyString(L"202 CG OK\r\n");
	return true;
}

bool CGCommand::DoExecuteInvoke() 
{
	std::wstringstream replyString;
	replyString << L"201 CG OK\r\n";

	if(parameters().size() > 2)
	{
		if(!ValidateLayer(parameters()[1]))
		{
			SetReplyString(L"403 CG ERROR\r\n");
			return false;
		}
		int layer = boost::lexical_cast<int>(parameters()[1]);
		auto result = cg_registry_->get_proxy(channel(), layer_index(core::cg_proxy::DEFAULT_LAYER))->invoke(layer, parameters()[2]);
		replyString << result << L"\r\n";
	}
	else 
	{
		SetReplyString(L"402 CG ERROR\r\n");
		return true;
	}
	
	SetReplyString(replyString.str());
	return true;
}

bool CGCommand::DoExecuteInfo() 
{
	std::wstringstream replyString;
	replyString << L"201 CG OK\r\n";

	if(parameters().size() > 1)
	{
		if(!ValidateLayer(parameters()[1]))
		{
			SetReplyString(L"403 CG ERROR\r\n");
			return false;
		}

		int layer = boost::lexical_cast<int>(parameters()[1]);
		auto desc = cg_registry_->get_proxy(channel(), layer_index(core::cg_proxy::DEFAULT_LAYER))->description(layer);
		
		replyString << desc << L"\r\n";
	}
	else 
	{
		auto info = cg_registry_->get_proxy(channel(), layer_index(core::cg_proxy::DEFAULT_LAYER))->template_host_info();
		replyString << info << L"\r\n";
	}	

	SetReplyString(replyString.str());
	return true;
}

bool DataCommand::DoExecute()
{
	std::wstring command = boost::to_upper_copy(parameters()[0]);
	if(command == L"STORE")
		return DoExecuteStore();
	else if(command == L"RETRIEVE")
		return DoExecuteRetrieve();
	else if(command == L"REMOVE")
		return DoExecuteRemove();
	else if(command == L"LIST")
		return DoExecuteList();

	SetReplyString(L"403 DATA ERROR\r\n");
	return false;
}

bool DataCommand::DoExecuteStore() 
{
	if(parameters().size() < 3) 
	{
		SetReplyString(L"402 DATA STORE ERROR\r\n");
		return false;
	}

	std::wstring filename = env::data_folder();
	filename.append(parameters()[1]);
	filename.append(L".ftd");

	auto data_path = boost::filesystem::path(filename).parent_path().wstring();
	auto found_data_path = find_case_insensitive(data_path);

	if (found_data_path)
		data_path = *found_data_path;

	if(!boost::filesystem::exists(data_path))
		boost::filesystem::create_directories(data_path);

	auto found_filename = find_case_insensitive(filename);

	if (found_filename)
		filename = *found_filename; // Overwrite case insensitive.

	boost::filesystem::wofstream datafile(filename);
	if(!datafile) 
	{
		SetReplyString(L"501 DATA STORE FAILED\r\n");
		return false;
	}

	datafile << static_cast<wchar_t>(65279); // UTF-8 BOM character
	datafile << parameters()[2] << std::flush;
	datafile.close();

	std::wstring replyString = L"202 DATA STORE OK\r\n";
	SetReplyString(replyString);
	return true;
}

bool DataCommand::DoExecuteRetrieve() 
{
	if(parameters().size() < 2) 
	{
		SetReplyString(L"402 DATA RETRIEVE ERROR\r\n");
		return false;
	}

	std::wstring filename = env::data_folder();
	filename.append(parameters()[1]);
	filename.append(L".ftd");

	std::wstring file_contents;

	auto found_file = find_case_insensitive(filename);

	if (found_file)
		file_contents = read_file(boost::filesystem::path(*found_file));

	if (file_contents.empty()) 
	{
		SetReplyString(L"404 DATA RETRIEVE ERROR\r\n");
		return false;
	}

	std::wstringstream reply(L"201 DATA RETRIEVE OK\r\n");

	std::wstringstream file_contents_stream(file_contents);
	std::wstring line;
	
	bool firstLine = true;
	while(std::getline(file_contents_stream, line))
	{
		if(firstLine)
			firstLine = false;
		else
			reply << "\n";

		reply << line;
	}

	reply << "\r\n";
	SetReplyString(reply.str());
	return true;
}

bool DataCommand::DoExecuteRemove()
{ 
	if (parameters().size() < 2)
	{
		SetReplyString(L"402 DATA REMOVE ERROR\r\n");
		return false;
	}

	std::wstring filename = env::data_folder();
	filename.append(parameters()[1]);
	filename.append(L".ftd");

	if (!boost::filesystem::exists(filename))
	{
		SetReplyString(L"404 DATA REMOVE ERROR\r\n");
		return false;
	}

	if (!boost::filesystem::remove(filename))
	{
		SetReplyString(L"403 DATA REMOVE ERROR\r\n");
		return false;
	}

	SetReplyString(L"201 DATA REMOVE OK\r\n");

	return true;
}

bool DataCommand::DoExecuteList() 
{
	std::wstringstream replyString;
	replyString << L"200 DATA LIST OK\r\n";

	for (boost::filesystem::recursive_directory_iterator itr(env::data_folder()), end; itr != end; ++itr)
	{			
		if(boost::filesystem::is_regular_file(itr->path()))
		{
			if(!boost::iequals(itr->path().extension().wstring(), L".ftd"))
				continue;
			
			auto relativePath = boost::filesystem::path(itr->path().wstring().substr(env::data_folder().size()-1, itr->path().wstring().size()));
			
			auto str = relativePath.replace_extension(L"").generic_wstring();
			if(str[0] == L'\\' || str[0] == L'/')
				str = std::wstring(str.begin() + 1, str.end());

			replyString << str << L"\r\n";
		}
	}
	
	replyString << L"\r\n";

	SetReplyString(boost::to_upper_copy(replyString.str()));
	return true;
}

bool CinfCommand::DoExecute()
{
	std::wstringstream replyString;
	
	try
	{
		std::wstring info;
		for (boost::filesystem::recursive_directory_iterator itr(env::media_folder()), end; itr != end && info.empty(); ++itr)
		{
			auto path = itr->path();
			auto file = path.replace_extension(L"").filename();
			if(boost::iequals(file.wstring(), parameters().at(0)))
				info += MediaInfo(itr->path(), system_info_repo_) + L"\r\n";
		}

		if(info.empty())
		{
			SetReplyString(L"404 CINF ERROR\r\n");
			return false;
		}
		replyString << L"200 CINF OK\r\n";
		replyString << info << "\r\n";
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"404 CINF ERROR\r\n");
		return false;
	}
	
	SetReplyString(replyString.str());
	return true;
}

void GenerateChannelInfo(int index, const spl::shared_ptr<core::video_channel>& pChannel, std::wstringstream& replyString)
{
	replyString << index+1 << L" " << pChannel->video_format_desc().name << L" PLAYING\r\n";
}

bool InfoCommand::DoExecute()
{
	std::wstringstream replyString;
	
	boost::property_tree::xml_writer_settings<std::wstring> w(' ', 3);

	try
	{
		if(parameters().size() >= 1 && boost::iequals(parameters()[0], L"TEMPLATE"))
		{		
			replyString << L"201 INFO TEMPLATE OK\r\n";

			auto filename = parameters().at(1);
						
			std::wstringstream str;
			str << u16(cg_registry_->read_meta_info(filename));
			boost::property_tree::wptree info;
			boost::property_tree::xml_parser::read_xml(str, info, boost::property_tree::xml_parser::trim_whitespace | boost::property_tree::xml_parser::no_comments);

			boost::property_tree::xml_parser::write_xml(replyString, info, w);
		}
		else if(parameters().size() >= 1 && boost::iequals(parameters()[0], L"CONFIG"))
		{		
			replyString << L"201 INFO CONFIG OK\r\n";

			boost::property_tree::write_xml(replyString, caspar::env::properties(), w);
		}
		else if(parameters().size() >= 1 && boost::iequals(parameters()[0], L"PATHS"))
		{
			replyString << L"201 INFO PATHS OK\r\n";

			boost::property_tree::wptree info;
			info.add_child(L"paths", caspar::env::properties().get_child(L"configuration.paths"));
			info.add(L"paths.initial-path", boost::filesystem::initial_path().wstring() + L"/");

			boost::property_tree::write_xml(replyString, info, w);
		}
		else if(parameters().size() >= 1 && boost::iequals(parameters()[0], L"SYSTEM"))
		{
			replyString << L"201 INFO SYSTEM OK\r\n";
			
			boost::property_tree::wptree info;
			
			info.add(L"system.name",					caspar::system_product_name());
			info.add(L"system.os.description",			caspar::os_description());
			info.add(L"system.cpu",						caspar::cpu_info());

			system_info_repo_->fill_information(info);
						
			boost::property_tree::write_xml(replyString, info, w);
		}
		else if(parameters().size() >= 1 && boost::iequals(parameters()[0], L"SERVER"))
		{
			replyString << L"201 INFO SERVER OK\r\n";
			
			boost::property_tree::wptree info;

			int index = 0;
			for (auto& channel : channels())
				info.add_child(L"channels.channel", channel.channel->info())
					.add(L"index", ++index);
			
			boost::property_tree::write_xml(replyString, info, w);
		}
		else // channel
		{			
			if(parameters().size() >= 1)
			{
				replyString << L"201 INFO OK\r\n";
				boost::property_tree::wptree info;

				std::vector<std::wstring> split;
				boost::split(split, parameters()[0], boost::is_any_of("-"));
					
				int layer = std::numeric_limits<int>::min();
				int channel = boost::lexical_cast<int>(split[0]) - 1;

				if(split.size() > 1)
					layer = boost::lexical_cast<int>(split[1]);
				
				if(layer == std::numeric_limits<int>::min())
				{	
					info.add_child(L"channel", channels().at(channel).channel->info())
							.add(L"index", channel);
				}
				else
				{
					if(parameters().size() >= 2)
					{
						if(boost::iequals(parameters()[1], L"B"))
							info.add_child(L"producer", channels().at(channel).channel->stage().background(layer).get()->info());
						else
							info.add_child(L"producer", channels().at(channel).channel->stage().foreground(layer).get()->info());
					}
					else
					{
						info.add_child(L"layer", channels().at(channel).channel->stage().info(layer).get())
							.add(L"index", layer);
					}
				}
				boost::property_tree::xml_parser::write_xml(replyString, info, w);
			}
			else
			{
				// This is needed for backwards compatibility with old clients
				replyString << L"200 INFO OK\r\n";
				for(size_t n = 0; n < channels().size(); ++n)
					GenerateChannelInfo(n, channels()[n].channel, replyString);
			}

		}
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"403 INFO ERROR\r\n");
		return false;
	}

	replyString << L"\r\n";
	SetReplyString(replyString.str());
	return true;
}

bool ClsCommand::DoExecute()
{
	try
	{
		std::wstringstream replyString;
		replyString << L"200 CLS OK\r\n";
		replyString << ListMedia(system_info_repo_);
		replyString << L"\r\n";
		SetReplyString(boost::to_upper_copy(replyString.str()));
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"501 CLS FAILED\r\n");
		return false;
	}

	return true;
}

bool TlsCommand::DoExecute()
{
	try
	{
		std::wstringstream replyString;
		replyString << L"200 TLS OK\r\n";

		replyString << ListTemplates(cg_registry_);
		replyString << L"\r\n";

		SetReplyString(replyString.str());
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"501 TLS FAILED\r\n");
		return false;
	}
	return true;
}

bool VersionCommand::DoExecute()
{
	std::wstring replyString = L"201 VERSION OK\r\n" + env::version() + L"\r\n";

	if (parameters().size() > 0 && !boost::iequals(parameters()[0], L"SERVER"))
	{
		auto version = system_info_repo_->get_version(parameters().at(0));

		if (version.empty())
			replyString = L"403 VERSION ERROR\r\n";
		else
			replyString = L"201 VERSION OK\r\n" + version + L"\r\n";
	}

	SetReplyString(replyString);
	return true;
}

bool ByeCommand::DoExecute()
{
	client()->disconnect();
	return true;
}

bool SetCommand::DoExecute()
{
	try
	{
		std::wstring name = boost::to_upper_copy(parameters()[0]);
		std::wstring value = boost::to_upper_copy(parameters()[1]);

		if(name == L"MODE")
		{
			auto format_desc = core::video_format_desc(value);
			if(format_desc.format != core::video_format::invalid)
			{
				channel()->video_format_desc(format_desc);
				SetReplyString(L"202 SET MODE OK\r\n");
			}
			else
				SetReplyString(L"501 SET MODE FAILED\r\n");
		}
		else
		{
			this->SetReplyString(L"403 SET ERROR\r\n");
		}
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"501 SET FAILED\r\n");
		return false;
	}

	return true;
}

bool LockCommand::DoExecute()
{
	try
	{
		auto it = parameters().begin();

		std::shared_ptr<caspar::IO::lock_container> lock;
		try
		{
			int channel_index = boost::lexical_cast<int>(*it) - 1;
			lock = channels().at(channel_index).lock;
		}
		catch(const boost::bad_lexical_cast&) {}
		catch(...)
		{
			SetReplyString(L"401 LOCK ERROR\r\n");
			return false;
		}

		if(lock)
			++it;

		if(it == parameters().end())	//too few parameters
		{
			SetReplyString(L"402 LOCK ERROR\r\n");
			return false;
		}

		std::wstring command = boost::to_upper_copy(*it);
		if(command == L"ACQUIRE")
		{
			++it;
			if(it == parameters().end())	//too few parameters
			{
				SetReplyString(L"402 LOCK ACQUIRE ERROR\r\n");
				return false;
			}
			std::wstring lock_phrase = (*it);

			//TODO: read options

			if(lock)
			{
				//just lock one channel
				if(!lock->try_lock(lock_phrase, client()))
				{
					SetReplyString(L"503 LOCK ACQUIRE FAILED\r\n");
					return false;
				}
			}
			else
			{
				//TODO: lock all channels
				CASPAR_THROW_EXCEPTION(not_implemented());
			}
			SetReplyString(L"202 LOCK ACQUIRE OK\r\n");

		}
		else if(command == L"RELEASE")
		{
			if(lock)
			{
				lock->release_lock(client());
			}
			else
			{
				//TODO: release all channels
				CASPAR_THROW_EXCEPTION(not_implemented());
			}
			SetReplyString(L"202 LOCK RELEASE OK\r\n");
		}
		else if(command == L"CLEAR")
		{
			std::wstring override_phrase = env::properties().get(L"configuration.lock-clear-phrase", L"");
			std::wstring client_override_phrase;
			if(!override_phrase.empty())
			{
				++it;
				if(it == parameters().end())
				{
					SetReplyString(L"402 LOCK CLEAR ERROR\r\n");
					return false;
				}
				client_override_phrase = (*it);
			}

			if(lock)
			{
				//just clear one channel
				if(client_override_phrase != override_phrase)
				{
					SetReplyString(L"503 LOCK CLEAR FAILED\r\n");
					return false;
				}
				
				lock->clear_locks();
			}
			else
			{
				//TODO: clear all channels
				CASPAR_THROW_EXCEPTION(not_implemented());
			}

			SetReplyString(L"202 LOCK CLEAR OK\r\n");
		}
		else
		{
			SetReplyString(L"403 LOCK ERROR\r\n");
			return false;
		}
	}
	catch(not_implemented&)
	{
		SetReplyString(L"600 LOCK FAILED\r\n");
		return false;
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(L"501 LOCK FAILED\r\n");
		return false;
	}

	return true;
}

bool ThumbnailCommand::DoExecute()
{
	std::wstring command = boost::to_upper_copy(parameters()[0]);

	if (command == L"RETRIEVE")
		return DoExecuteRetrieve();
	else if (command == L"LIST")
		return DoExecuteList();
	else if (command == L"GENERATE")
		return DoExecuteGenerate();
	else if (command == L"GENERATE_ALL")
		return DoExecuteGenerateAll();

	SetReplyString(L"403 THUMBNAIL ERROR\r\n");
	return false;
}

bool ThumbnailCommand::DoExecuteRetrieve() 
{
	if(parameters().size() < 2) 
	{
		SetReplyString(L"402 THUMBNAIL RETRIEVE ERROR\r\n");
		return false;
	}

	std::wstring filename = env::thumbnails_folder();
	filename.append(parameters()[1]);
	filename.append(L".png");

	std::wstring file_contents;

	auto found_file = find_case_insensitive(filename);

	if (found_file)
		file_contents = read_file_base64(boost::filesystem::path(*found_file));

	if (file_contents.empty())
	{
		SetReplyString(L"404 THUMBNAIL RETRIEVE ERROR\r\n");
		return false;
	}

	std::wstringstream reply;

	reply << L"201 THUMBNAIL RETRIEVE OK\r\n";
	reply << file_contents;
	reply << L"\r\n";
	SetReplyString(reply.str());
	return true;
}

bool ThumbnailCommand::DoExecuteList()
{
	std::wstringstream replyString;
	replyString << L"200 THUMBNAIL LIST OK\r\n";

	for (boost::filesystem::recursive_directory_iterator itr(env::thumbnails_folder()), end; itr != end; ++itr)
	{      
		if(boost::filesystem::is_regular_file(itr->path()))
		{
			if(!boost::iequals(itr->path().extension().wstring(), L".png"))
				continue;

			auto relativePath = boost::filesystem::path(itr->path().wstring().substr(env::thumbnails_folder().size()-1, itr->path().wstring().size()));

			auto str = relativePath.replace_extension(L"").generic_wstring();
			if(str[0] == '\\' || str[0] == '/')
				str = std::wstring(str.begin() + 1, str.end());

			auto mtime = boost::filesystem::last_write_time(itr->path());
			auto mtime_readable = boost::posix_time::to_iso_wstring(boost::posix_time::from_time_t(mtime));
			auto file_size = boost::filesystem::file_size(itr->path());

			replyString << L"\"" << str << L"\" " << mtime_readable << L" " << file_size << L"\r\n";
		}
	}

	replyString << L"\r\n";

	SetReplyString(boost::to_upper_copy(replyString.str()));
	return true;
}

bool ThumbnailCommand::DoExecuteGenerate()
{
	if (parameters().size() < 2) 
	{
		SetReplyString(L"402 THUMBNAIL GENERATE ERROR\r\n");
		return false;
	}

	if (thumb_gen_)
	{
		thumb_gen_->generate(parameters()[1]);
		SetReplyString(L"202 THUMBNAIL GENERATE OK\r\n");
		return true;
	}
	else
	{
		SetReplyString(L"500 THUMBNAIL GENERATE ERROR\r\n");
		return false;
	}
}

bool ThumbnailCommand::DoExecuteGenerateAll()
{
	if (thumb_gen_)
	{
		thumb_gen_->generate_all();
		SetReplyString(L"202 THUMBNAIL GENERATE_ALL OK\r\n");
		return true;
	}
	else
	{
		SetReplyString(L"500 THUMBNAIL GENERATE_ALL ERROR\r\n");
		return false;
	}
}

bool KillCommand::DoExecute()
{
	shutdown_server_now_->set_value(false);	//false for not attempting to restart
	SetReplyString(L"202 KILL OK\r\n");
	return true;
}

bool RestartCommand::DoExecute()
{
	shutdown_server_now_->set_value(true);	//true for attempting to restart
	SetReplyString(L"202 RESTART OK\r\n");
	return true;
}

}	//namespace amcp
}}	//namespace caspar
