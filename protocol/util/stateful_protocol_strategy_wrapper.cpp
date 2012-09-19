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

#include "../StdAfx.h"

#include "stateful_protocol_strategy_wrapper.h"

namespace caspar { namespace IO {

stateful_protocol_strategy_wrapper::stateful_protocol_strategy_wrapper(
	const std::function<ProtocolStrategyPtr ()>& factory)
	: factory_(factory)
	, codepage_(factory()->GetCodepage())
{
}

void stateful_protocol_strategy_wrapper::purge_dead_clients()
{
	for (auto it = strategies_.begin(); it != strategies_.end();)
	{
		auto strong = it->first.lock();

		if (strong)
			++it;
		else
			it = strategies_.erase(it);
	}
}

void stateful_protocol_strategy_wrapper::Parse(
	const wchar_t* pData, int charCount, ClientInfoPtr pClientInfo)
{
	purge_dead_clients();

	std::weak_ptr<ClientInfo> weak(pClientInfo);
	ProtocolStrategyPtr strategy;
	auto strategy_iter = strategies_.find(weak);

	if (strategy_iter == strategies_.end())
	{
		strategy = factory_();
		strategies_.insert(std::make_pair(weak, strategy));
	}
	else
	{
		strategy = strategy_iter->second;
	}

	strategy->Parse(pData, charCount, pClientInfo);
}

unsigned int stateful_protocol_strategy_wrapper::GetCodepage()
{
	return codepage_;
}

}}
