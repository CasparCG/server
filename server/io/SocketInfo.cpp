#include "..\stdafx.h"
#include "SocketInfo.h"
#include "AsyncEventServer.h"

namespace caspar {
namespace IO {

SocketInfo::SocketInfo(SOCKET socket, AsyncEventServer* pServer) : socket_(socket), pServer_(pServer), recvLeftoverOffset_(0), currentlySendingOffset_(0)
{
	event_ = WSACreateEvent();
	if(event_ == WSA_INVALID_EVENT) {
		throw std::exception("Failed to create WSAEvent");
	}
}

SocketInfo::~SocketInfo() {

	WSACloseEvent(event_);
	event_ = 0;

	closesocket(socket_);
	socket_ = 0;
}

void SocketInfo::Send(const tstring& data) {
	if(data.length() > 0) {
		{
			//The lock has to be let go before DoSend is called since that too tries to lock to object
			Lock lock(*this);
			sendQueue_.push(data);
		}

		pServer_->DoSend(*this);
	}
}

void SocketInfo::Disconnect() {
	pServer_->DisconnectClient(*this);
}

}	//namespace IO
}	//namespace caspar