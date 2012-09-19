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
* Author: Helge Norberg, helge.norberg@svt.se
*/

#pragma once

#include "ProtocolStrategy.h"

#include <functional>
#include <map>
#include <memory>

namespace caspar { namespace IO {

/**
 * A protocol strategy which creates unique protocol strategy instances per
 * client connection, using a supplied factory.
 *
 * This allows for the strategy implementation to only concern about one
 * client.
 */
class stateful_protocol_strategy_wrapper : public IProtocolStrategy
{
	std::function<ProtocolStrategyPtr ()> factory_;
	std::map<
		std::weak_ptr<ClientInfo>, 
		ProtocolStrategyPtr, 
		std::owner_less<std::weak_ptr<ClientInfo>>> strategies_;
	unsigned int codepage_;
public:
	stateful_protocol_strategy_wrapper(
		const std::function<ProtocolStrategyPtr ()>& factory);

	virtual void Parse(
		const wchar_t* pData, int charCount, ClientInfoPtr pClientInfo);

	virtual unsigned int GetCodepage();
private:
	void purge_dead_clients();
};

}}
