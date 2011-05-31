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