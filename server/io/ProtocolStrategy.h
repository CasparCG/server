#ifndef _PROTOCOLSTRATEGY_H__
#define _PROTOCOLSTRATEGY_H__

#pragma once

#include <string>
#include "clientInfo.h"

namespace caspar {
namespace IO {

class IProtocolStrategy
{
public:
	virtual ~IProtocolStrategy()
	{}

	virtual void Parse(const TCHAR* pData, int charCount, ClientInfoPtr pClientInfo) = 0;
};

typedef std::tr1::shared_ptr<IProtocolStrategy> ProtocolStrategyPtr;

}	//namespace IO
}	//namespace caspar

#endif	//_PROTOCOLSTRATEGY_H__