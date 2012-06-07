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

#include "AMCPProtocolStrategy.h"

#include "../util/AsyncEventServer.h"
#include "AMCPCommandsImpl.h"

#include <stdio.h>
#include <crtdbg.h>
#include <string.h>
#include <algorithm>
#include <cctype>

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/lexical_cast.hpp>

#if defined(_MSC_VER)
#pragma warning (push, 1) // TODO: Legacy code, just disable warnings
#endif

namespace caspar { namespace protocol { namespace amcp {

using IO::ClientInfoPtr;

const std::wstring AMCPProtocolStrategy::MessageDelimiter = TEXT("\r\n");

inline std::shared_ptr<core::video_channel> GetChannelSafe(unsigned int index, const std::vector<safe_ptr<core::video_channel>>& channels)
{
	return index < channels.size() ? std::shared_ptr<core::video_channel>(channels[index]) : nullptr;
}

AMCPProtocolStrategy::AMCPProtocolStrategy(const std::vector<safe_ptr<core::video_channel>>& channels) : channels_(channels) {
	AMCPCommandQueuePtr pGeneralCommandQueue(new AMCPCommandQueue());
	commandQueues_.push_back(pGeneralCommandQueue);


	std::shared_ptr<core::video_channel> pChannel;
	unsigned int index = -1;
	//Create a commandpump for each video_channel
	while((pChannel = GetChannelSafe(++index, channels_)) != 0) {
		AMCPCommandQueuePtr pChannelCommandQueue(new AMCPCommandQueue());
		std::wstring title = TEXT("video_channel ");

		//HACK: Perform real conversion from int to string
		TCHAR num = TEXT('1')+static_cast<TCHAR>(index);
		title += num;
		
		commandQueues_.push_back(pChannelCommandQueue);
	}
}

AMCPProtocolStrategy::~AMCPProtocolStrategy() {
}

void AMCPProtocolStrategy::Parse(const TCHAR* pData, int charCount, ClientInfoPtr pClientInfo)
{
	size_t pos;
	std::wstring recvData(pData, charCount);
	std::wstring availibleData = (pClientInfo != nullptr ? pClientInfo->currentMessage_ : L"") + recvData;

	while(true) {
		pos = availibleData.find(MessageDelimiter);
		if(pos != std::wstring::npos)
		{
			std::wstring message = availibleData.substr(0,pos);

			//This is where a complete message gets taken care of
			if(message.length() > 0) {
				ProcessMessage(message, pClientInfo);
			}

			std::size_t nextStartPos = pos + MessageDelimiter.length();
			if(nextStartPos < availibleData.length())
				availibleData = availibleData.substr(nextStartPos);
			else {
				availibleData.clear();
				break;
			}
		}
		else
		{
			break;
		}
	}
	if(pClientInfo)
		pClientInfo->currentMessage_ = availibleData;
}

void AMCPProtocolStrategy::ProcessMessage(const std::wstring& message, ClientInfoPtr& pClientInfo)
{	
	CASPAR_LOG(info) << L"Received message from " << pClientInfo->print() << ": " << message << L"\\r\\n";
	
	bool bError = true;
	MessageParserState state = New;

	AMCPCommandPtr pCommand;

	pCommand = InterpretCommandString(message, &state);

	if(pCommand != 0) {
		pCommand->SetClientInfo(pClientInfo);	
		if(QueueCommand(pCommand))
			bError = false;
		else
			state = GetChannel;
	}

	if(bError == true) {
		std::wstringstream answer;
		switch(state)
		{
		case GetCommand:
			answer << TEXT("400 ERROR\r\n") + message << "\r\n";
			break;
		case GetChannel:
			answer << TEXT("401 ERROR\r\n");
			break;
		case GetParameters:
			answer << TEXT("402 ERROR\r\n");
			break;
		default:
			answer << TEXT("500 FAILED\r\n");
			break;
		}
		pClientInfo->Send(answer.str());
	}
}

AMCPCommandPtr AMCPProtocolStrategy::InterpretCommandString(const std::wstring& message, MessageParserState* pOutState)
{
	std::vector<std::wstring> tokens;
	unsigned int currentToken = 0;
	std::wstring commandSwitch;

	AMCPCommandPtr pCommand;
	MessageParserState state = New;

	std::size_t tokensInMessage = TokenizeMessage(message, &tokens);

	//parse the message one token at the time
	while(currentToken < tokensInMessage)
	{
		switch(state)
		{
		case New:
			if(tokens[currentToken][0] == TEXT('/'))
				state = GetSwitch;
			else
				state = GetCommand;
			break;

		case GetSwitch:
			commandSwitch = tokens[currentToken];
			state = GetCommand;
			++currentToken;
			break;

		case GetCommand:
			pCommand = CommandFactory(tokens[currentToken]);
			if(pCommand == 0) {
				goto ParseFinnished;
			}
			else
			{
				pCommand->SetChannels(channels_);
				//Set scheduling
				if(commandSwitch.size() > 0) {
					transform(commandSwitch.begin(), commandSwitch.end(), commandSwitch.begin(), toupper);

					if(commandSwitch == TEXT("/APP"))
						pCommand->SetScheduling(AddToQueue);
					else if(commandSwitch  == TEXT("/IMMF"))
						pCommand->SetScheduling(ImmediatelyAndClear);
				}

				if(pCommand->NeedChannel())
					state = GetChannel;
				else
					state = GetParameters;
			}
			++currentToken;
			break;

		case GetParameters:
			{
				_ASSERTE(pCommand != 0);
				int parameterCount=0;
				while(currentToken<tokensInMessage)
				{
					pCommand->AddParameter(tokens[currentToken++]);
					++parameterCount;
				}

				if(parameterCount < pCommand->GetMinimumParameters()) {
					goto ParseFinnished;
				}

				state = Done;
				break;
			}

		case GetChannel:
			{
//				assert(pCommand != 0);

				std::wstring str = boost::trim_copy(tokens[currentToken]);
				std::vector<std::wstring> split;
				boost::split(split, str, boost::is_any_of("-"));
					
				int channelIndex = -1;
				int layerIndex = -1;
				try
				{
					channelIndex = boost::lexical_cast<int>(split[0]) - 1;

					if(split.size() > 1)
						layerIndex = boost::lexical_cast<int>(split[1]);
				}
				catch(...)
				{
					goto ParseFinnished;
				}

				std::shared_ptr<core::video_channel> pChannel = GetChannelSafe(channelIndex, channels_);
				if(pChannel == 0) {
					goto ParseFinnished;
				}

				pCommand->SetChannel(pChannel);
				pCommand->SetChannelIndex(channelIndex);
				pCommand->SetLayerIntex(layerIndex);

				state = GetParameters;
				++currentToken;
				break;
			}

		default:	//Done and unexpected
			goto ParseFinnished;
		}
	}

ParseFinnished:
	if(state == GetParameters && pCommand->GetMinimumParameters()==0)
		state = Done;

	if(state != Done) {
		pCommand.reset();
	}

	if(pOutState != 0) {
		*pOutState = state;
	}

	return pCommand;
}

bool AMCPProtocolStrategy::QueueCommand(AMCPCommandPtr pCommand) {
	if(pCommand->NeedChannel()) {
		unsigned int channelIndex = pCommand->GetChannelIndex() + 1;
		if(commandQueues_.size() > channelIndex) {
			commandQueues_[channelIndex]->AddCommand(pCommand);
		}
		else
			return false;
	}
	else {
		commandQueues_[0]->AddCommand(pCommand);
	}
	return true;
}

AMCPCommandPtr AMCPProtocolStrategy::CommandFactory(const std::wstring& str)
{
	std::wstring s = str;
	transform(s.begin(), s.end(), s.begin(), toupper);
	
	if	   (s == TEXT("MIXER"))			return std::make_shared<MixerCommand>();
	else if(s == TEXT("DIAG"))			return std::make_shared<DiagnosticsCommand>();
	else if(s == TEXT("CHANNEL_GRID"))	return std::make_shared<ChannelGridCommand>();
	else if(s == TEXT("CALL"))			return std::make_shared<CallCommand>();
	else if(s == TEXT("SWAP"))			return std::make_shared<SwapCommand>();
	else if(s == TEXT("LOAD"))			return std::make_shared<LoadCommand>();
	else if(s == TEXT("LOADBG"))		return std::make_shared<LoadbgCommand>();
	else if(s == TEXT("ADD"))			return std::make_shared<AddCommand>();
	else if(s == TEXT("REMOVE"))		return std::make_shared<RemoveCommand>();
	else if(s == TEXT("PAUSE"))			return std::make_shared<PauseCommand>();
	else if(s == TEXT("PLAY"))			return std::make_shared<PlayCommand>();
	else if(s == TEXT("STOP"))			return std::make_shared<StopCommand>();
	else if(s == TEXT("CLEAR"))			return std::make_shared<ClearCommand>();
	else if(s == TEXT("PRINT"))			return std::make_shared<PrintCommand>();
	else if(s == TEXT("LOG"))			return std::make_shared<LogCommand>();
	else if(s == TEXT("CG"))			return std::make_shared<CGCommand>();
	else if(s == TEXT("DATA"))			return std::make_shared<DataCommand>();
	else if(s == TEXT("CINF"))			return std::make_shared<CinfCommand>();
	else if(s == TEXT("INFO"))			return std::make_shared<InfoCommand>(channels_);
	else if(s == TEXT("CLS"))			return std::make_shared<ClsCommand>();
	else if(s == TEXT("TLS"))			return std::make_shared<TlsCommand>();
	else if(s == TEXT("VERSION"))		return std::make_shared<VersionCommand>();
	else if(s == TEXT("BYE"))			return std::make_shared<ByeCommand>();
	else if(s == TEXT("SET"))			return std::make_shared<SetCommand>();
	//else if(s == TEXT("MONITOR"))
	//{
	//	result = AMCPCommandPtr(new MonitorCommand());
	//}
	//else if(s == TEXT("KILL"))
	//{
	//	result = AMCPCommandPtr(new KillCommand());
	//}
	return nullptr;
}

std::size_t AMCPProtocolStrategy::TokenizeMessage(const std::wstring& message, std::vector<std::wstring>* pTokenVector)
{
	//split on whitespace but keep strings within quotationmarks
	//treat \ as the start of an escape-sequence: the following char will indicate what to actually put in the string

	std::wstring currentToken;

	char inQuote = 0;
	bool getSpecialCode = false;

	for(unsigned int charIndex=0; charIndex<message.size(); ++charIndex)
	{
		if(getSpecialCode)
		{
			//insert code-handling here
			switch(message[charIndex])
			{
			case TEXT('\\'):
				currentToken += TEXT("\\");
				break;
			case TEXT('\"'):
				currentToken += TEXT("\"");
				break;
			case TEXT('n'):
				currentToken += TEXT("\n");
				break;
			default:
				break;
			};
			getSpecialCode = false;
			continue;
		}

		if(message[charIndex]==TEXT('\\'))
		{
			getSpecialCode = true;
			continue;
		}

		if(message[charIndex]==' ' && inQuote==false)
		{
			if(currentToken.size()>0)
			{
				pTokenVector->push_back(currentToken);
				currentToken.clear();
			}
			continue;
		}

		if(message[charIndex]==TEXT('\"'))
		{
			inQuote ^= 1;

			if(currentToken.size()>0)
			{
				pTokenVector->push_back(currentToken);
				currentToken.clear();
			}
			continue;
		}

		currentToken += message[charIndex];
	}

	if(currentToken.size()>0)
	{
		pTokenVector->push_back(currentToken);
		currentToken.clear();
	}

	return pTokenVector->size();
}

}	//namespace amcp
}}	//namespace caspar