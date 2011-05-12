/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include "../StdAfx.h"

#include "AMCPCommandsImpl.h"
#include "AMCPProtocolStrategy.h"

#include <common/env.h>

#include <core/producer/frame_producer.h>
#include <core/video_format.h>
#include <modules/flash/flash.h>
#include <modules/flash/producer/flash_producer.h>
#include <modules/flash/producer/cg_producer.h>
#include <core/producer/transition/transition_producer.h>

#include <core/producer/frame/image_transform.h>
#include <core/producer/frame/audio_transform.h>

#include <algorithm>
#include <locale>
#include <fstream>
#include <cctype>
#include <io.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#if defined(_MSC_VER)
#pragma warning (push, 1) // TODO: Legacy code, just disable warnings
#endif

/* Return codes

100 [action]			Information om att något har hänt  
101 [action]			Information om att något har hänt, en rad data skickas  

202 [kommando] OK		Kommandot har utförts  
201 [kommando] OK		Kommandot har utförts, och en rad data skickas tillbaka  
200 [kommando] OK		Kommandot har utförts, och flera rader data skickas tillbaka. Avslutas med tomrad  

400 ERROR				Kommandot kunde inte förstås  
401 [kommando] ERROR	Ogiltig kanal  
402 [kommando] ERROR	Parameter saknas  
403 [kommando] ERROR	Ogiltig parameter  
404 [kommando] ERROR	Mediafilen hittades inte  

500 FAILED				Internt configurationfel  
501 [kommando] FAILED	Internt configurationfel  
502 [kommando] FAILED	Oläslig mediafil  

600 [kommando] FAILED	funktion ej implementerad
*/

namespace caspar { namespace protocol {

using namespace core;

std::wstring ListMedia()
{	
	std::wstringstream replyString;
	for (boost::filesystem::wrecursive_directory_iterator itr(env::media_folder()), end; itr != end; ++itr)
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

				auto relativePath = boost::filesystem::wpath(itr->path().file_string().substr(env::media_folder().size()-1, itr->path().file_string().size()));

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

std::wstring ListTemplates() 
{
	std::wstringstream replyString;

	for (boost::filesystem::wrecursive_directory_iterator itr(env::template_folder()), end; itr != end; ++itr)
	{		
		if(boost::filesystem::is_regular_file(itr->path()) && itr->path().extension() == L".ft")
		{
			auto relativePath = boost::filesystem::wpath(itr->path().file_string().substr(env::template_folder().size()-1, itr->path().file_string().size()));

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
	pChannel_->producer()->clear();
	pClientInfo_.reset();
	channelIndex_ = 0;
	_parameters.clear();
}

bool MixerCommand::DoExecute()
{	
	//Perform loading of the clip
	try
	{	
		if(_parameters[0] == L"VIDEO")
		{
			if(_parameters[1] == L"IS_KEY")
			{
				bool value = lexical_cast_or_default(_parameters.at(2), false);
				auto transform = [=](image_transform transform) -> image_transform
				{
					transform.set_is_key(value);
					return transform;					
				};

				int layer = GetLayerIndex(std::numeric_limits<int>::min());
				if(layer != std::numeric_limits<int>::min())					
					GetChannel()->mixer()->apply_image_transform(GetLayerIndex(), transform, 0);
				else
					GetChannel()->mixer()->apply_image_transform(transform, 0);
			}
			else if(_parameters[1] == L"OPACITY")
			{
				int duration = _parameters.size() > 2 ? lexical_cast_or_default(_parameters[3], 0) : 0;
				std::wstring tween = _parameters.size() > 3 ? _parameters[4] : L"linear";

				double value = boost::lexical_cast<double>(_parameters.at(2));
			
				auto transform = [=](image_transform transform) -> image_transform
				{
					transform.set_opacity(value);
					return transform;					
				};

				int layer = GetLayerIndex(std::numeric_limits<int>::min());
				if(layer != std::numeric_limits<int>::min())					
					GetChannel()->mixer()->apply_image_transform(GetLayerIndex(), transform, duration, tween);
				else
					GetChannel()->mixer()->apply_image_transform(transform, duration, tween);
			}
			else if(_parameters[1] == L"GAIN")
			{
				int duration = _parameters.size() > 2 ? lexical_cast_or_default(_parameters[3], 0) : 0;
				std::wstring tween = _parameters.size() > 3 ? _parameters[4] : L"linear";
				double value = boost::lexical_cast<double>(_parameters.at(2));
				
				auto transform = [=](image_transform transform) -> image_transform
				{
					transform.set_gain(value);
					return transform;					
				};

				int layer = GetLayerIndex(std::numeric_limits<int>::min());
				if(layer != std::numeric_limits<int>::min())
					GetChannel()->mixer()->apply_image_transform(GetLayerIndex(), transform, duration, tween);
				else
					GetChannel()->mixer()->apply_image_transform(transform, duration, tween);
			}
			else if(_parameters[1] == L"FILL_RECT")
			{
				int duration = _parameters.size() > 5 ? lexical_cast_or_default(_parameters[6], 0) : 0;
				std::wstring tween = _parameters.size() > 6 ? _parameters[7] : L"linear";
				double x	= boost::lexical_cast<double>(_parameters.at(2));
				double y	= boost::lexical_cast<double>(_parameters.at(3));
				double x_s	= boost::lexical_cast<double>(_parameters.at(4));
				double y_s	= boost::lexical_cast<double>(_parameters.at(5));

				auto transform = [=](image_transform transform) -> image_transform
				{
					transform.set_fill_translation(x, y);
					transform.set_fill_scale(x_s, y_s);
					transform.set_key_translation(x, y);
					transform.set_key_scale(x_s, y_s);
					return transform;
				};

				int layer = GetLayerIndex(std::numeric_limits<int>::min());
				if(layer != std::numeric_limits<int>::min())
					GetChannel()->mixer()->apply_image_transform(GetLayerIndex(), transform, duration, tween);
				else
					GetChannel()->mixer()->apply_image_transform(transform, duration, tween);
			}
			else if(_parameters[1] == L"KEY_RECT")
			{
				int duration = _parameters.size() > 5 ? lexical_cast_or_default(_parameters[6], 0) : 0;
				std::wstring tween = _parameters.size() > 6 ? _parameters[7] : L"linear";
				double x	= boost::lexical_cast<double>(_parameters.at(2));
				double y	= boost::lexical_cast<double>(_parameters.at(3));
				double x_s	= boost::lexical_cast<double>(_parameters.at(4));
				double y_s	= boost::lexical_cast<double>(_parameters.at(5));

				auto transform = [=](image_transform transform) -> image_transform
				{
					transform.set_key_translation(x, y);
					transform.set_key_scale(x_s, y_s);
					return transform;
				};

				int layer = GetLayerIndex(std::numeric_limits<int>::min());
				if(layer != std::numeric_limits<int>::min())
					GetChannel()->mixer()->apply_image_transform(GetLayerIndex(), transform, duration, tween);
				else
					GetChannel()->mixer()->apply_image_transform(transform, duration, tween);
			}
			else if(_parameters[1] == L"GRID")
			{
				int duration = _parameters.size() > 2 ? lexical_cast_or_default(_parameters[3], 0) : 0;
				std::wstring tween = _parameters.size() > 3 ? _parameters[4] : L"linear";
				int n = boost::lexical_cast<int>(_parameters.at(2));
				double delta = 1.0/static_cast<double>(n);
				for(int x = 0; x < n; ++x)
				{
					for(int y = 0; y < n; ++y)
					{
						int index = x+y*n+1;
						auto transform = [=](image_transform transform) -> image_transform
						{				
							transform.set_fill_translation(x*delta, y*delta);
							transform.set_fill_scale(delta, delta);			
							transform.set_key_translation(x*delta, y*delta);
							transform.set_key_scale(delta, delta);
							return transform;
						};
						GetChannel()->mixer()->apply_image_transform(index, transform, duration, tween);
					}
				}
			}
			else if(_parameters[1] == L"RESET")
			{
				int duration = _parameters.size() > 1 ? lexical_cast_or_default(_parameters[2], 0) : 0;
				std::wstring tween = _parameters.size() > 2 ? _parameters[3] : L"linear";

				int layer = GetLayerIndex(std::numeric_limits<int>::min());
				if(layer != std::numeric_limits<int>::min())
					GetChannel()->mixer()->reset_image_transform(GetLayerIndex(), duration, tween);
				else
					GetChannel()->mixer()->reset_image_transform(duration, tween);
			}
		}
		else if(_parameters[0] == L"AUDIO")
		{
			if(_parameters[1] == L"GAIN")
			{
				int duration = _parameters.size() > 2 ? lexical_cast_or_default(_parameters[3], 0) : 0;
				std::wstring tween = _parameters.size() > 3 ? _parameters[4] : L"linear";
				double value = boost::lexical_cast<double>(_parameters[2]);

				auto transform = [=](audio_transform transform) -> audio_transform
				{
					transform.set_gain(value);
					return transform;
				};
				
				int layer = GetLayerIndex(std::numeric_limits<int>::min());
				if(layer != std::numeric_limits<int>::min())
					GetChannel()->mixer()->apply_audio_transform(GetLayerIndex(), transform, duration, tween);
				else
					GetChannel()->mixer()->apply_audio_transform(transform, duration, tween);
			}
			else if(_parameters[1] == L"RESET")
			{
				int duration = _parameters.size() > 1 ? lexical_cast_or_default(_parameters[2], 0) : 0;
				std::wstring tween = _parameters.size() > 2 ? _parameters[3] : L"linear";
				GetChannel()->mixer()->reset_audio_transform(duration, tween);
			}
		}
		else if(_parameters[0] == L"RESET")
		{
			int duration = _parameters.size() > 1 ? lexical_cast_or_default(_parameters[2], 0) : 0;
			std::wstring tween = _parameters.size() > 2 ? _parameters[3] : L"linear";
			GetChannel()->mixer()->reset_image_transform(duration, tween);
			GetChannel()->mixer()->reset_audio_transform(duration, tween);
		}
	
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

			ch1->producer()->swap_layer(l1, l2, *ch2->producer());
		}
		else
		{
			auto ch1 = GetChannel();
			auto ch2 = GetChannels().at(boost::lexical_cast<int>(_parameters[0])-1);
			ch1->producer()->swap(*ch2->producer());
		}

		CASPAR_LOG(info) << "Swapped successfully";

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
		GetChannel()->consumer()->add(GetLayerIndex(), create_consumer(_parameters));
	
		CASPAR_LOG(info) << "Added " <<  _parameters[0] << TEXT(" successfully");

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
		GetChannel()->consumer()->remove(GetLayerIndex());

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
		_parameters[0] = _parameters[0];
		auto pFP = create_producer(GetChannel()->mixer(), _parameters);		
		GetChannel()->producer()->load(GetLayerIndex(), pFP, true);
	
		CASPAR_LOG(info) << "Loaded " <<  _parameters[0] << TEXT(" successfully");

		SetReplyString(TEXT("202 LOAD OK\r\n"));

		return true;
	}
	catch(file_not_found&)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
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



//std::function<std::wstring()> channel_cg_add_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
//{
//	static boost::wregex expr(L"^CG\\s(?<CHANNEL>\\d+)-?(?<LAYER>\\d+)?\\sADD\\s(?<FLASH_LAYER>\\d+)\\s(?<TEMPLATE>\\S+)\\s?(?<START_LABEL>\\S\\S+)?\\s?(?<PLAY_ON_LOAD>\\d)?\\s?(?<DATA>.*)?");
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
//		flash::get_default_cg_producer(info.channel, std::max<int>(DEFAULT_CHANNEL_LAYER+1, info.layer_index))
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
		
	static const boost::wregex expr(L".*(?<TRANSITION>CUT|PUSH|SLIDE|WIPE)\\s*(?<DURATION>\\d+)\\s*(?<TWEEN>(LINEAR)|(EASE[^\\s]*))?\\s*(?<DIRECTION>FROMLEFT|FROMRIGHT|LEFT|RIGHT)?.*");
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
		_parameters[0] = _parameters[0];
		auto pFP = create_producer(GetChannel()->mixer(), _parameters);
		if(pFP == frame_producer::empty())
			BOOST_THROW_EXCEPTION(file_not_found() << msg_info(_parameters.size() > 0 ? narrow(_parameters[0]) : ""));

		auto pFP2 = create_transition_producer(GetChannel()->get_video_format_desc().mode, pFP, transitionInfo);
		GetChannel()->producer()->load(GetLayerIndex(), pFP2); // TODO: LOOP
	
		CASPAR_LOG(info) << "Loaded " << _parameters[0] << TEXT(" successfully to background");
		SetReplyString(TEXT("202 LOADBG OK\r\n"));

		return true;
	}
	catch(file_not_found&)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
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
		GetChannel()->producer()->pause(GetLayerIndex());
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
			for(auto it = _parameters.begin(); it != _parameters.end(); ++it)
				lbg.AddParameter(*it);
			if(!lbg.Execute())
				CASPAR_LOG(warning) << " Failed to play.";

			CASPAR_LOG(info) << "Playing " << _parameters[0];
		}

		GetChannel()->producer()->play(GetLayerIndex());
		
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
		GetChannel()->producer()->stop(GetLayerIndex());
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
		GetChannel()->producer()->clear(index);
	else
		GetChannel()->producer()->clear();
		
	SetReplyString(TEXT("202 CLEAR OK\r\n"));

	return true;
}

bool CGCommand::DoExecute()
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
		label = _parameters[3];
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
		const std::wstring& dataString = _parameters[dataIndex];

		if(dataString[0] == TEXT('<')) //the data is an XML-string
			pDataString = dataString.c_str();
		else 
		{
			//The data is not an XML-string, it must be a filename
			std::wstring filename = env::data_folder();
			filename.append(dataString);
			filename.append(TEXT(".ftd"));

			//open file
			std::wifstream datafile(filename.c_str());
			if(datafile) 
			{
				//read all data
				data << datafile.rdbuf();
				datafile.close();

				//extract data to _parameters
				dataFromFile = data.str();
				pDataString = dataFromFile.c_str();
			}
		}
	}

	std::wstring fullFilename = find_flash_template(env::template_folder() + _parameters[2]);
	if(!fullFilename.empty())
	{
		std::wstring extension = boost::filesystem::wpath(fullFilename).extension();
		std::wstring filename = _parameters[2];
		filename.append(extension);

		get_default_cg_producer(safe_ptr<core::channel>(GetChannel()), GetLayerIndex(cg_producer::DEFAULT_LAYER))->add(layer, filename, bDoStart, label, (pDataString!=0) ? pDataString : TEXT(""));
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
		get_default_cg_producer(safe_ptr<core::channel>(GetChannel()), GetLayerIndex(cg_producer::DEFAULT_LAYER))->play(layer);
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
		get_default_cg_producer(safe_ptr<core::channel>(GetChannel()), GetLayerIndex(cg_producer::DEFAULT_LAYER))->stop(layer, 0);
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
		get_default_cg_producer(safe_ptr<core::channel>(GetChannel()), GetLayerIndex(cg_producer::DEFAULT_LAYER))->next(layer);
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
		get_default_cg_producer(safe_ptr<core::channel>(GetChannel()), GetLayerIndex(cg_producer::DEFAULT_LAYER))->remove(layer);
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
	GetChannel()->producer()->clear(GetLayerIndex(cg_producer::DEFAULT_LAYER));
	SetReplyString(TEXT("202 CG OK\r\n"));
	return true;
}

bool CGCommand::DoExecuteUpdate() 
{
	if(_parameters.size() > 2) 
	{
		if(!ValidateLayer(_parameters[1]))
		{
			SetReplyString(TEXT("403 CG ERROR\r\n"));
			return false;
		}
		int layer = _ttoi(_parameters[1].c_str());
		//TODO: Implement indirect data loading from file. Same as in Add
		get_default_cg_producer(safe_ptr<core::channel>(GetChannel()), GetLayerIndex(cg_producer::DEFAULT_LAYER))->update(layer, _parameters[2]);
	}
	else 
	{
		SetReplyString(TEXT("402 CG ERROR\r\n"));
		return true;
	}

	SetReplyString(TEXT("202 CG OK\r\n"));
	return true;
}

bool CGCommand::DoExecuteInvoke() 
{
	if(_parameters.size() > 2)
	{
		if(!ValidateLayer(_parameters[1]))
		{
			SetReplyString(TEXT("403 CG ERROR\r\n"));
			return false;
		}
		int layer = _ttoi(_parameters[1].c_str());
		get_default_cg_producer(safe_ptr<core::channel>(GetChannel()), GetLayerIndex(cg_producer::DEFAULT_LAYER))->invoke(layer, _parameters[2]);
	}
	else 
	{
		SetReplyString(TEXT("402 CG ERROR\r\n"));
		return true;
	}

	SetReplyString(TEXT("202 CG OK\r\n"));
	return true;
}

bool CGCommand::DoExecuteInfo() 
{
	// TODO
	//get_default_cg_producer(GetChannel())->Info();
	SetReplyString(TEXT("600 CG FAILED\r\n"));
	return true;
}

bool DataCommand::DoExecute()
{
	std::wstring command = _parameters[0];
	if(command == TEXT("STORE"))
		return DoExecuteStore();
	else if(command == TEXT("RETRIEVE"))
		return DoExecuteRetrieve();
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

	std::wofstream datafile(filename.c_str());
	if(!datafile) 
	{
		SetReplyString(TEXT("501 DATA STORE FAILED\r\n"));
		return false;
	}

	datafile << _parameters[2];
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

	std::wifstream datafile(filename.c_str());
	if(!datafile) 
	{
		SetReplyString(TEXT("404 DATA RETRIEVE ERROR\r\n"));
		return false;
	}

	std::wstringstream reply(TEXT("201 DATA RETRIEVE OK\r\n"));
	std::wstring line;
	bool bFirstLine = true;
	while(std::getline(datafile, line))
	{
		if(!bFirstLine)
			reply << "\\n";
		else
			bFirstLine = false;

		reply << line;
	}
	datafile.close();

	reply << "\r\n";
	SetReplyString(reply.str());
	return true;
}

bool DataCommand::DoExecuteList() 
{
	std::wstringstream replyString;
	replyString << TEXT("200 DATA LIST OK\r\n");
	replyString << ListMedia();
	replyString << TEXT("\r\n");

	SetReplyString(boost::to_upper_copy(replyString.str()));
	return true;
}

bool CinfCommand::DoExecute()
{
	std::wstringstream replyString;

	std::wstring filename = _parameters[0];

	// TODO:

	//FileInfo fileInfo;

	//MediaManagerPtr pMediaManager = GetApplication()->FindMediaFile(filename, &fileInfo);
	//if(pMediaManager != 0 && fileInfo.filetype.length() >0)	//File was found
	//{
	//	if(pMediaManager->getFileInfo(&fileInfo))
	//	{
	//		TCHAR numBuffer[32];
	//		_ui64tot_s(fileInfo.size, numBuffer, 32, 10);

	//		replyString << TEXT("201 CINF OK\r\n\"") << fileInfo.filename << TEXT("\" ") << fileInfo.type << TEXT("/") << fileInfo.filetype << TEXT("/") << fileInfo.encoding << TEXT(" ") << numBuffer << TEXT("\r\n");

	//		SetReplyString(replyString.str());
	//		return true;
	//	}
	//}

	SetReplyString(TEXT("404 CINF ERROR\r\n"));
	return false;
}

void GenerateChannelInfo(int index, const safe_ptr<core::channel>& pChannel, std::wstringstream& replyString)
{
	replyString << index+1 << TEXT(" ") << pChannel->get_video_format_desc().name << TEXT(" PLAYING") << TEXT("\r\n");
}

bool InfoCommand::DoExecute()
{
	std::wstringstream replyString;

	if(_parameters.size() >= 1)
	{
		int channelIndex = _ttoi(_parameters[0].c_str())-1;

		if(channelIndex < channels_.size())
		{
			replyString << TEXT("201 INFO OK\r\n");
			GenerateChannelInfo(channelIndex, channels_[channelIndex], replyString);
		}
		else
		{
			SetReplyString(TEXT("401 INFO ERROR\r\n"));
			return false;
		}
	}
	else
	{
		replyString << TEXT("200 INFO OK\r\n");
		for(size_t n = 0; n < channels_.size(); ++n)
			GenerateChannelInfo(n, channels_[n], replyString);
		replyString << TEXT("\r\n");
	}

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
	std::wstring replyString = TEXT("201 VERSION OK\r\n SERVER: ") + env::version() + TEXT("\r\n");

	if(_parameters.size() > 0)
	{
		if(_parameters[0] == L"FLASH")
			replyString = TEXT("201 VERSION OK\r\n FLASH: ") + get_flash_version() + TEXT("\r\n");
		else if(_parameters[0] == L"TEMPLATEHOST")
			replyString = TEXT("201 VERSION OK\r\n TEMPLATEHOST: ") + get_cg_version() + TEXT("\r\n");
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


}	//namespace amcp
}}	//namespace caspar