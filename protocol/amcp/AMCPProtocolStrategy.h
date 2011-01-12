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
#pragma once

#include "../util/protocolstrategy.h"
#include <core/channel.h>

#include "AMCPCommand.h"
#include "AMCPCommandQueue.h"

#include <boost/noncopyable.hpp>

namespace caspar { namespace protocol { namespace amcp {

class AMCPProtocolStrategy : public IO::IProtocolStrategy, boost::noncopyable
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
	AMCPProtocolStrategy(const std::vector<safe_ptr<core::channel>>& channels);
	virtual ~AMCPProtocolStrategy();

	virtual void Parse(const TCHAR* pData, int charCount, IO::ClientInfoPtr pClientInfo);
	virtual UINT GetCodepage() {
		return CP_UTF8;
	}

	AMCPCommandPtr InterpretCommandString(const std::wstring& str, MessageParserState* pOutState=0);

private:
	friend class AMCPCommand;

	void ProcessMessage(const std::wstring& message, IO::ClientInfoPtr& pClientInfo);
	std::size_t TokenizeMessage(const std::wstring& message, std::vector<std::wstring>* pTokenVector);
	AMCPCommandPtr CommandFactory(const std::wstring& str);

	bool QueueCommand(AMCPCommandPtr);

	std::vector<safe_ptr<core::channel>> channels_;
	std::vector<AMCPCommandQueuePtr> commandQueues_;
	static const std::wstring MessageDelimiter;
};

}}}
