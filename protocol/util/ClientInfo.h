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

#include <memory>
#include <string>
#include <iostream>

namespace caspar { namespace IO {

class ClientInfo 
{
protected:
	ClientInfo(){}

public:
	virtual ~ClientInfo(){}

	virtual void Send(const std::wstring& data) = 0;
	virtual void Disconnect() = 0;

	std::wstring		currentMessage_;
};
typedef std::shared_ptr<ClientInfo> ClientInfoPtr;

struct ConsoleClientInfo : public caspar::IO::ClientInfo 
{
	void Send(const std::wstring& data)
	{
		std::wcout << (L"\n--------------------\n" + data + L"--------------------\n");
	}
	void Disconnect(){}
};

}}
