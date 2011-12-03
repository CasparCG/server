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

#include "../stdafx.h"
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

void SocketInfo::Send(const std::wstring& data) {
	if(pServer_ != 0 && data.length() > 0) {
		{
			//The lock has to be let go before DoSend is called since that too tries to lock to object
			tbb::mutex::scoped_lock lock(mutex_);
			sendQueue_.push(data);
		}

		pServer_->DoSend(*this);
	}
}

void SocketInfo::Disconnect() {
	if(pServer_)
		pServer_->DisconnectClient(*this);
}

}	//namespace IO
}	//namespace caspar