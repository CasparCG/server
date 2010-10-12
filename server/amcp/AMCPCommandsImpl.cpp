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
 
#include "..\StdAfx.h"
#include "AMCPCommandsImpl.h"
#include "AMCPProtocolStrategy.h"
#include "..\MediaManager.h"
#include "..\Application.h"
#include "..\channel.h"
#include "..\FileInfo.h"
#include "..\utils\findwrapper.h"
#include "..\utils\fileexists.h"
#include "..\cg\cgcontrol.h"

#include <algorithm>
#include <locale>
#include <fstream>
#include <cctype>
#include <io.h>

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

500 FAILED				Internt serverfel  
501 [kommando] FAILED	Internt serverfel  
502 [kommando] FAILED	Oläslig mediafil  

600 [kommando] FAILED	funktion ej implementerad
*/

namespace caspar {
namespace amcp {

using namespace utils;

AMCPCommand::AMCPCommand() : channelIndex_(0), scheduling_(Default)
{}

void AMCPCommand::SendReply() {
	if(pClientInfo_) {
		if(replyString_.length() > 0) {
			pClientInfo_->Send(replyString_);
		}
	}
}

void AMCPCommand::Clear() {
	pChannel_.reset();
	pClientInfo_.reset();
	channelIndex_ = 0;
	_parameters.clear();
}

//////////
// LOAD
AMCPCommandCondition LoadCommand::CheckConditions()
{
	return ConditionGood;
}

bool LoadCommand::Execute()
{
	if(!GetChannel())
		return false;
	if(_parameters.size() < 1)
		return false;

	bool bLoop = false;
	FileInfo fileInfo;
	MediaManagerPtr pMediaManager;
	
	tstring fullFilename = _parameters[0];

	if(_parameters.size()>1)
	{
		transform(_parameters[1].begin(), _parameters[1].end(), _parameters[1].begin(), toupper);
		if(_parameters[1] == TEXT("LOOP"))
			bLoop = true;
	}

	if(fullFilename[0] == TEXT('#'))
		pMediaManager = GetApplication()->GetColorMediaManager();
	else
		pMediaManager = GetApplication()->FindMediaFile(GetApplication()->GetMediaFolder()+fullFilename, &fileInfo);

	for(unsigned int i=0;i<_parameters.size();++i)
		transform(_parameters[i].begin(), _parameters[i].end(), _parameters[i].begin(), toupper);

	if(pMediaManager != 0)
	{
		if(fileInfo.filetype.length()>0)
		{
			fullFilename += TEXT(".");
			fullFilename += fileInfo.filetype;
		}

		MediaProducerPtr pFP;
		if(fullFilename[0] == TEXT('#'))
			pFP = pMediaManager->CreateProducer(fullFilename);
		else
			pFP = pMediaManager->CreateProducer(GetApplication()->GetMediaFolder()+fullFilename);

		if(GetChannel()->Load(pFP, bLoop))
		{
			LOG << LogLevel::Verbose << TEXT("Loaded ") <<  fullFilename << TEXT(" successfully");

			SetReplyString(TEXT("202 LOAD OK\r\n"));

#ifdef ENABLE_MONITOR
			GetChannel()->GetMonitor().Inform(Monitor::LOAD, _parameters[0]);
#endif
			return true;
		}
		else
		{
			LOG << LogLevel::Verbose << TEXT("Failed to load ") << fullFilename << TEXT(". It might be corrupt");

			SetReplyString(TEXT("502 LOAD FAILED\r\n"));
			return false;
		}
	}
	//else
	LOG << LogLevel::Verbose << TEXT("Could not find ") << fullFilename;

	SetReplyString(TEXT("404 LOAD ERROR\r\n"));
	return false;

}

//////////
// LOADBG
AMCPCommandCondition LoadbgCommand::CheckConditions()
{
	return ConditionGood;
}

bool LoadbgCommand::Execute()
{
	if(!GetChannel())
		return false;
	if(_parameters.size() < 1)
		return false;

	TransitionInfo transitionInfo;

	FileInfo fileInfo;
	MediaManagerPtr pMediaManager;

	tstring fullFilename = _parameters[0];

	bool bLoop = false;
	unsigned short transitionParameterIndex = 1;

	if(_parameters.size()>1)
	{
		transform(_parameters[1].begin(), _parameters[1].end(), _parameters[1].begin(), toupper);
		if(_parameters[1] == TEXT("LOOP"))
		{
			++transitionParameterIndex;
			bLoop = true;
		}
	}

	if(fullFilename[0] == TEXT('#'))
		pMediaManager = GetApplication()->GetColorMediaManager();
	else
		pMediaManager = GetApplication()->FindMediaFile(GetApplication()->GetMediaFolder()+fullFilename, &fileInfo);

	//Setup transition info
	if(_parameters.size()>transitionParameterIndex)	//type
	{
		tstring transitionType = _parameters[transitionParameterIndex];
		transform(transitionType.begin(), transitionType.end(), transitionType.begin(), toupper);

		if(transitionType == TEXT("CUT"))
			transitionInfo.type_ = Cut;
		else if(transitionType == TEXT("MIX"))
			transitionInfo.type_ = Mix;
		else if(transitionType == TEXT("PUSH"))
			transitionInfo.type_ = Push;
		else if(transitionType == TEXT("SLIDE"))
			transitionInfo.type_ = Slide;
		else if(transitionType == TEXT("WIPE"))
			transitionInfo.type_ = Wipe;

		if(_parameters.size() > static_cast<unsigned short>(transitionParameterIndex+1))	//duration
		{
			int duration = _ttoi(_parameters[transitionParameterIndex+1].c_str());
			if(duration > 0)
				transitionInfo.duration_ = duration;

			if(_parameters.size() > static_cast<unsigned short>(transitionParameterIndex+2))	//direction
			{
				tstring direction = _parameters[transitionParameterIndex+2];
				transform(direction.begin(), direction.end(), direction.begin(), toupper);

				if(direction == TEXT("FROMLEFT"))
					transitionInfo.direction_ = FromLeft;
				else if(direction == TEXT("FROMRIGHT"))
					transitionInfo.direction_ = FromRight;
				else if(direction == TEXT("LEFT"))
					transitionInfo.direction_ = FromRight;
				else if(direction == TEXT("RIGHT"))
					transitionInfo.direction_ = FromLeft;

				if(_parameters.size() > static_cast<unsigned short>(transitionParameterIndex+3))	//border
				{
					tstring border = _parameters[transitionParameterIndex+3];
					if(border.size()>0)
					{
						if(border[0] == TEXT('#'))
							transitionInfo.borderColor_ = border;
						else
							transitionInfo.borderImage_ = border;
					}

					if(_parameters.size() > static_cast<unsigned short>(transitionParameterIndex+4))	//border width
					{
						transitionInfo.borderWidth_ = _ttoi(_parameters[transitionParameterIndex+4].c_str());
					}
				}
			}
		}
	}

	//Perform loading of the clip
	if(pMediaManager != 0)
	{
		if(fileInfo.filetype.length()>0)
		{
			fullFilename += TEXT(".");
			fullFilename += fileInfo.filetype;
		}

		MediaProducerPtr pFP;
		if(fullFilename[0] == TEXT('#'))
			pFP = pMediaManager->CreateProducer(fullFilename);
		else
			pFP = pMediaManager->CreateProducer(GetApplication()->GetMediaFolder()+fullFilename);

		if(GetChannel()->LoadBackground(pFP, transitionInfo, bLoop))
		{
			LOG << LogLevel::Verbose << TEXT("Loaded ") << fullFilename << TEXT(" successfully to background");
			SetReplyString(TEXT("202 LOADBG OK\r\n"));

#ifdef ENABLE_MONITOR
			GetChannel()->GetMonitor().Inform(Monitor::LOADBG, _parameters[0]);
#endif
			return true;
		}
		else
		{
			LOG << LogLevel::Verbose << TEXT("Failed to load ") << fullFilename << TEXT(" to background. It might be corrupt");
			SetReplyString(TEXT("502 LOADBG FAILED\r\n"));
			return false;
		}
	}
	//else
	LOG << LogLevel::Verbose << TEXT("Could not find ") << fullFilename;
	SetReplyString(TEXT("404 LOADBG ERROR\r\n"));
	return false;

}

//////////
// PLAY
AMCPCommandCondition PlayCommand::CheckConditions()
{
	return ConditionGood;
}

bool PlayCommand::Execute()
{
	if(!GetChannel())
		return false;

	if(GetChannel()->Play())
	{
		SetReplyString(TEXT("202 PLAY OK\r\n"));
		return true;
	}

	SetReplyString(TEXT("501 PLAY FAILED\r\n"));
	return false;
}

//////////
// STOP
AMCPCommandCondition StopCommand::CheckConditions()
{
	return ConditionGood;
}

bool StopCommand::Execute()
{
	if(!GetChannel())
		return false;

	if(GetChannel()->Stop())
	{
		SetReplyString(TEXT("202 STOP OK\r\n"));
		return true;
	}

	SetReplyString(TEXT("501 STOP FAILED\r\n"));
	return false;
}

//////////
// CLEAR
AMCPCommandCondition ClearCommand::CheckConditions()
{
	return ConditionGood;
}

bool ClearCommand::Execute()
{
	if(!GetChannel())
		return false;

	if(GetChannel()->Clear())
	{
		SetReplyString(TEXT("202 CLEAR OK\r\n"));

#ifdef ENABLE_MONITOR
		GetChannel()->GetMonitor().Inform(Monitor::CLEAR);
#endif
		return true;
	}

	SetReplyString(TEXT("501 CLEAR FAILED\r\n"));
	return false;
}

//////////
// PARAM
AMCPCommandCondition ParamCommand::CheckConditions()
{
	return ConditionGood;
}

bool ParamCommand::Execute()
{
	if(_parameters.size() < 1)
		return false;

	if(!GetChannel())
		return false;

	if(GetChannel()->Param(_parameters[0]))
	{
		SetReplyString(TEXT("202 PARAM OK\r\n"));
		return true;
	}

	SetReplyString(TEXT("501 PARAM FAILED\r\n"));
	return true;
}

//////////
// CG
AMCPCommandCondition CGCommand::CheckConditions()
{
	return ConditionGood;
}

bool CGCommand::Execute()
{
	if(_parameters.size() < 1)
		return false;
	if(!GetChannel())
		return false;

	tstring command = _parameters[0];
	std::transform(command.begin(), command.end(), command.begin(), toupper);
	if(command == TEXT("ADD"))
		return ExecuteAdd();
	else if(command == TEXT("PLAY"))
		return ExecutePlay();
	else if(command == TEXT("STOP"))
		return ExecuteStop();
	else if(command == TEXT("NEXT"))
		return ExecuteNext();
	else if(command == TEXT("REMOVE"))
		return ExecuteRemove();
	else if(command == TEXT("CLEAR"))
		return ExecuteClear();
	else if(command == TEXT("UPDATE"))
		return ExecuteUpdate();
	else if(command == TEXT("INVOKE"))
		return ExecuteInvoke();
	else if(command == TEXT("INFO"))
		return ExecuteInfo();

	SetReplyString(TEXT("403 CG ERROR\r\n"));
	return false;
}

bool CGCommand::ValidateLayer(const tstring& layerstring) {
	int length = layerstring.length();
	for(int i = 0; i < length; ++i) {
		if(!_istdigit(layerstring[i])) {
			return false;
		}
	}

	return true;
}

bool CGCommand::ExecuteAdd() {
	//CG 1 ADD 0 "templatefolder/templatename" [STARTLABEL] 0/1 [DATA]

	int layer = 0;				//_parameters[1]
//	tstring templateName;	//_parameters[2]
	tstring label;		//_parameters[3]
	bool bDoStart = false;		//_parameters[3] alt. _parameters[4]
//	tstring data;			//_parameters[4] alt. _parameters[5]

	if(_parameters.size() < 4) {
		SetReplyString(TEXT("402 CG ERROR\r\n"));
		return false;
	}
	unsigned int dataIndex = 4;

	if(!ValidateLayer(_parameters[1])) {
		SetReplyString(TEXT("403 CG ERROR\r\n"));
		return false;
	}

	layer = _ttoi(_parameters[1].c_str());

	if(_parameters[3].length() > 1) {	//read label
		label = _parameters[3];
		++dataIndex;

		if(_parameters.size() > 4 && _parameters[4].length() > 0)	//read play-on-load-flag
			bDoStart = (_parameters[4][0]==TEXT('1')) ? true : false;
		else {
			SetReplyString(TEXT("402 CG ERROR\r\n"));
			return false;
		}
	}
	else if(_parameters[3].length() > 0) {	//read play-on-load-flag
		bDoStart = (_parameters[3][0]==TEXT('1')) ? true : false;
	}
	else {
		SetReplyString(TEXT("403 CG ERROR\r\n"));
		return false;
	}

	const TCHAR* pDataString = 0;
	tstringstream data;
	tstring dataFromFile;
	if(_parameters.size() > dataIndex) {	//read data
		const tstring& dataString = _parameters[dataIndex];

		if(dataString[0] == TEXT('<')) {
			//the data is an XML-string
			pDataString = dataString.c_str();
		}
		else {
			//The data is not an XML-string, it must be a filename
			tstring filename = GetApplication()->GetDataFolder();
			filename.append(dataString);
			filename.append(TEXT(".ftd"));

			//open file
			std::wifstream datafile(filename.c_str());
			if(datafile) {
				//read all data
				data << datafile.rdbuf();
				datafile.close();

				//extract data to _parameters
				dataFromFile = data.str();
				pDataString = dataFromFile.c_str();
			}
		}
	}

	tstring fullFilename = GetApplication()->GetTemplateFolder() + _parameters[2];
	if(GetApplication()->FindTemplate(fullFilename))
	{
		GetChannel()->GetCGControl()->Add(layer, _parameters[2], bDoStart, label, (pDataString!=0) ? pDataString : TEXT(""));
		SetReplyString(TEXT("202 CG OK\r\n"));

#ifdef ENABLE_MONITOR
		GetChannel()->GetMonitor().Inform(Monitor::CG_ADD, _parameters[2]);
#endif
	}
	else
	{
		LOG << LogLevel::Verbose << TEXT("Could not find template ") << _parameters[2];
		SetReplyString(TEXT("404 CG ERROR\r\n"));
	}
	return true;
}

bool CGCommand::ExecutePlay() {
	if(_parameters.size() > 1) {
		if(!ValidateLayer(_parameters[1])) {
			SetReplyString(TEXT("403 CG ERROR\r\n"));
			return false;
		}
		int layer = _ttoi(_parameters[1].c_str());
		GetChannel()->GetCGControl()->Play(layer);
	}
	else {
		SetReplyString(TEXT("402 CG ERROR\r\n"));
		return true;
	}

	SetReplyString(TEXT("202 CG OK\r\n"));
	return true;
}

bool CGCommand::ExecuteStop() {
	if(_parameters.size() > 1) {
		if(!ValidateLayer(_parameters[1])) {
			SetReplyString(TEXT("403 CG ERROR\r\n"));
			return false;
		}
		int layer = _ttoi(_parameters[1].c_str());
		GetChannel()->GetCGControl()->Stop(layer, 0);
	}
	else {
		SetReplyString(TEXT("402 CG ERROR\r\n"));
		return true;
	}

	SetReplyString(TEXT("202 CG OK\r\n"));
	return true;
}

bool CGCommand::ExecuteNext() {
	if(_parameters.size() > 1) {
		if(!ValidateLayer(_parameters[1])) {
			SetReplyString(TEXT("403 CG ERROR\r\n"));
			return false;
		}
		int layer = _ttoi(_parameters[1].c_str());
		GetChannel()->GetCGControl()->Next(layer);
	}
	else {
		SetReplyString(TEXT("402 CG ERROR\r\n"));
		return true;
	}

	SetReplyString(TEXT("202 CG OK\r\n"));
	return true;
}

bool CGCommand::ExecuteRemove() {
	if(_parameters.size() > 1) {
		if(!ValidateLayer(_parameters[1])) {
			SetReplyString(TEXT("403 CG ERROR\r\n"));
			return false;
		}
		int layer = _ttoi(_parameters[1].c_str());
		GetChannel()->GetCGControl()->Remove(layer);
	}
	else {
		SetReplyString(TEXT("402 CG ERROR\r\n"));
		return true;
	}

	SetReplyString(TEXT("202 CG OK\r\n"));
	return true;
}

bool CGCommand::ExecuteClear() {
	GetChannel()->GetCGControl()->Clear();
	SetReplyString(TEXT("202 CG OK\r\n"));
#ifdef ENABLE_MONITOR
	GetChannel()->GetMonitor().Inform(Monitor::CG_CLEAR);
#endif
	return true;
}

bool CGCommand::ExecuteUpdate() {
	if(_parameters.size() > 2) {
		if(!ValidateLayer(_parameters[1])) {
			SetReplyString(TEXT("403 CG ERROR\r\n"));
			return false;
		}
		int layer = _ttoi(_parameters[1].c_str());
		//TODO: Implement indirect data loading from file. Same as in Add
		GetChannel()->GetCGControl()->Update(layer, _parameters[2]);
	}
	else {
		SetReplyString(TEXT("402 CG ERROR\r\n"));
		return true;
	}

	SetReplyString(TEXT("202 CG OK\r\n"));
	return true;
}

bool CGCommand::ExecuteInvoke() {
	if(_parameters.size() > 2) {
		if(!ValidateLayer(_parameters[1])) {
			SetReplyString(TEXT("403 CG ERROR\r\n"));
			return false;
		}
		int layer = _ttoi(_parameters[1].c_str());
		GetChannel()->GetCGControl()->Invoke(layer, _parameters[2]);
	}
	else {
		SetReplyString(TEXT("402 CG ERROR\r\n"));
		return true;
	}

	SetReplyString(TEXT("202 CG OK\r\n"));
	return true;
}

bool CGCommand::ExecuteInfo() {
//	GetChannel()->GetCGControl()->Info();
	SetReplyString(TEXT("600 CG FAILED\r\n"));
	return true;
}

//////////
// DATA
AMCPCommandCondition DataCommand::CheckConditions()
{
	return ConditionGood;
}

bool DataCommand::Execute()
{
	if(_parameters.size() < 1)
		return false;

	tstring command = _parameters[0];
	std::transform(command.begin(), command.end(), command.begin(), toupper);
	if(command == TEXT("STORE"))
		return ExecuteStore();
	else if(command == TEXT("RETRIEVE"))
		return ExecuteRetrieve();
	else if(command == TEXT("LIST"))
		return ExecuteList();

	SetReplyString(TEXT("403 DATA ERROR\r\n"));
	return false;
}

bool DataCommand::ExecuteStore() {
	if(_parameters.size() < 3) {
		SetReplyString(TEXT("402 DATA STORE ERROR\r\n"));
		return false;
	}

	tstring filename = GetApplication()->GetDataFolder();
	filename.append(_parameters[1]);
	filename.append(TEXT(".ftd"));

	std::wofstream datafile(filename.c_str());
	if(!datafile) {
		SetReplyString(TEXT("501 DATA STORE FAILED\r\n"));
		return false;
	}

	datafile << _parameters[2];
	datafile.close();

	tstring replyString = TEXT("202 DATA STORE OK\r\n");
	SetReplyString(replyString);
	return true;
}

bool DataCommand::ExecuteRetrieve() {
	if(_parameters.size() < 2) {
		SetReplyString(TEXT("402 DATA RETRIEVE ERROR\r\n"));
		return false;
	}

	tstring filename = GetApplication()->GetDataFolder();
	filename.append(_parameters[1]);
	filename.append(TEXT(".ftd"));

	std::wifstream datafile(filename.c_str());
	if(!datafile) {
		SetReplyString(TEXT("404 DATA RETRIEVE ERROR\r\n"));
		return false;
	}

	tstringstream reply(TEXT("201 DATA RETRIEVE OK\r\n"));
	tstring line;
	bool bFirstLine = true;
	while(std::getline(datafile, line)) {
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

bool DataCommand::ExecuteList() {
	tstringstream replyString;
	replyString << TEXT("200 DATA LIST OK\r\n");

	WIN32_FIND_DATA fileInfo;
	caspar::utils::FindWrapper findWrapper(GetApplication()->GetDataFolder() + TEXT("*.ftd"), &fileInfo);
	if(findWrapper.Success())
	{
		do
		{
			tstring filename = fileInfo.cFileName;
			transform(filename.begin(), filename.end(), filename.begin(), toupper);
			
			tstring::size_type pos = filename.rfind(TEXT("."));
			if(pos != tstring::npos)
			{
				TCHAR numBuffer[32];
				TCHAR timeBuffer[32];
				TCHAR dateBuffer[32];

				unsigned __int64 fileSize = fileInfo.nFileSizeHigh;
				fileSize *= 0x100000000;
				fileSize += fileInfo.nFileSizeLow;

				_ui64tot_s(fileSize, numBuffer, 32, 10);

				SYSTEMTIME lastWriteTime;
				FileTimeToSystemTime(&(fileInfo.ftLastWriteTime), &lastWriteTime);
				GetDateFormat(LOCALE_USER_DEFAULT, 0, &lastWriteTime, _T("yyyyMMdd"), dateBuffer, sizeof(dateBuffer));
				GetTimeFormat(LOCALE_USER_DEFAULT, 0, &lastWriteTime, _T("HHmmss"), timeBuffer, sizeof(timeBuffer));

				replyString << TEXT("\"") << filename.substr(0, pos) << TEXT("\" ") << numBuffer << TEXT(" ") << dateBuffer << timeBuffer << TEXT("\r\n");
			}
		}
		while(findWrapper.FindNext(&fileInfo));
	}
	replyString << TEXT("\r\n");

	SetReplyString(replyString.str());
	return true;
}

//////////
// CINF
AMCPCommandCondition CinfCommand::CheckConditions()
{
	return ConditionGood;
}

bool CinfCommand::Execute()
{
	tstringstream replyString;

	if(_parameters.size() < 1)
		return false;

	tstring filename = GetApplication()->GetMediaFolder()+_parameters[0];

	FileInfo fileInfo;

	MediaManagerPtr pMediaManager = GetApplication()->FindMediaFile(filename, &fileInfo);
	if(pMediaManager != 0 && fileInfo.filetype.length() >0)	//File was found
	{
		if(pMediaManager->getFileInfo(&fileInfo))
		{
			TCHAR numBuffer[32];
			_ui64tot_s(fileInfo.size, numBuffer, 32, 10);

			replyString << TEXT("201 CINF OK\r\n\"") << fileInfo.filename << TEXT("\" ") << fileInfo.type << TEXT("/") << fileInfo.filetype << TEXT("/") << fileInfo.encoding << TEXT(" ") << numBuffer << TEXT("\r\n");

			SetReplyString(replyString.str());
			return true;
		}
	}

	SetReplyString(TEXT("404 CINF ERROR\r\n"));
	return false;
}

//////////
// INFO
AMCPCommandCondition InfoCommand::CheckConditions()
{
	return ConditionGood;
}

bool InfoCommand::Execute()
{
	tstringstream replyString;

	if(_parameters.size() >= 1)
	{
		int channelIndex = _ttoi(_parameters[0].c_str())-1;
		ChannelPtr pChannel = GetApplication()->GetChannel(static_cast<unsigned int>(channelIndex));

		if(pChannel != 0)
		{
			replyString << TEXT("201 INFO OK\r\n");
			GenerateChannelInfo(pChannel, replyString);
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

		ChannelPtr pChannel;
		unsigned int channelIndex = 0;
		while((pChannel = GetApplication()->GetChannel(channelIndex++)) != 0)
		{
			GenerateChannelInfo(pChannel, replyString);
		}
		replyString << TEXT("\r\n");
	}

	SetReplyString(replyString.str());
	return true;
}

void InfoCommand::GenerateChannelInfo(ChannelPtr& pChannel, tstringstream& replyString)
{
	replyString << pChannel->GetIndex() << TEXT(" ") << pChannel->GetFormatDescription() << (pChannel->IsPlaybackRunning() ? TEXT(" PLAYING") : TEXT(" STOPPED")) << TEXT("\r\n");
}

//////////
// CLS
AMCPCommandCondition ClsCommand::CheckConditions()
{
	return ConditionGood;
}

bool ClsCommand::Execute()
{
	/*
		wav = audio
		mp3 = audio
		swf	= movie
		dv  = movie
		tga = still
		col = still
	*/
	tstring cliptype[4] = { _T(" N/A "), _T(" AUDIO "), _T(" MOVIE "), _T(" STILL ") };


	tstringstream replyString;
	replyString << TEXT("200 CLS OK\r\n");

	WIN32_FIND_DATA fileInfo;
	caspar::utils::FindWrapper findWrapper(GetApplication()->GetMediaFolder() + TEXT("*.*"), &fileInfo);
	if(findWrapper.Success())
	{
		unsigned char cliptypeindex = 0;
		bool bGood = false;

		do
		{
			tstring filename = fileInfo.cFileName;

			transform(filename.begin(), filename.end(), filename.begin(), toupper);
			
			tstring::size_type pos = filename.rfind(TEXT("."));
			if(pos != tstring::npos)
			{
				tstring extension = filename.substr(pos+1);
				if(extension == TEXT("TGA") || extension == TEXT("COL"))
				{
					cliptypeindex = 3;
					bGood = true;
				}
				else if(extension == TEXT("SWF") || extension == TEXT("DV") || extension == TEXT("MOV") || extension == TEXT("MPG") || extension == TEXT("AVI") || extension == TEXT("STGA"))
				{
					cliptypeindex = 2;
					bGood = true;
				}
				else if(extension == TEXT("WAV") || extension == TEXT("MP3"))
				{
					cliptypeindex = 1;
					bGood = true;
				}

				if(bGood)
				{
					TCHAR numBuffer[32];
					TCHAR timeBuffer[32];
					TCHAR dateBuffer[32];

					unsigned __int64 fileSize = fileInfo.nFileSizeHigh;
					fileSize *= 0x100000000;
					fileSize += fileInfo.nFileSizeLow;

					_ui64tot_s(fileSize, numBuffer, 32, 10);

					SYSTEMTIME lastWriteTime;
					FileTimeToSystemTime(&(fileInfo.ftLastWriteTime), &lastWriteTime);
					GetDateFormat(LOCALE_USER_DEFAULT, 0, &lastWriteTime, _T("yyyyMMdd"), dateBuffer, sizeof(dateBuffer));
					GetTimeFormat(LOCALE_USER_DEFAULT, 0, &lastWriteTime, _T("HHmmss"), timeBuffer, sizeof(timeBuffer));

					replyString << TEXT("\"") << filename.substr(0, pos) << TEXT("\" ") << cliptype[cliptypeindex] << TEXT(" ") << numBuffer << TEXT(" ") << dateBuffer << timeBuffer << TEXT("\r\n");

					cliptypeindex = 0;
					bGood = false;
				}
			}
		}
		while(findWrapper.FindNext(&fileInfo));
	}
	replyString << TEXT("\r\n");

	SetReplyString(replyString.str());
	return true;
}

//////////
// TLS
AMCPCommandCondition TlsCommand::CheckConditions()
{
	return ConditionGood;
}

bool TlsCommand::Execute()
{
	tstringstream replyString;
	replyString << TEXT("200 TLS OK\r\n");

	FindInDirectory(TEXT(""), replyString);
	replyString << TEXT("\r\n");

	SetReplyString(replyString.str());
	return true;
}

void TlsCommand::FindInDirectory(const tstring& dir, tstringstream& replyString) {
	{	//Find files in directory
		WIN32_FIND_DATA fileInfo;
		caspar::utils::FindWrapper findWrapper(GetApplication()->GetTemplateFolder() + dir + TEXT("*.ft"), &fileInfo);
		if(findWrapper.Success())
		{
			do
			{
				if(((fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY) && ((fileInfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != FILE_ATTRIBUTE_HIDDEN)) {
					tstring filename = fileInfo.cFileName;

					transform(filename.begin(), filename.end(), filename.begin(), toupper);
					
					TCHAR numBuffer[32];
					TCHAR timeBuffer[32];
					TCHAR dateBuffer[32];

					unsigned __int64 fileSize = fileInfo.nFileSizeHigh;
					fileSize *= 0x100000000;
					fileSize += fileInfo.nFileSizeLow;

					_ui64tot_s(fileSize, numBuffer, 32, 10);

					SYSTEMTIME lastWriteTime;
					FileTimeToSystemTime(&(fileInfo.ftLastWriteTime), &lastWriteTime);
					GetDateFormat(LOCALE_USER_DEFAULT, 0, &lastWriteTime, _T("yyyyMMdd"), dateBuffer, sizeof(dateBuffer));
					GetTimeFormat(LOCALE_USER_DEFAULT, 0, &lastWriteTime, _T("HHmmss"), timeBuffer, sizeof(timeBuffer));

					replyString << TEXT("\"") << dir << filename.substr(0, filename.size()-3) << TEXT("\" ") << numBuffer << TEXT(" ") << dateBuffer << timeBuffer << TEXT("\r\n");
				}
			}
			while(findWrapper.FindNext(&fileInfo));
		}
	}

	{	//Find subdirectories in directory
		WIN32_FIND_DATA fileInfo;
		caspar::utils::FindWrapper findWrapper(GetApplication()->GetTemplateFolder() + dir + TEXT("*"), &fileInfo);
		if(findWrapper.Success())
		{
			do
			{
				if((fileInfo.cFileName[0] != TEXT('.')) && ((fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) && ((fileInfo.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != FILE_ATTRIBUTE_HIDDEN)) {
					FindInDirectory(dir + fileInfo.cFileName + TEXT('\\'), replyString);
				}
			}
			while(findWrapper.FindNext(&fileInfo));
		}
	}
}

//////////
// VERSION
AMCPCommandCondition VersionCommand::CheckConditions()
{
	return ConditionGood;
}

bool VersionCommand::Execute()
{
	tstringstream replyString;
	replyString << TEXT("201 VERSION OK\r\n") << GetApplication()->GetVersionString() << TEXT("\r\n");

	SetReplyString(replyString.str());
	return true;
}

//////////
// BYE
AMCPCommandCondition ByeCommand::CheckConditions()
{
	return ConditionGood;
}

bool ByeCommand::Execute()
{
	GetClientInfo()->Disconnect();
	return true;
}


//////////
// SET

AMCPCommandCondition SetCommand::CheckConditions()
{
	return ConditionGood;
}

bool SetCommand::Execute()
{
	tstring name = _parameters[0];
	std::transform(name.begin(), name.end(), name.begin(), toupper);

	tstring value = _parameters[1];
	std::transform(value.begin(), value.end(), value.begin(), toupper);

	if(name == TEXT("MODE"))
	{
		if(this->GetChannel()->SetVideoFormat(value))
			this->SetReplyString(TEXT("202 SET MODE OK\r\n"));
		else
			this->SetReplyString(TEXT("501 SET MODE FAILED\r\n"));
	}
	else
	{
		this->SetReplyString(TEXT("403 SET ERROR\r\n"));
	}

	return true;
}

#ifdef ENABLE_MONITOR
///////////
// MONITOR
AMCPCommandCondition MonitorCommand::CheckConditions()
{
	return ConditionGood;
}

bool MonitorCommand::Execute()
{
	if(!GetChannel())
		return false;

	tstring cmd = _parameters[0];
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), toupper);

	if(cmd == TEXT("START")) {
		GetChannel()->GetMonitor().AddListener(GetClientInfo());
		SetReplyString(TEXT("202 MONITOR START OK\r\n"));
	}
	else if(cmd == TEXT("STOP")) {
		GetChannel()->GetMonitor().RemoveListener(GetClientInfo());
		SetReplyString(TEXT("202 MONITOR STOP OK\r\n"));
	}
	else
		SetReplyString(TEXT("403 MONITOR ERROR\r\n"));

	return true;
}
#endif

//////////
// KILL
//AMCPCommandCondition KillCommand::CheckConditions()
//{
//	return ConditionGood;
//}
//
//bool KillCommand::Execute()
//{
//	//int* pS = 0;
//	//*pS = 42;
//	caspar::GetApplication()->GetTerminateEvent().Set();
//	return true;
//}

/*
//////////
// FLS
AMCPCommandCondition FlsCommand::CheckConditions()
{
	return ConditionGood;
}

bool FlsCommand::Execute()
{
	tstring returnMessage = "200 FLS OK";
	returnMessage += "\r\n";

	tstring searchPattern = "*.*";
	tstring directory = "";

	if(_parameters.size() >= 1) {
		tstring::size_type separatorIndex = _parameters[0].find_last_of('\\');
		if(separatorIndex != tstring::npos) {
			directory = _parameters[0].substr(0, separatorIndex+1);
			if(_parameters[0].length() > (separatorIndex+1))
				searchPattern = _parameters[0].substr(separatorIndex+1);
		}
		else
			searchPattern = _parameters[0];
	}

	FindFilesInDirectory(directory, searchPattern, returnMessage);
	SetReplyString(returnMessage);

	return true;
}

void FlsCommand::FindFilesInDirectory(const tstring& dir, const tstring& pattern, tstring& result)
{
	WIN32_FIND_DATA fileInfo;

	tstring searchPattern = pattern;
	if(dir.length() > 0) {
		searchPattern = dir + '\\' + pattern;
	}
	HANDLE hFile = FindFirstFile(searchPattern.c_str(), &fileInfo);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if((fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY) {
				if(fileInfo.cFileName[0] != '.') {
					tstring completeDir = fileInfo.cFileName;
					if(dir.length() > 0) {
						completeDir = dir + '\\' + completeDir;
					}

					FindFilesInDirectory(completeDir, pattern, result);
				}
			}
			else {
				tstring filename = fileInfo.cFileName;
				if(dir.length() > 0) {
					filename = dir + '\\' + filename;
				}

				transform(filename.begin(), filename.end(), filename.begin(), toupper);
				
				char numBuffer[32];
				TCHAR timeBuffer[32];
				TCHAR dateBuffer[32];

				_itoa_s(fileInfo.nFileSizeLow, numBuffer, sizeof(numBuffer), 10);

				SYSTEMTIME lastWriteTime;
				FileTimeToSystemTime(&(fileInfo.ftLastWriteTime), &lastWriteTime);
				GetDateFormat(LOCALE_USER_DEFAULT, 0, &lastWriteTime, _T("yyyyMMdd"), dateBuffer, sizeof(dateBuffer));
				GetTimeFormat(LOCALE_USER_DEFAULT, 0, &lastWriteTime, _T("HHmmss"), timeBuffer, sizeof(timeBuffer));

				result += "\"";
				result += filename;
				result += "\" ";

				result += numBuffer;
				result += " ";
				result += dateBuffer;
				result += timeBuffer;
				result += "\r\n";
			}
		}
		while(FindNextFile(hFile, &fileInfo));
	}
	FindClose(hFile);
}

*/

}	//namespace amcp
}	//namespace caspar