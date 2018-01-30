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

#include "AMCPCommand.h"

#include <common/executor.h>
#include <common/memory.h>
#include <common/timer.h>

#include <boost/property_tree/ptree_fwd.hpp>

#include <mutex>

namespace caspar { namespace protocol { namespace amcp {

class AMCPCommandQueue
{
	AMCPCommandQueue(const AMCPCommandQueue&);
	AMCPCommandQueue& operator=(const AMCPCommandQueue&);
public:
	typedef spl::shared_ptr<AMCPCommandQueue> ptr_type;

	AMCPCommandQueue(const std::wstring& name);
	~AMCPCommandQueue();

	void AddCommand(AMCPCommand::ptr_type pCommand);

	boost::property_tree::wptree info() const;

private:
	executor				executor_;
	mutable std::mutex   	running_command_mutex_;
	bool					running_command_		= false;
	std::wstring			running_command_name_;
	std::wstring			running_command_params_;
	caspar::timer			running_command_since_;
};

}}}
