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