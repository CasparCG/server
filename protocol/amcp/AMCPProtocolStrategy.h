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

#pragma once

#include "../util/protocolstrategy.h"
#include <core/video_channel.h>

#include "AMCPCommand.h"
#include "AMCPCommandQueue.h"

#include <boost/noncopyable.hpp>

#include <string>

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
	AMCPProtocolStrategy(const std::vector<spl::shared_ptr<core::video_channel>>& channels);
	virtual ~AMCPProtocolStrategy();

	virtual void Parse(const TCHAR* pData, int charCount, IO::ClientInfoPtr pClientInfo);
	virtual std::string GetCodepage() {
		return "UTF-8";
	}

	AMCPCommandPtr InterpretCommandString(const std::wstring& str, MessageParserState* pOutState=0);

private:
	friend class AMCPCommand;

	void ProcessMessage(const std::wstring& message, IO::ClientInfoPtr& pClientInfo);
	std::size_t TokenizeMessage(const std::wstring& message, std::vector<std::wstring>* pTokenVector);
	AMCPCommandPtr CommandFactory(const std::wstring& str);

	bool QueueCommand(AMCPCommandPtr);

	std::vector<spl::shared_ptr<core::video_channel>> channels_;
	std::vector<AMCPCommandQueuePtr> commandQueues_;
	static const std::wstring MessageDelimiter;
};

}}}
