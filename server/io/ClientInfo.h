#ifndef _CLIENTINFO_H__
#define _CLIENTINFO_H__

#pragma once

#include <string>

namespace caspar {
namespace IO {

class ClientInfo 
{
protected:
	ClientInfo()
	{}

public:
	virtual ~ClientInfo() {
	}

	virtual void Send(const tstring& data) = 0;
	virtual void Disconnect() = 0;

	tstring		currentMessage_;
};
typedef std::tr1::shared_ptr<ClientInfo> ClientInfoPtr;

}	//namespace IO
}	//namespace caspar

#endif	//_CLIENTINFO_H__