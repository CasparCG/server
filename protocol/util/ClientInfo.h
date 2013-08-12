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

#include <memory>
#include <string>
#include <iostream>

#include <common/log.h>
#include "protocol_strategy.h"

namespace caspar { namespace IO {
/*
class ClientInfo 
{
protected:
	ClientInfo(){}

public:
	virtual ~ClientInfo(){}

	virtual void Send(const std::wstring& data) = 0;
	virtual void Disconnect() = 0;
	virtual std::wstring print() const = 0;
	virtual void add_lifecycle_bound_object(const std::wstring& key, const std::shared_ptr<void>& lifecycle_bound) = 0;
	virtual std::shared_ptr<void> remove_lifecycle_bound_object(const std::wstring& key) = 0;
};
*/
typedef spl::shared_ptr<client_connection<wchar_t>> ClientInfoPtr;

struct ConsoleClientInfo : public client_connection<wchar_t>
{
	virtual void send(std::wstring&& data)
	{
		std::wcout << (L"#" + caspar::log::replace_nonprintable_copy(data, L'?'));
	}
	virtual void disconnect() {}
	virtual std::wstring print() const {return L"Console";}
	virtual void add_lifecycle_bound_object(const std::wstring& key, const std::shared_ptr<void>& lifecycle_bound) {}
	virtual std::shared_ptr<void> remove_lifecycle_bound_object(const std::wstring& key) { return std::shared_ptr<void>(); }
};

}}
