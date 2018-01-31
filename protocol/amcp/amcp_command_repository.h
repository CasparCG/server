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

#include "AMCPCommand.h"
#include "../util/ClientInfo.h"

#include <common/memory.h>

#include <core/fwd.h>

#include <future>
#include <functional>

namespace caspar { namespace protocol { namespace amcp {

class amcp_command_repository : boost::noncopyable
{
public:
	amcp_command_repository(
			const std::vector<spl::shared_ptr<core::video_channel>>& channels,
			const spl::shared_ptr<core::cg_producer_registry>& cg_registry,
			const spl::shared_ptr<const core::frame_producer_registry>& producer_registry,
			const spl::shared_ptr<const core::frame_consumer_registry>& consumer_registry,
			std::function<void(bool)> shutdown_server_now);

	AMCPCommand::ptr_type create_command(const std::wstring& s, IO::ClientInfoPtr client, std::list<std::wstring>& tokens) const;
	AMCPCommand::ptr_type create_channel_command(
			const std::wstring& s,
			IO::ClientInfoPtr client,
			unsigned int channel_index,
			int layer_index,
			std::list<std::wstring>& tokens) const;

	const std::vector<channel_context>& channels() const;

	void register_command(std::wstring category, std::wstring name, amcp_command_func command, int min_num_params);
	void register_channel_command(std::wstring category, std::wstring name, amcp_command_func command, int min_num_params);
private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}}
