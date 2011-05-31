#include "..\StdAfx.h"
#include "..\Application.h"

#include "AMCPProtocolStrategy.h"

#include "..\io\AsyncEventServer.h"
#include "AMCPCommandsImpl.h"
#include "..\Channel.h"

#include <stdio.h>
#include <crtdbg.h>
#include <string.h>
#include <algorithm>
#include <cctype>

namespace caspar {
namespace amcp {

using namespace utils;
using IO::ClientInfoPtr;

const tstring AMCPProtocolStrategy::MessageDelimiter = TEXT("\r\n");

AMCPProtocolStrategy::AMCPProtocolStrategy() {
	AMCPCommandQueuePtr pGeneralCommandQueue(new AMCPCommandQueue());
	if(!pGeneralCommandQueue->Start()) {
		LOG << TEXT("Failed to start the general command-queue");

		//TODO: THROW!
	}
	else
		commandQueues_.push_back(pGeneralCommandQueue);


	ChannelPtr pChannel;
	unsigned int index = -1;
	//Create a commandpump for each channel
	while((pChannel = GetApplication()->GetChannel(++index)) != 0) {
		AMCPCommandQueuePtr pChannelCommandQueue(new AMCPCommandQueue());
		tstring title = TEXT("CHANNEL ");

		//HACK: Perform real conversion from int to string
		TCHAR num = TEXT('1')+static_cast<TCHAR>(index);
		title += num;

		if(!pChannelCommandQueue->Start()) {
			tstring logString = TEXT("Failed to start command-queue for ");
			logString += title;
			LOG << logString;

			//TODO: THROW!
		}
		else
			commandQueues_.push_back(pChannelCommandQueue);
	}
}

AMCPProtocolStrategy::~AMCPProtocolStrategy() {
}

void AMCPProtocolStrategy::Parse(const TCHAR* pData, int charCount, ClientInfoPtr pClientInfo)
{
	size_t pos;
	tstring recvData(pData, charCount);
	tstring availibleData = pClientInfo->currentMessage_ + recvData;

	while(true) {
		pos = availibleData.find(MessageDelimiter);
		if(pos != tstring::npos)
		{
			tstring message = availibleData.substr(0,pos);

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
	pClientInfo->currentMessage_ = availibleData;
}

void AMCPProtocolStrategy::ProcessMessage(const tstring& message, ClientInfoPtr& pClientInfo)
{
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
		tstringstream answer;
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

AMCPCommandPtr AMCPProtocolStrategy::InterpretCommandString(const tstring& message, MessageParserState* pOutState)
{
	std::vector<tstring> tokens;
	unsigned int currentToken = 0;
	tstring commandSwitch;

	AMCPCommandPtr pCommand;
	MessageParserState state = New;

	LOG << message;

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

				int channelIndex = _ttoi(tokens[currentToken].c_str())-1;

				ChannelPtr pChannel = GetApplication()->GetChannel(channelIndex);
				if(pChannel == 0) {
					goto ParseFinnished;
				}

				pCommand->SetChannel(pChannel);
				pCommand->SetChannelIndex(channelIndex);

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

AMCPCommandPtr AMCPProtocolStrategy::CommandFactory(const tstring& str)
{
	tstring s = str;
	transform(s.begin(), s.end(), s.begin(), toupper);

	AMCPCommandPtr result;

	if(s == TEXT("LOAD"))
	{
		result = AMCPCommandPtr(new LoadCommand());
	}
	else if(s == TEXT("LOADBG"))
	{
		result = AMCPCommandPtr(new LoadbgCommand());
	}
	else if(s == TEXT("PLAY"))
	{
		result = AMCPCommandPtr(new PlayCommand());
	}
	else if(s == TEXT("STOP"))
	{
		result = AMCPCommandPtr(new StopCommand());
	}
	else if(s == TEXT("CLEAR"))
	{
		result = AMCPCommandPtr(new ClearCommand());
	}
	else if(s == TEXT("PARAM"))
	{
		result = AMCPCommandPtr(new ParamCommand());
	}
	else if(s == TEXT("CG"))
	{
		result = AMCPCommandPtr(new CGCommand());
	}
	else if(s == TEXT("DATA"))
	{
		result = AMCPCommandPtr(new DataCommand());
	}
	else if(s == TEXT("CINF"))
	{
		result = AMCPCommandPtr(new CinfCommand());
	}
	else if(s == TEXT("INFO"))
	{
		result = AMCPCommandPtr(new InfoCommand());
	}
	else if(s == TEXT("CLS"))
	{
		result = AMCPCommandPtr(new ClsCommand());
	}
	else if(s == TEXT("TLS"))
	{
		result = AMCPCommandPtr(new TlsCommand());
	}
	else if(s == TEXT("VERSION"))
	{
		result = AMCPCommandPtr(new VersionCommand());
	}
	else if(s == TEXT("BYE"))
	{
		result = AMCPCommandPtr(new ByeCommand());
	}
	//else if(s == TEXT("KILL"))
	//{
	//	result = AMCPCommandPtr(new KillCommand());
	//}
	return result;
}

std::size_t AMCPProtocolStrategy::TokenizeMessage(const tstring& message, std::vector<tstring>* pTokenVector)
{
	//split on whitespace but keep strings within quotationmarks
	//treat \ as the start of an escape-sequence: the following char will indicate what to actually put in the string

	tstring currentToken;

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
}	//namespace caspar