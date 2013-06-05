/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include <common/log/log.h>
#include <common/diagnostics/graph.h>
#include <common/os/windows/current_version.h>
#include <common/os/windows/system_info.h>
#include <common/utility/string.h>
#include <common/utility/utf8conv.h>
#include <common/utility/base64.h>

#include <core/producer/frame_producer.h>
#include <core/video_format.h>
#include <core/producer/transition/transition_producer.h>
#include <core/producer/channel/channel_producer.h>
#include <core/producer/frame/frame_transform.h>
#include <core/producer/stage.h>
#include <core/producer/layer.h>
#include <core/mixer/mixer.h>
#include <core/mixer/gpu/ogl_device.h>
#include <core/consumer/output.h>

#include <modules/bluefish/bluefish.h>
#include <modules/decklink/decklink.h>
#include <modules/ffmpeg/ffmpeg.h>
#include <modules/flash/flash.h>
#include <modules/flash/util/swf.h>
#include <modules/flash/producer/flash_producer.h>
#include <modules/flash/producer/cg_producer.h>
#include <modules/ffmpeg/producer/util/util.h>
#include <modules/image/image.h>
#include <modules/ogl/ogl.h>

#include <algorithm>
#include <locale>
#include <fstream>
#include <memory>
#include <cctype>
#include <io.h>

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

100 [action]			Information om att n�got har h�nt  
101 [action]			Information om att n�got har h�nt, en rad data skickas  

202 [kommando] OK		Kommandot har utf�rts  
201 [kommando] OK		Kommandot har utf�rts, och en rad data skickas tillbaka  
200 [kommando] OK		Kommandot har utf�rts, och flera rader data skickas tillbaka. Avslutas med tomrad  

400 ERROR				Kommandot kunde inte f�rst�s  
401 [kommando] ERROR	Ogiltig kanal  
402 [kommando] ERROR	Parameter saknas  
403 [kommando] ERROR	Ogiltig parameter  
404 [kommando] ERROR	Mediafilen hittades inte  

500 FAILED				Internt configurationfel  
501 [kommando] FAILED	Internt configurationfel  
502 [kommando] FAILED	Ol�slig mediafil  

600 [kommando] FAILED	funktion ej implementerad
*/

namespace caspar { namespace protocol {

using namespace core;

std::wstring read_file_base64(const boost::filesystem::wpath& file)
{
	using namespace boost::archive::iterators;

	boost::filesystem::ifstream filestream(file, std::ios::binary);

	if (!filestream)
		return L"";

	auto length = boost::filesystem::file_size(file);
	std::vector<char> bytes;
	bytes.resize(length);
	filestream.read(bytes.data(), length);

	return widen(to_base64(bytes.data(), length));
}

std::wstring read_utf8_file(const boost::filesystem::wpath& file)
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

std::wstring read_latin1_file(const boost::filesystem::wpath& file)
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
	auto from_signed_to_signed = std::function<unsigned char(char)>(
		[] (char c) { return static_cast<unsigned char>(c); }
	);
	boost::copy(
		result | boost::adaptors::transformed(from_signed_to_signed),
		std::back_inserter(widened_result));

	return widened_result;
}

std::wstring read_file(const boost::filesystem::wpath& file)
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

std::wstring MediaInfo(const boost::filesystem::wpath& path)
{
	if(boost::filesystem::is_regular_file(path))
	{
		std::wstring clipttype = TEXT(" N/A ");
		std::wstring extension = boost::to_upper_copy(path.extension());
		if(extension == TEXT(".TGA") || extension == TEXT(".COL") || extension == L".PNG" || extension == L".JPEG" || extension == L".JPG" ||
			extension == L"GIF" || extension == L"BMP")
			clipttype = TEXT(" STILL ");
		else if(extension == TEXT(".WAV") || extension == TEXT(".MP3"))
			clipttype = TEXT(" AUDIO ");
		else if(extension == TEXT(".SWF") || extension == TEXT(".CT") || 
			    extension == TEXT(".DV") || extension == TEXT(".MOV") || 
				extension == TEXT(".MPG") || extension == TEXT(".AVI") || 
				extension == TEXT(".MP4") || extension == TEXT(".FLV") || 
				extension == TEXT(".STGA") || 
				caspar::ffmpeg::is_valid_file(path.file_string()))
			clipttype = TEXT(" MOVIE ");

		if(clipttype != TEXT(" N/A "))
		{		
			auto is_not_digit = [](char c){ return std::isdigit(c) == 0; };

			auto relativePath = boost::filesystem::wpath(path.file_string().substr(env::media_folder().size()-1, path.file_string().size()));

			auto writeTimeStr = boost::posix_time::to_iso_string(boost::posix_time::from_time_t(boost::filesystem::last_write_time(path)));
			writeTimeStr.erase(std::remove_if(writeTimeStr.begin(), writeTimeStr.end(), is_not_digit), writeTimeStr.end());
			auto writeTimeWStr = std::wstring(writeTimeStr.begin(), writeTimeStr.end());

			auto sizeStr = boost::lexical_cast<std::wstring>(boost::filesystem::file_size(path));
			sizeStr.erase(std::remove_if(sizeStr.begin(), sizeStr.end(), is_not_digit), sizeStr.end());
			auto sizeWStr = std::wstring(sizeStr.begin(), sizeStr.end());
				
			auto str = relativePath.replace_extension(TEXT("")).external_file_string();
			if(str[0] == '\\' || str[0] == '/')
				str = std::wstring(str.begin() + 1, str.end());

			return std::wstring() + TEXT("\"") + str +
					+ TEXT("\" ") + clipttype +
					+ TEXT(" ") + sizeStr +
					+ TEXT(" ") + writeTimeWStr +
					+ TEXT("\r\n"); 	
		}	
	}
	return L"";
}

std::wstring ListMedia()
{	
	std::wstringstream replyString;
	for (boost::filesystem::wrecursive_directory_iterator itr(env::media_folder()), end; itr != end; ++itr)	
		replyString << MediaInfo(itr->path());
	
	return boost::to_upper_copy(replyString.str());
}

std::wstring ListTemplates() 
{
	std::wstringstream replyString;

	for (boost::filesystem::wrecursive_directory_iterator itr(env::template_folder()), end; itr != end; ++itr)
	{		
		if(boost::filesystem::is_regular_file(itr->path()) && (itr->path().extension() == L".ft" || itr->path().extension() == L".ct"))
		{
			auto relativePath = boost::filesystem::wpath(itr->path().file_string().substr(env::template_folder().size()-1, itr->path().file_string().size()));

			auto writeTimeStr = boost::posix_time::to_iso_string(boost::posix_time::from_time_t(boost::filesystem::last_write_time(itr->path())));
			writeTimeStr.erase(std::remove_if(writeTimeStr.begin(), writeTimeStr.end(), [](char c){ return std::isdigit(c) == 0;}), writeTimeStr.end());
			auto writeTimeWStr = std::wstring(writeTimeStr.begin(), writeTimeStr.end());

			auto sizeStr = boost::lexical_cast<std::string>(boost::filesystem::file_size(itr->path()));
			sizeStr.erase(std::remove_if(sizeStr.begin(), sizeStr.end(), [](char c){ return std::isdigit(c) == 0;}), sizeStr.end());

			auto sizeWStr = std::wstring(sizeStr.begin(), sizeStr.end());

			std::wstring dir = relativePath.parent_path().external_directory_string();
			std::wstring file = boost::to_upper_copy(relativePath.filename());
			relativePath = boost::filesystem::wpath(dir + L"/" + file);
						
			auto str = relativePath.replace_extension(TEXT("")).external_file_string();
			boost::trim_if(str, boost::is_any_of("\\/"));

			replyString << TEXT("\"") << str
						<< TEXT("\" ") << sizeWStr
						<< TEXT(" ") << writeTimeWStr
						<< TEXT("\r\n");		
		}
	}
	return replyString.str();
}

namespace amcp {
	
AMCPCommand::AMCPCommand() : channelIndex_(0), scheduling_(Default), layerIndex_(-1)
{}

void AMCPCommand::SendReply()
{
	if(!pClientInfo_) 
		return;

	if(replyString_.empty())
		return;
	pClientInfo_->Send(replyString_);
}

void AMCPCommand::Clear() 
{
	pChannel_->stage()->clear();
	pClientInfo_.reset();
	channelIndex_ = 0;
	_parameters.clear();
}

bool DiagnosticsCommand::DoExecute()
{	
	try
	{
		diagnostics::show_graphs(true);

		SetReplyString(TEXT("202 DIAG OK\r\n"));

		return true;
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(TEXT("502 DIAG FAILED\r\n"));
		return false;
	}
}

bool ChannelGridCommand::DoExecute()
{
	int index = 1;
	auto self = GetChannels().back();
	
	core::parameters params;
	params.push_back(L"SCREEN");
	params.push_back(L"NAME");
	params.push_back(L"Channel Grid Window");
	auto screen = create_consumer(params);

	self->output()->add(screen);

	BOOST_FOREACH(auto channel, GetChannels())
	{
		if(channel != self)
		{
			auto producer = create_channel_producer(self->mixer(), channel);		
			self->stage()->load(index, producer, false);
			self->stage()->play(index);
			index++;
		}
	}

	int n = GetChannels().size()-1;
	double delta = 1.0/static_cast<double>(n);
	for(int x = 0; x < n; ++x)
	{
		for(int y = 0; y < n; ++y)
		{
			int index = x+y*n+1;
			auto transform = [=](frame_transform transform) -> frame_transform
			{		
				transform.fill_translation[0]	= x*delta;
				transform.fill_translation[1]	= y*delta;
				transform.fill_scale[0]			= delta;
				transform.fill_scale[1]			= delta;
				transform.clip_translation[0]	= x*delta;
				transform.clip_translation[1]	= y*delta;
				transform.clip_scale[0]			= delta;
				transform.clip_scale[1]			= delta;			
				return transform;
			};
			self->stage()->apply_transform(index, transform);
		}
	}

	return true;
}

bool CallCommand::DoExecute()
{	
	//Perform loading of the clip
	try
	{
		auto what = _parameters.at(0);
				
		boost::unique_future<std::wstring> result;
		auto& params_orig = _parameters.get_original();
		if(what == L"B" || what == L"F")
		{
			std::wstring param;
			for(auto it = std::begin(params_orig)+1; it != std::end(params_orig); ++it, param += L" ")
				param += *it;
			result = GetChannel()->stage()->call(GetLayerIndex(), what == L"F", boost::trim_copy(param));
		}
		else
		{
			std::wstring param;
			for(auto it = std::begin(params_orig); it != std::end(params_orig); ++it, param += L" ")
				param += *it;
			result = GetChannel()->stage()->call(GetLayerIndex(), true, boost::trim_copy(param));
		}

		if(!result.timed_wait(boost::posix_time::seconds(2)))
			BOOST_THROW_EXCEPTION(timed_out());
				
		std::wstringstream replyString;
		if(result.get().empty())
			replyString << TEXT("202 CALL OK\r\n");
		else
			replyString << TEXT("201 CALL OK\r\n") << result.get() << L"\r\n";
		
		SetReplyString(replyString.str());

		return true;
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(TEXT("502 CALL FAILED\r\n"));
		return false;
	}
}

// UGLY HACK
tbb::concurrent_unordered_map<int, std::vector<stage::transform_tuple_t>> deferred_transforms;

core::frame_transform MixerCommand::get_current_transform()
{
	return GetChannel()->stage()->get_current_transform(GetLayerIndex());
}

bool MixerCommand::DoExecute()
{
	using boost::lexical_cast;
	//Perform loading of the clip
	try
	{	
		bool defer = _parameters.back() == L"DEFER";
		if(defer)
			_parameters.pop_back();

		std::vector<stage::transform_tuple_t> transforms;

		if(_parameters[0] == L"KEYER" || _parameters[0] == L"IS_KEY")
		{
			if (_parameters.size() == 1)
				return reply_value([](const frame_transform& t) { return t.is_key ? 1 : 0; });

			bool value = boost::lexical_cast<int>(_parameters.at(1));
			transforms.push_back(stage::transform_tuple_t(GetLayerIndex(), [=](frame_transform transform) -> frame_transform
			{
				transform.is_key = value;
				return transform;					
			}, 0, L"linear"));
		}
		else if(_parameters[0] == L"OPACITY")
		{
			if (_parameters.size() == 1)
				return reply_value([](const frame_transform& t) { return t.opacity; });

			int duration = _parameters.size() > 2 ? boost::lexical_cast<int>(_parameters[2]) : 0;
			std::wstring tween = _parameters.size() > 3 ? _parameters[3] : L"linear";

			double value = boost::lexical_cast<double>(_parameters.at(1));
			
			transforms.push_back(stage::transform_tuple_t(GetLayerIndex(), [=](frame_transform transform) -> frame_transform
			{
				transform.opacity = value;
				return transform;					
			}, duration, tween));
		}
		else if(_parameters[0] == L"FILL" || _parameters[0] == L"FILL_RECT")
		{
			if (_parameters.size() == 1)
			{
				auto transform = get_current_transform();
				auto translation = transform.fill_translation;
				auto scale = transform.fill_scale;
				SetReplyString(
						L"201 MIXER OK\r\n" 
						+ lexical_cast<std::wstring>(translation[0]) + L" "
						+ lexical_cast<std::wstring>(translation[1]) + L" "
						+ lexical_cast<std::wstring>(scale[0]) + L" "
						+ lexical_cast<std::wstring>(scale[1]) + L"\r\n");
				return true;
			}

			int duration = _parameters.size() > 5 ? boost::lexical_cast<int>(_parameters[5]) : 0;
			std::wstring tween = _parameters.size() > 6 ? _parameters[6] : L"linear";
			double x	= boost::lexical_cast<double>(_parameters.at(1));
			double y	= boost::lexical_cast<double>(_parameters.at(2));
			double x_s	= boost::lexical_cast<double>(_parameters.at(3));
			double y_s	= boost::lexical_cast<double>(_parameters.at(4));

			transforms.push_back(stage::transform_tuple_t(GetLayerIndex(), [=](frame_transform transform) mutable -> frame_transform
			{
				transform.fill_translation[0]	= x;
				transform.fill_translation[1]	= y;
				transform.fill_scale[0]			= x_s;
				transform.fill_scale[1]			= y_s;
				return transform;
			}, duration, tween));
		}
		else if(_parameters[0] == L"CLIP" || _parameters[0] == L"CLIP_RECT")
		{
			if (_parameters.size() == 1)
			{
				auto transform = get_current_transform();
				auto translation = transform.clip_translation;
				auto scale = transform.clip_scale;
				SetReplyString(
						L"201 MIXER OK\r\n" 
						+ lexical_cast<std::wstring>(translation[0]) + L" "
						+ lexical_cast<std::wstring>(translation[1]) + L" "
						+ lexical_cast<std::wstring>(scale[0]) + L" "
						+ lexical_cast<std::wstring>(scale[1]) + L"\r\n");
				return true;
			}

			int duration = _parameters.size() > 5 ? boost::lexical_cast<int>(_parameters[5]) : 0;
			std::wstring tween = _parameters.size() > 6 ? _parameters[6] : L"linear";
			double x	= boost::lexical_cast<double>(_parameters.at(1));
			double y	= boost::lexical_cast<double>(_parameters.at(2));
			double x_s	= boost::lexical_cast<double>(_parameters.at(3));
			double y_s	= boost::lexical_cast<double>(_parameters.at(4));
			if(x_s < 0 || y_s < 0)
			{
				SetReplyString(L"403 MIXER ERROR\r\n");
				return false;
			}

			transforms.push_back(stage::transform_tuple_t(GetLayerIndex(), [=](frame_transform transform) -> frame_transform
			{
				transform.clip_translation[0]	= x;
				transform.clip_translation[1]	= y;
				transform.clip_scale[0]			= x_s;
				transform.clip_scale[1]			= y_s;
				return transform;
			}, duration, tween));
		}
		else if(_parameters[0] == L"GRID")
		{
			int duration = _parameters.size() > 2 ? boost::lexical_cast<int>(_parameters[2]) : 0;
			std::wstring tween = _parameters.size() > 3 ? _parameters[3] : L"linear";
			int n = boost::lexical_cast<int>(_parameters.at(1));
			double delta = 1.0/static_cast<double>(n);
			for(int x = 0; x < n; ++x)
			{
				for(int y = 0; y < n; ++y)
				{
					int index = x+y*n+1;
					transforms.push_back(stage::transform_tuple_t(index, [=](frame_transform transform) -> frame_transform
					{		
						transform.fill_translation[0]	= x*delta;
						transform.fill_translation[1]	= y*delta;
						transform.fill_scale[0]			= delta;
						transform.fill_scale[1]			= delta;
						transform.clip_translation[0]	= x*delta;
						transform.clip_translation[1]	= y*delta;
						transform.clip_scale[0]			= delta;
						transform.clip_scale[1]			= delta;			
						return transform;
					}, duration, tween));
				}
			}
		}
		else if(_parameters[0] == L"BLEND")
		{
			if (_parameters.size() == 1)
			{
				auto blend_mode = GetChannel()->mixer()->get_blend_mode(GetLayerIndex());
				SetReplyString(L"201 MIXER OK\r\n" 
					+ lexical_cast<std::wstring>(get_blend_mode(blend_mode)) 
					+ L"\r\n");
				return true;
			}

			auto blend_str = _parameters.at(1);								
			int layer = GetLayerIndex();
			blend_mode::type && blend = get_blend_mode(blend_str);
			GetChannel()->mixer()->set_blend_mode(GetLayerIndex(), blend);	
		}
        else if(_parameters[0] == L"CHROMA")
        {
			if (_parameters.size() == 1)
			{
				auto chroma = GetChannel()->mixer()->get_chroma(GetLayerIndex());
				SetReplyString(L"201 MIXER OK\r\n" 
					+ get_chroma_mode(chroma.key)
					+ (chroma.key == chroma::none
						? L""
						: L" "
						+ lexical_cast<std::wstring>(chroma.threshold) + L" "
						+ lexical_cast<std::wstring>(chroma.softness))
					+ L"\r\n");
					// Add the rest when they are actually used and documented
				return true;
			}

			int layer = GetLayerIndex();
            chroma  chroma;
            chroma.key = get_chroma_mode(_parameters[1]);

			if (chroma.key != chroma::none)
			{
				chroma.threshold    = boost::lexical_cast<double>(_parameters[2]);
				chroma.softness     = boost::lexical_cast<double>(_parameters[3]);
				chroma.spill        = _parameters.size() > 4 ? boost::lexical_cast<double>(_parameters[4]) : 0.0f;
				chroma.blur         = _parameters.size() > 5 ? boost::lexical_cast<double>(_parameters[5]) : 0.0f;
				chroma.show_mask    = _parameters.size() > 6 ? bool(boost::lexical_cast<int>(_parameters[6])) : false;
			}

            GetChannel()->mixer()->set_chroma(GetLayerIndex(), chroma);
        }
		else if(_parameters[0] == L"MASTERVOLUME")
		{
			if (_parameters.size() == 1)
			{
				auto volume = GetChannel()->mixer()->get_master_volume();
				SetReplyString(L"201 MIXER OK\r\n" 
					+ lexical_cast<std::wstring>(volume) + L"\r\n");
				return true;
			}

			float master_volume = boost::lexical_cast<float>(_parameters.at(1));
			GetChannel()->mixer()->set_master_volume(master_volume);
		}
		else if(_parameters[0] == L"BRIGHTNESS")
		{
			if (_parameters.size() == 1)
				return reply_value([](const frame_transform& t) { return t.brightness; });

			auto value = boost::lexical_cast<double>(_parameters.at(1));
			int duration = _parameters.size() > 2 ? boost::lexical_cast<int>(_parameters[2]) : 0;
			std::wstring tween = _parameters.size() > 3 ? _parameters[3] : L"linear";
			transforms.push_back(stage::transform_tuple_t(GetLayerIndex(), [=](frame_transform transform) -> frame_transform
			{
				transform.brightness = value;
				return transform;
			}, duration, tween));
		}
		else if(_parameters[0] == L"SATURATION")
		{
			if (_parameters.size() == 1)
				return reply_value([](const frame_transform& t) { return t.saturation; });

			auto value = boost::lexical_cast<double>(_parameters.at(1));
			int duration = _parameters.size() > 2 ? boost::lexical_cast<int>(_parameters[2]) : 0;
			std::wstring tween = _parameters.size() > 3 ? _parameters[3] : L"linear";
			transforms.push_back(stage::transform_tuple_t(GetLayerIndex(), [=](frame_transform transform) -> frame_transform
			{
				transform.saturation = value;
				return transform;
			}, duration, tween));	
		}
		else if(_parameters[0] == L"CONTRAST")
		{
			if (_parameters.size() == 1)
				return reply_value([](const frame_transform& t) { return t.contrast; });

			auto value = boost::lexical_cast<double>(_parameters.at(1));
			int duration = _parameters.size() > 2 ? boost::lexical_cast<int>(_parameters[2]) : 0;
			std::wstring tween = _parameters.size() > 3 ? _parameters[3] : L"linear";
			transforms.push_back(stage::transform_tuple_t(GetLayerIndex(), [=](frame_transform transform) -> frame_transform
			{
				transform.contrast = value;
				return transform;
			}, duration, tween));	
		}
		else if(_parameters[0] == L"LEVELS")
		{
			if (_parameters.size() == 1)
			{
				auto levels = get_current_transform().levels;
				SetReplyString(L"201 MIXER OK\r\n"
					+ lexical_cast<std::wstring>(levels.min_input) + L" "
					+ lexical_cast<std::wstring>(levels.max_input) + L" "
					+ lexical_cast<std::wstring>(levels.gamma) + L" "
					+ lexical_cast<std::wstring>(levels.min_output) + L" "
					+ lexical_cast<std::wstring>(levels.max_output) + L"\r\n");
				return true;
			}

			levels value;
			value.min_input  = boost::lexical_cast<double>(_parameters.at(1));
			value.max_input  = boost::lexical_cast<double>(_parameters.at(2));
			value.gamma		 = boost::lexical_cast<double>(_parameters.at(3));
			value.min_output = boost::lexical_cast<double>(_parameters.at(4));
			value.max_output = boost::lexical_cast<double>(_parameters.at(5));
			int duration = _parameters.size() > 6 ? boost::lexical_cast<int>(_parameters[6]) : 0;
			std::wstring tween = _parameters.size() > 7 ? _parameters[7] : L"linear";

			transforms.push_back(stage::transform_tuple_t(GetLayerIndex(), [=](frame_transform transform) -> frame_transform
			{
				transform.levels = value;
				return transform;
			}, duration, tween));
		}
		else if(_parameters[0] == L"VOLUME")
		{
			if (_parameters.size() == 1)
				return reply_value([](const frame_transform& t) { return t.volume; });

			int duration = _parameters.size() > 2 ? boost::lexical_cast<int>(_parameters[2]) : 0;
			std::wstring tween = _parameters.size() > 3 ? _parameters[3] : L"linear";
			double value = boost::lexical_cast<double>(_parameters[1]);

			transforms.push_back(stage::transform_tuple_t(GetLayerIndex(), [=](frame_transform transform) -> frame_transform
			{
				transform.volume = value;
				return transform;
			}, duration, tween));
		}
		else if(_parameters[0] == L"CLEAR")
		{
			int layer = GetLayerIndex(std::numeric_limits<int>::max());
			if (layer == std::numeric_limits<int>::max())
			{
				GetChannel()->stage()->clear_transforms();
				GetChannel()->mixer()->clear_blend_modes();
			}
			else
			{
				GetChannel()->stage()->clear_transforms(layer);
				GetChannel()->mixer()->clear_blend_mode(layer);
			}
		}
		else if(_parameters[0] == L"COMMIT")
		{
			transforms = std::move(deferred_transforms[GetChannelIndex()]);
		}
		else
		{
			SetReplyString(TEXT("404 MIXER ERROR\r\n"));
			return false;
		}

		if(defer)
		{
			auto& defer_tranforms = deferred_transforms[GetChannelIndex()];
			defer_tranforms.insert(defer_tranforms.end(), transforms.begin(), transforms.end());
		}
		else
			GetChannel()->stage()->apply_transforms(transforms);
	
		SetReplyString(TEXT("202 MIXER OK\r\n"));

		return true;
	}
	catch(file_not_found&)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(TEXT("404 MIXER ERROR\r\n"));
		return false;
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(TEXT("502 MIXER FAILED\r\n"));
		return false;
	}
}

bool SwapCommand::DoExecute()
{	
	//Perform loading of the clip
	try
	{
		if(GetLayerIndex(-1) != -1)
		{
			std::vector<std::string> strs;
			boost::split(strs, _parameters[0], boost::is_any_of("-"));
			
			auto ch1 = GetChannel();
			auto ch2 = GetChannels().at(boost::lexical_cast<int>(strs.at(0))-1);

			int l1 = GetLayerIndex();
			int l2 = boost::lexical_cast<int>(strs.at(1));

			ch1->stage()->swap_layer(l1, l2, ch2->stage());
		}
		else
		{
			auto ch1 = GetChannel();
			auto ch2 = GetChannels().at(boost::lexical_cast<int>(_parameters[0])-1);
			ch1->stage()->swap_layers(ch2->stage());
		}
		
		SetReplyString(TEXT("202 SWAP OK\r\n"));

		return true;
	}
	catch(file_not_found&)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(TEXT("404 SWAP ERROR\r\n"));
		return false;
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(TEXT("502 SWAP FAILED\r\n"));
		return false;
	}
}

bool AddCommand::DoExecute()
{	
	//Perform loading of the clip
	try
	{
		auto consumer = create_consumer(_parameters);
		GetChannel()->output()->add(GetLayerIndex(consumer->index()), consumer);
	
		SetReplyString(TEXT("202 ADD OK\r\n"));

		return true;
	}
	catch(file_not_found&)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(TEXT("404 ADD ERROR\r\n"));
		return false;
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(TEXT("502 ADD FAILED\r\n"));
		return false;
	}
}

bool RemoveCommand::DoExecute()
{	
	//Perform loading of the clip
	try
	{
		auto index = GetLayerIndex(std::numeric_limits<int>::min());
		if(index == std::numeric_limits<int>::min())
			index = create_consumer(_parameters)->index();

		GetChannel()->output()->remove(index);

		SetReplyString(TEXT("202 REMOVE OK\r\n"));

		return true;
	}
	catch(file_not_found&)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(TEXT("404 REMOVE ERROR\r\n"));
		return false;
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(TEXT("502 REMOVE FAILED\r\n"));
		return false;
	}
}

bool LoadCommand::DoExecute()
{	
	//Perform loading of the clip
	try
	{
		auto pFP = create_producer(GetChannel()->mixer(), _parameters);		
		GetChannel()->stage()->load(GetLayerIndex(), pFP, true);
	
		SetReplyString(TEXT("202 LOAD OK\r\n"));

		return true;
	}
	catch(file_not_found&)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(TEXT("404 LOAD ERROR\r\n"));
		return false;
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(TEXT("502 LOAD FAILED\r\n"));
		return false;
	}
}



//std::function<std::wstring()> channel_cg_add_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
//{
//	static boost::wregex expr(L"^CG\\s(?<video_channel>\\d+)-?(?<LAYER>\\d+)?\\sADD\\s(?<FLASH_LAYER>\\d+)\\s(?<TEMPLATE>\\S+)\\s?(?<START_LABEL>\\S\\S+)?\\s?(?<PLAY_ON_LOAD>\\d)?\\s?(?<DATA>.*)?");
//
//	boost::wsmatch what;
//	if(!boost::regex_match(message, what, expr))
//		return nullptr;
//
//	auto info = channel_info::parse(what, channels);
//
//	int flash_layer_index = boost::lexical_cast<int>(what["FLASH_LAYER"].str());
//
//	std::wstring templatename = what["TEMPLATE"].str();
//	bool play_on_load = what["PLAY_ON_LOAD"].matched ? what["PLAY_ON_LOAD"].str() != L"0" : 0;
//	std::wstring start_label = what["START_LABEL"].str();	
//	std::wstring data = get_data(what["DATA"].str());
//	
//	boost::replace_all(templatename, "\"", "");
//
//	return [=]() -> std::wstring
//	{	
//		std::wstring fullFilename = flash::flash_producer::find_template(server::template_folder() + templatename);
//		if(fullFilename.empty())
//			BOOST_THROW_EXCEPTION(file_not_found());
//	
//		std::wstring extension = boost::filesystem::wpath(fullFilename).extension();
//		std::wstring filename = templatename;
//		filename.append(extension);
//
//		flash::flash::get_default_cg_producer(info.video_channel, std::max<int>(DEFAULT_CHANNEL_LAYER+1, info.layer_index))
//			->add(flash_layer_index, filename, play_on_load, start_label, data);
//
//		CASPAR_LOG(info) << L"Executed [amcp_channel_cg_add]";
//		return L"";
//	};

bool LoadbgCommand::DoExecute()
{
	transition_info transitionInfo;
	
	bool bLoop = false;

	// TRANSITION

	std::wstring message;
	for(size_t n = 0; n < _parameters.size(); ++n)
		message += _parameters[n] + L" ";
		
	static const boost::wregex expr(L".*(?<TRANSITION>CUT|PUSH|SLIDE|WIPE|MIX)\\s*(?<DURATION>\\d+)\\s*(?<TWEEN>(LINEAR)|(EASE[^\\s]*))?\\s*(?<DIRECTION>FROMLEFT|FROMRIGHT|LEFT|RIGHT)?.*");
	boost::wsmatch what;
	if(boost::regex_match(message, what, expr))
	{
		auto transition = what["TRANSITION"].str();
		transitionInfo.duration = lexical_cast_or_default<size_t>(what["DURATION"].str());
		auto direction = what["DIRECTION"].matched ? what["DIRECTION"].str() : L"";
		auto tween = what["TWEEN"].matched ? what["TWEEN"].str() : L"";
		transitionInfo.tweener = get_tweener(tween);		

		if(transition == TEXT("CUT"))
			transitionInfo.type = transition::cut;
		else if(transition == TEXT("MIX"))
			transitionInfo.type = transition::mix;
		else if(transition == TEXT("PUSH"))
			transitionInfo.type = transition::push;
		else if(transition == TEXT("SLIDE"))
			transitionInfo.type = transition::slide;
		else if(transition == TEXT("WIPE"))
			transitionInfo.type = transition::wipe;
		
		if(direction == TEXT("FROMLEFT"))
			transitionInfo.direction = transition_direction::from_left;
		else if(direction == TEXT("FROMRIGHT"))
			transitionInfo.direction = transition_direction::from_right;
		else if(direction == TEXT("LEFT"))
			transitionInfo.direction = transition_direction::from_right;
		else if(direction == TEXT("RIGHT"))
			transitionInfo.direction = transition_direction::from_left;
	}
	
	//Perform loading of the clip
	try
	{
		auto pFP = create_producer(GetChannel()->mixer(), _parameters);
		if(pFP == frame_producer::empty())
			BOOST_THROW_EXCEPTION(file_not_found() << msg_info(_parameters.size() > 0 ? narrow(_parameters[0]) : ""));

		bool auto_play = std::find(_parameters.begin(), _parameters.end(), L"AUTO") != _parameters.end();

		auto pFP2 = create_transition_producer(GetChannel()->get_video_format_desc().field_mode, pFP, transitionInfo);
		GetChannel()->stage()->load(GetLayerIndex(), pFP2, false, auto_play ? transitionInfo.duration : -1); // TODO: LOOP
	
		SetReplyString(TEXT("202 LOADBG OK\r\n"));

		return true;
	}
	catch(file_not_found&)
	{		
		CASPAR_LOG(error) << L"File not found. No match found for parameters. Check syntax:" << _parameters.get_original_string();
		SetReplyString(TEXT("404 LOADBG ERROR\r\n"));
		return false;
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		SetReplyString(TEXT("502 LOADBG FAILED\r\n"));
		return false;
	}
}

bool PauseCommand::DoExecute()
{
	try
	{
		GetChannel()->stage()->pause(GetLayerIndex());
		SetReplyString(TEXT("202 PAUSE OK\r\n"));
		return true;
	}
	catch(...)
	{
		SetReplyString(TEXT("501 PAUSE FAILED\r\n"));
	}

	return false;
}

bool PlayCommand::DoExecute()
{
	try
	{
		if(!_parameters.empty())
		{
			LoadbgCommand lbg;
			lbg.SetChannel(GetChannel());
			lbg.SetChannelIndex(GetChannelIndex());
			lbg.SetLayerIntex(GetLayerIndex());
			lbg.SetClientInfo(GetClientInfo());
			lbg.SetParameters(_parameters);
			if(!lbg.Execute())
				throw std::exception();
		}

		GetChannel()->stage()->play(GetLayerIndex());
		
		SetReplyString(TEXT("202 PLAY OK\r\n"));
		return true;
	}
	catch(...)
	{
		SetReplyString(TEXT("501 PLAY FAILED\r\n"));
	}

	return false;
}

bool StopCommand::DoExecute()
{
	try
	{
		GetChannel()->stage()->stop(GetLayerIndex());
		SetReplyString(TEXT("202 STOP OK\r\n"));
		return true;
	}
	catch(...)
	{
		SetReplyString(TEXT("501 STOP FAILED\r\n"));
	}

	return false;
}

bool ClearCommand::DoExecute()
{
	int index = GetLayerIndex(std::numeric_limits<int>::min());
	if(index != std::numeric_limits<int>::min())
		GetChannel()->stage()->clear(index);
	else
		GetChannel()->stage()->clear();
		
	SetReplyString(TEXT("202 CLEAR OK\r\n"));

	return true;
}

bool PrintCommand::DoExecute()
{
	parameters params;
	params.push_back(L"IMAGE");
	GetChannel()->output()->add(create_consumer(params));
		
	SetReplyString(TEXT("202 PRINT OK\r\n"));

	return true;
}

bool LogCommand::DoExecute()
{
	if(_parameters.at(0) == L"LEVEL")
		log::set_log_level(_parameters.at(1));

	SetReplyString(TEXT("202 LOG OK\r\n"));

	return true;
}

bool CGCommand::DoExecute()
{
	try
	{
		std::wstring command = _parameters[0];
		if(command == TEXT("ADD"))
			return DoExecuteAdd();
		else if(command == TEXT("PLAY"))
			return DoExecutePlay();
		else if(command == TEXT("STOP"))
			return DoExecuteStop();
		else if(command == TEXT("NEXT"))
			return DoExecuteNext();
		else if(command == TEXT("REMOVE"))
			return DoExecuteRemove();
		else if(command == TEXT("CLEAR"))
			return DoExecuteClear();
		else if(command == TEXT("UPDATE"))
			return DoExecuteUpdate();
		else if(command == TEXT("INVOKE"))
			return DoExecuteInvoke();
		else if(command == TEXT("INFO"))
			return DoExecuteInfo();
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
	}

	SetReplyString(TEXT("403 CG ERROR\r\n"));
	return false;
}

bool CGCommand::ValidateLayer(const std::wstring& layerstring) {
	int length = layerstring.length();
	for(int i = 0; i < length; ++i) {
		if(!_istdigit(layerstring[i])) {
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

	if(_parameters.size() < 4) 
	{
		SetReplyString(TEXT("402 CG ERROR\r\n"));
		return false;
	}
	unsigned int dataIndex = 4;

	if(!ValidateLayer(_parameters[1])) 
	{
		SetReplyString(TEXT("403 CG ERROR\r\n"));
		return false;
	}

	layer = _ttoi(_parameters[1].c_str());

	if(_parameters[3].length() > 1) 
	{	//read label
		label = _parameters.at_original(3);
		++dataIndex;

		if(_parameters.size() > 4 && _parameters[4].length() > 0)	//read play-on-load-flag
			bDoStart = (_parameters[4][0]==TEXT('1')) ? true : false;
		else 
		{
			SetReplyString(TEXT("402 CG ERROR\r\n"));
			return false;
		}
	}
	else if(_parameters[3].length() > 0) {	//read play-on-load-flag
		bDoStart = (_parameters[3][0]==TEXT('1')) ? true : false;
	}
	else 
	{
		SetReplyString(TEXT("403 CG ERROR\r\n"));
		return false;
	}

	const TCHAR* pDataString = 0;
	std::wstringstream data;
	std::wstring dataFromFile;
	if(_parameters.size() > dataIndex) 
	{	//read data
		const std::wstring& dataString = _parameters.at_original(dataIndex);

		if(dataString[0] == TEXT('<')) //the data is an XML-string
			pDataString = dataString.c_str();
		else 
		{
			//The data is not an XML-string, it must be a filename
			std::wstring filename = env::data_folder();
			filename.append(dataString);
			filename.append(TEXT(".ftd"));

			dataFromFile = read_file(boost::filesystem::wpath(filename));
			pDataString = dataFromFile.c_str();
		}
	}

	std::wstring fullFilename = flash::find_template(env::template_folder() + _parameters[2]);
	if(!fullFilename.empty())
	{
		std::wstring extension = boost::filesystem::wpath(fullFilename).extension();
		std::wstring filename = _parameters[2];
		filename.append(extension);

		flash::get_default_cg_producer(safe_ptr<core::video_channel>(GetChannel()), GetLayerIndex(flash::cg_producer::DEFAULT_LAYER))->add(layer, filename, bDoStart, label, (pDataString!=0) ? pDataString : TEXT(""));
		SetReplyString(TEXT("202 CG OK\r\n"));
	}
	else
	{
		CASPAR_LOG(warning) << "Could not find template " << _parameters[2];
		SetReplyString(TEXT("404 CG ERROR\r\n"));
	}
	return true;
}

bool CGCommand::DoExecutePlay()
{
	if(_parameters.size() > 1)
	{
		if(!ValidateLayer(_parameters[1])) 
		{
			SetReplyString(TEXT("403 CG ERROR\r\n"));
			return false;
		}
		int layer = _ttoi(_parameters[1].c_str());
		flash::get_default_cg_producer(safe_ptr<core::video_channel>(GetChannel()), GetLayerIndex(flash::cg_producer::DEFAULT_LAYER))->play(layer);
	}
	else
	{
		SetReplyString(TEXT("402 CG ERROR\r\n"));
		return true;
	}

	SetReplyString(TEXT("202 CG OK\r\n"));
	return true;
}

bool CGCommand::DoExecuteStop() 
{
	if(_parameters.size() > 1)
	{
		if(!ValidateLayer(_parameters[1])) 
		{
			SetReplyString(TEXT("403 CG ERROR\r\n"));
			return false;
		}
		int layer = _ttoi(_parameters[1].c_str());
		flash::get_default_cg_producer(safe_ptr<core::video_channel>(GetChannel()), GetLayerIndex(flash::cg_producer::DEFAULT_LAYER))->stop(layer, 0);
	}
	else 
	{
		SetReplyString(TEXT("402 CG ERROR\r\n"));
		return true;
	}

	SetReplyString(TEXT("202 CG OK\r\n"));
	return true;
}

bool CGCommand::DoExecuteNext()
{
	if(_parameters.size() > 1) 
	{
		if(!ValidateLayer(_parameters[1])) 
		{
			SetReplyString(TEXT("403 CG ERROR\r\n"));
			return false;
		}

		int layer = _ttoi(_parameters[1].c_str());
		flash::get_default_cg_producer(safe_ptr<core::video_channel>(GetChannel()), GetLayerIndex(flash::cg_producer::DEFAULT_LAYER))->next(layer);
	}
	else 
	{
		SetReplyString(TEXT("402 CG ERROR\r\n"));
		return true;
	}

	SetReplyString(TEXT("202 CG OK\r\n"));
	return true;
}

bool CGCommand::DoExecuteRemove() 
{
	if(_parameters.size() > 1) 
	{
		if(!ValidateLayer(_parameters[1])) 
		{
			SetReplyString(TEXT("403 CG ERROR\r\n"));
			return false;
		}

		int layer = _ttoi(_parameters[1].c_str());
		flash::get_default_cg_producer(safe_ptr<core::video_channel>(GetChannel()), GetLayerIndex(flash::cg_producer::DEFAULT_LAYER))->remove(layer);
	}
	else 
	{
		SetReplyString(TEXT("402 CG ERROR\r\n"));
		return true;
	}

	SetReplyString(TEXT("202 CG OK\r\n"));
	return true;
}

bool CGCommand::DoExecuteClear() 
{
	GetChannel()->stage()->clear(GetLayerIndex(flash::cg_producer::DEFAULT_LAYER));
	SetReplyString(TEXT("202 CG OK\r\n"));
	return true;
}

bool CGCommand::DoExecuteUpdate() 
{
	try
	{
		if(!ValidateLayer(_parameters.at(1)))
		{
			SetReplyString(TEXT("403 CG ERROR\r\n"));
			return false;
		}
						
		std::wstring dataString = _parameters.at_original(2);
		if(dataString.at(0) != TEXT('<'))
		{
			//The data is not an XML-string, it must be a filename
			std::wstring filename = env::data_folder();
			filename.append(dataString);
			filename.append(TEXT(".ftd"));

			dataString = read_file(boost::filesystem::wpath(filename));
		}		

		int layer = _ttoi(_parameters.at(1).c_str());
		flash::get_default_cg_producer(safe_ptr<core::video_channel>(GetChannel()), GetLayerIndex(flash::cg_producer::DEFAULT_LAYER))->update(layer, dataString);
	}
	catch(...)
	{
		SetReplyString(TEXT("402 CG ERROR\r\n"));
		return true;
	}

	SetReplyString(TEXT("202 CG OK\r\n"));
	return true;
}

bool CGCommand::DoExecuteInvoke() 
{
	std::wstringstream replyString;
	replyString << TEXT("201 CG OK\r\n");

	if(_parameters.size() > 2)
	{
		if(!ValidateLayer(_parameters[1]))
		{
			SetReplyString(TEXT("403 CG ERROR\r\n"));
			return false;
		}
		int layer = _ttoi(_parameters[1].c_str());
		auto result = flash::get_default_cg_producer(safe_ptr<core::video_channel>(GetChannel()), GetLayerIndex(flash::cg_producer::DEFAULT_LAYER))->invoke(layer, _parameters.at_original(2));
		replyString << result << TEXT("\r\n"); 
	}
	else 
	{
		SetReplyString(TEXT("402 CG ERROR\r\n"));
		return true;
	}
	
	SetReplyString(replyString.str());
	return true;
}

bool CGCommand::DoExecuteInfo() 
{
	std::wstringstream replyString;
	replyString << TEXT("201 CG OK\r\n");

	if(_parameters.size() > 1)
	{
		if(!ValidateLayer(_parameters[1]))
		{
			SetReplyString(TEXT("403 CG ERROR\r\n"));
			return false;
		}

		int layer = _ttoi(_parameters[1].c_str());
		auto desc = flash::get_default_cg_producer(safe_ptr<core::video_channel>(GetChannel()), GetLayerIndex(flash::cg_producer::DEFAULT_LAYER))->description(layer);
		
		replyString << desc << TEXT("\r\n"); 
	}
	else 
	{
		auto info = flash::get_default_cg_producer(safe_ptr<core::video_channel>(GetChannel()), GetLayerIndex(flash::cg_producer::DEFAULT_LAYER))->template_host_info();
		replyString << info << TEXT("\r\n"); 
	}	

	SetReplyString(replyString.str());
	return true;
}

bool DataCommand::DoExecute()
{
	std::wstring command = _parameters[0];
	if(command == TEXT("STORE"))
		return DoExecuteStore();
	else if(command == TEXT("RETRIEVE"))
		return DoExecuteRetrieve();
	else if(command == TEXT("REMOVE"))
		return DoExecuteRemove();
	else if(command == TEXT("LIST"))
		return DoExecuteList();

	SetReplyString(TEXT("403 DATA ERROR\r\n"));
	return false;
}

bool DataCommand::DoExecuteStore() 
{
	if(_parameters.size() < 3) 
	{
		SetReplyString(TEXT("402 DATA STORE ERROR\r\n"));
		return false;
	}

	std::wstring filename = env::data_folder();
	filename.append(_parameters[1]);
	filename.append(TEXT(".ftd"));

	auto data_path = boost::filesystem::wpath(
			boost::filesystem::wpath(filename).parent_path());
	
	if(!boost::filesystem::exists(data_path))
		boost::filesystem::create_directories(data_path);

	std::wofstream datafile(filename.c_str());

	if(!datafile) 
	{
		SetReplyString(TEXT("501 DATA STORE FAILED\r\n"));
		return false;
	}

	datafile << static_cast<wchar_t>(65279); // UTF-8 BOM character
	datafile << _parameters.at_original(2) << std::flush;
	datafile.close();

	std::wstring replyString = TEXT("202 DATA STORE OK\r\n");
	SetReplyString(replyString);
	return true;
}

bool DataCommand::DoExecuteRetrieve() 
{
	if(_parameters.size() < 2) 
	{
		SetReplyString(TEXT("402 DATA RETRIEVE ERROR\r\n"));
		return false;
	}

	std::wstring filename = env::data_folder();
	filename.append(_parameters[1]);
	filename.append(TEXT(".ftd"));

	std::wstring file_contents = read_file(boost::filesystem::wpath(filename));

	if (file_contents.empty()) 
	{
		SetReplyString(TEXT("404 DATA RETRIEVE ERROR\r\n"));
		return false;
	}

	std::wstringstream reply;
	reply << TEXT("201 DATA RETRIEVE OK\r\n");

	std::wstringstream file_contents_stream(file_contents);
	std::wstring line;
	bool bFirstLine = true;
	
	while(std::getline(file_contents_stream, line))
	{
		if(!bFirstLine)
			reply << "\n";
		else
			bFirstLine = false;

		reply << line;
	}

	reply << "\r\n";
	SetReplyString(reply.str());
	return true;
}

bool DataCommand::DoExecuteRemove() 
{ 
	if (_parameters.size() < 2) 
	{
		SetReplyString(TEXT("402 DATA REMOVE ERROR\r\n"));
		return false;
	}

	std::wstring filename = env::data_folder();
	filename.append(_parameters[1]);
	filename.append(TEXT(".ftd"));

	if (!boost::filesystem::exists(filename)) 
	{
		SetReplyString(TEXT("404 DATA REMOVE ERROR\r\n"));
		return false;
	}

	if (!boost::filesystem::remove(filename))
	{
		SetReplyString(TEXT("403 DATA REMOVE ERROR\r\n"));
		return false;
	}

	SetReplyString(TEXT("201 DATA REMOVE OK\r\n"));

	return true;
}

bool DataCommand::DoExecuteList() 
{
	std::wstringstream replyString;
	replyString << TEXT("200 DATA LIST OK\r\n");

	for (boost::filesystem::wrecursive_directory_iterator itr(env::data_folder()), end; itr != end; ++itr)
	{			
		if(boost::filesystem::is_regular_file(itr->path()))
		{
			if(!boost::iequals(itr->path().extension(), L".ftd"))
				continue;
			
			auto relativePath = boost::filesystem::wpath(itr->path().file_string().substr(env::data_folder().size()-1, itr->path().file_string().size()));
			
			auto str = relativePath.replace_extension(TEXT("")).external_file_string();
			if(str[0] == '\\' || str[0] == '/')
				str = std::wstring(str.begin() + 1, str.end());

			replyString << str << TEXT("\r\n"); 	
		}
	}
	
	replyString << TEXT("\r\n");

	SetReplyString(boost::to_upper_copy(replyString.str()));
	return true;
}

bool ThumbnailCommand::DoExecute()
{
	std::wstring command = _parameters[0];

	if (command == TEXT("RETRIEVE"))
		return DoExecuteRetrieve();
	else if (command == TEXT("LIST"))
		return DoExecuteList();
	else if (command == TEXT("GENERATE"))
		return DoExecuteGenerate();
	else if (command == TEXT("GENERATE_ALL"))
		return DoExecuteGenerateAll();

	SetReplyString(TEXT("403 THUMBNAIL ERROR\r\n"));
	return false;
}

bool ThumbnailCommand::DoExecuteRetrieve() 
{
	if(_parameters.size() < 2) 
	{
		SetReplyString(TEXT("402 THUMBNAIL RETRIEVE ERROR\r\n"));
		return false;
	}

	std::wstring filename = env::thumbnails_folder();
	filename.append(_parameters[1]);
	filename.append(TEXT(".png"));

	std::wstring file_contents = read_file_base64(boost::filesystem::wpath(filename));

	if (file_contents.empty())
	{
		SetReplyString(TEXT("404 THUMBNAIL RETRIEVE ERROR\r\n"));
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
	replyString << TEXT("200 THUMBNAIL LIST OK\r\n");

	for (boost::filesystem::wrecursive_directory_iterator itr(env::thumbnails_folder()), end; itr != end; ++itr)
	{			
		if(boost::filesystem::is_regular_file(itr->path()))
		{
			if(!boost::iequals(itr->path().extension(), L".png"))
				continue;
			
			auto relativePath = boost::filesystem::wpath(itr->path().file_string().substr(env::thumbnails_folder().size()-1, itr->path().file_string().size()));
			
			auto str = relativePath.replace_extension(L"").external_file_string();
			if(str[0] == '\\' || str[0] == '/')
				str = std::wstring(str.begin() + 1, str.end());

			auto mtime = boost::filesystem::last_write_time(itr->path());
			auto mtime_readable = boost::posix_time::to_iso_string(boost::posix_time::from_time_t(mtime));
			auto file_size = boost::filesystem::file_size(itr->path());

			replyString << L"\"" << str << L"\" " << widen(mtime_readable) << L" " << file_size << L"\r\n";
		}
	}
	
	replyString << TEXT("\r\n");

	SetReplyString(boost::to_upper_copy(replyString.str()));
	return true;
}

bool ThumbnailCommand::DoExecuteGenerate()
{
	if (_parameters.size() < 2) 
	{
		SetReplyString(L"402 THUMBNAIL GENERATE ERROR\r\n");
		return false;
	}

	auto thumb_gen = GetThumbGenerator();

	if (thumb_gen)
	{
		thumb_gen->generate(_parameters[1]);
		SetReplyString(L"200 THUMBNAIL GENERATE OK\r\n");
		return true;
	}
	else
	{
		SetReplyString(L"501 THUMBNAIL GENERATE ERROR\r\n");
		return false;
	}
}

bool ThumbnailCommand::DoExecuteGenerateAll()
{
	auto thumb_gen = GetThumbGenerator();

	if (thumb_gen)
	{
		thumb_gen->generate_all();
		SetReplyString(L"200 THUMBNAIL GENERATE_ALL OK\r\n");
		return true;
	}
	else
	{
		SetReplyString(L"501 THUMBNAIL GENERATE_ALL ERROR\r\n");
		return false;
	}
}

bool CinfCommand::DoExecute()
{
	std::wstringstream replyString;
	
	try
	{
		std::wstring info;
		for (boost::filesystem::wrecursive_directory_iterator itr(env::media_folder()), end; itr != end; ++itr)
		{
			auto path = itr->path();
			auto file = path.replace_extension(L"").filename();
			if(boost::iequals(file, _parameters.at(0)))
				info += MediaInfo(itr->path()) + L"\r\n";
		}

		if(info.empty())
		{
			SetReplyString(TEXT("404 CINF ERROR\r\n"));
			return false;
		}
		replyString << TEXT("200 CINF OK\r\n");
		replyString << info << "\r\n";
	}
	catch(...)
	{
		SetReplyString(TEXT("404 CINF ERROR\r\n"));
		return false;
	}
	
	SetReplyString(replyString.str());
	return true;
}

void GenerateChannelInfo(int index, const safe_ptr<core::video_channel>& pChannel, std::wstringstream& replyString)
{
	replyString << index+1 << TEXT(" ") << pChannel->get_video_format_desc().name << TEXT(" PLAYING") << TEXT("\r\n");
}

bool InfoCommand::DoExecute()
{
	std::wstringstream replyString;
	
	boost::property_tree::xml_writer_settings<wchar_t> w(' ', 3);

	try
	{
		if(_parameters.size() >= 1 && _parameters[0] == L"TEMPLATE")
		{		
			replyString << L"201 INFO TEMPLATE OK\r\n";

			// Needs to be extended for any file, not just flash.

			auto filename = flash::find_template(env::template_folder() + _parameters.at(1));
						
			std::wstringstream str;
			str << widen(flash::read_template_meta_info(filename));
			boost::property_tree::wptree info;
			boost::property_tree::xml_parser::read_xml(str, info, boost::property_tree::xml_parser::trim_whitespace | boost::property_tree::xml_parser::no_comments);

			boost::property_tree::xml_parser::write_xml(replyString, info, w);
		}
		else if(_parameters.size() >= 1 && _parameters[0] == L"CONFIG")
		{		
			replyString << L"201 INFO CONFIG OK\r\n";

			boost::property_tree::write_xml(replyString, caspar::env::properties(), w);
		}
		else if(_parameters.size() >= 1 && _parameters[0] == L"PATHS")
		{
			replyString << L"201 INFO PATHS OK\r\n";

			boost::property_tree::wptree info;
			info.add_child(L"paths", caspar::env::properties().get_child(L"configuration.paths"));
			info.add(L"paths.initial-path", boost::filesystem2::initial_path<boost::filesystem2::wpath>().directory_string() + L"\\");

			boost::property_tree::write_xml(replyString, info, w);
		}
		else if(_parameters.size() >= 1 && _parameters[0] == L"SYSTEM")
		{
			replyString << L"201 INFO SYSTEM OK\r\n";
			
			boost::property_tree::wptree info;
			
			info.add(L"system.name",					caspar::get_system_product_name());
			info.add(L"system.windows.name",			caspar::get_win_product_name());
			info.add(L"system.windows.service-pack",	caspar::get_win_sp_version());
			info.add(L"system.cpu",						caspar::get_cpu_info());
	
			BOOST_FOREACH(auto device, caspar::decklink::get_device_list())
				info.add(L"system.caspar.decklink.device", device);

			BOOST_FOREACH(auto device, caspar::bluefish::get_device_list())
				info.add(L"system.caspar.bluefish.device", device);
				
			info.add(L"system.caspar.flash",					caspar::flash::get_version());
			info.add(L"system.caspar.template-host",			caspar::flash::get_cg_version());
			info.add(L"system.caspar.free-image",				caspar::image::get_version());
			info.add(L"system.caspar.ffmpeg.avcodec",			caspar::ffmpeg::get_avcodec_version());
			info.add(L"system.caspar.ffmpeg.avformat",			caspar::ffmpeg::get_avformat_version());
			info.add(L"system.caspar.ffmpeg.avfilter",			caspar::ffmpeg::get_avfilter_version());
			info.add(L"system.caspar.ffmpeg.avutil",			caspar::ffmpeg::get_avutil_version());
			info.add(L"system.caspar.ffmpeg.swscale",			caspar::ffmpeg::get_swscale_version());
									
			boost::property_tree::write_xml(replyString, info, w);
		}
		else if(_parameters.size() >= 1 && _parameters[0] == L"SERVER")
		{
			replyString << L"201 INFO SERVER OK\r\n";
			
			boost::property_tree::wptree info;

			int index = 0;
			BOOST_FOREACH(auto channel, channels_)
				info.add_child(L"channels.channel", channel->info())
					.add(L"index", ++index);
			
			boost::property_tree::write_xml(replyString, info, w);
		}
		else // channel
		{			
			if(_parameters.size() >= 1)
			{
				replyString << TEXT("201 INFO OK\r\n");
				boost::property_tree::wptree info;

				std::vector<std::wstring> split;
				boost::split(split, _parameters[0], boost::is_any_of("-"));
					
				int layer = std::numeric_limits<int>::min();
				int channel = boost::lexical_cast<int>(split[0]) - 1;

				if(split.size() > 1)
					layer = boost::lexical_cast<int>(split[1]);
				
				if(layer == std::numeric_limits<int>::min())
				{	
					info.add_child(L"channel", channels_.at(channel)->info())
							.add(L"index", channel);
				}
				else
				{
					if(_parameters.size() >= 2)
					{
						if(_parameters[1] == L"B")
							info.add_child(L"producer", channels_.at(channel)->stage()->background(layer).get()->info());
						else
							info.add_child(L"producer", channels_.at(channel)->stage()->foreground(layer).get()->info());
					}
					else
					{
						info.add_child(L"layer", channels_.at(channel)->stage()->info(layer).get())
							.add(L"index", layer);
					}
				}
				boost::property_tree::xml_parser::write_xml(replyString, info, w);
			}
			else
			{
				// This is needed for backwards compatibility with old clients
				replyString << TEXT("200 INFO OK\r\n");
				for(size_t n = 0; n < channels_.size(); ++n)
					GenerateChannelInfo(n, channels_[n], replyString);
			}

		}
	}
	catch(...)
	{
		SetReplyString(TEXT("403 INFO ERROR\r\n"));
		return false;
	}

	replyString << TEXT("\r\n");
	SetReplyString(replyString.str());
	return true;
}

bool ClsCommand::DoExecute()
{
/*
		wav = audio
		mp3 = audio
		swf	= movie
		dv  = movie
		tga = still
		col = still
	*/
	std::wstringstream replyString;
	replyString << TEXT("200 CLS OK\r\n");
	replyString << ListMedia();
	replyString << TEXT("\r\n");
	SetReplyString(boost::to_upper_copy(replyString.str()));
	return true;
}

bool TlsCommand::DoExecute()
{
	std::wstringstream replyString;
	replyString << TEXT("200 TLS OK\r\n");

	replyString << ListTemplates();
	replyString << TEXT("\r\n");

	SetReplyString(replyString.str());
	return true;
}

bool VersionCommand::DoExecute()
{
	std::wstring replyString = TEXT("201 VERSION OK\r\n") + env::version() + TEXT("\r\n");

	if(_parameters.size() > 0)
	{
		if(_parameters[0] == L"FLASH")
			replyString = TEXT("201 VERSION OK\r\n") + flash::get_version() + TEXT("\r\n");
		else if(_parameters[0] == L"TEMPLATEHOST")
			replyString = TEXT("201 VERSION OK\r\n") + flash::get_cg_version() + TEXT("\r\n");
		else if(_parameters[0] != L"SERVER")
			replyString = TEXT("403 VERSION ERROR\r\n");
	}

	SetReplyString(replyString);
	return true;
}

bool ByeCommand::DoExecute()
{
	GetClientInfo()->Disconnect();
	return true;
}

bool SetCommand::DoExecute()
{
	std::wstring name = _parameters[0];
	std::transform(name.begin(), name.end(), name.begin(), toupper);

	std::wstring value = _parameters[1];
	std::transform(value.begin(), value.end(), value.begin(), toupper);

	if(name == TEXT("MODE"))
	{
		auto format_desc = core::video_format_desc::get(value);
		if(format_desc.format != core::video_format::invalid)
		{
			GetChannel()->set_video_format_desc(format_desc);
			SetReplyString(TEXT("202 SET MODE OK\r\n"));
		}
		else
			SetReplyString(TEXT("501 SET MODE FAILED\r\n"));
	}
	else
	{
		this->SetReplyString(TEXT("403 SET ERROR\r\n"));
	}

	return true;
}

bool KillCommand::DoExecute()
{
	GetShutdownServerNow().set_value(false); // False for not attempting to restart.
	return true;
}

bool RestartCommand::DoExecute()
{
	GetShutdownServerNow().set_value(true); // True for attempting to restart
	return true;
}

}	//namespace amcp
}}	//namespace caspar
