#pragma once

#include "..\utils\lockable.h"
#include "ClientInfo.h"
#include <queue>
#include <vector>
#include "..\utils\databuffer.h"

namespace caspar {
namespace IO {

class AsyncEventServer;
class SocketInfo : public ClientInfo, private utils::LockableObject
{
	SocketInfo(const SocketInfo&);
	SocketInfo& operator=(const SocketInfo&);

public:
	SocketInfo(SOCKET, AsyncEventServer*);
	virtual ~SocketInfo();

	void Send(const tstring& data);
	void Disconnect();

	SOCKET			socket_;
	HANDLE			event_;
	tstring	host_;

private:
	friend class AsyncEventServer;
	std::queue<tstring> sendQueue_;
	AsyncEventServer* pServer_;

	caspar::utils::DataBuffer<char> currentlySending_;
	unsigned int currentlySendingOffset_;

	caspar::utils::DataBuffer<wchar_t> wideRecvBuffer_;
	char recvBuffer_[512];
	int recvLeftoverOffset_;
};
typedef std::tr1::shared_ptr<SocketInfo> SocketInfoPtr;

}	//namespace IO
}	//namespace caspar