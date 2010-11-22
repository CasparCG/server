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
 
#ifndef _AMCPMESSAGEHANDLER_H__
#define _AMCPMESSAGEHANDLER_H__
#pragma once

#include <string>
#include <vector>

#include "..\io\protocolstrategy.h"
#include "AMCPCommand.h"
#include "AMCPCommandQueue.h"

namespace caspar {
namespace amcp {


class AMCPProtocolStrategy : public caspar::IO::IProtocolStrategy
{
	enum MessageParserState {
		New = 0,
		GetSwitch,
		GetCommand,
		GetParameters,
		GetChannel,
		Done
	};

	AMCPProtocolStrategy(const AMCPProtocolStrategy&);
	AMCPProtocolStrategy& operator=(const AMCPProtocolStrategy&);

public:
	AMCPProtocolStrategy();
	virtual ~AMCPProtocolStrategy();

	virtual void Parse(const TCHAR* pData, int charCount, caspar::IO::ClientInfoPtr pClientInfo);
	virtual UINT GetCodepage() {
		return CP_UTF8;
	}

	static AMCPCommandPtr InterpretCommandString(const tstring& str, MessageParserState* pOutState=0);

private:
	friend class AMCPCommand;

	void ProcessMessage(const tstring& message, caspar::IO::ClientInfoPtr& pClientInfo);
	static std::size_t TokenizeMessage(const tstring& message, std::vector<tstring>* pTokenVector);
	static AMCPCommandPtr CommandFactory(const tstring& str);

	bool QueueCommand(AMCPCommandPtr);

	std::vector<AMCPCommandQueuePtr> commandQueues_;
	static const tstring MessageDelimiter;
};

}	//namespace amcp
}	//namespace caspar

#endif	//_AMCPMESSAGEHANDLER_H__