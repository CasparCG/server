/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include "..\stdafx.h"

#include "AMCPCommandQueue.h"

#include <boost/property_tree/ptree.hpp>

#include <common/concurrency/lock.h>

namespace caspar { namespace protocol { namespace amcp {

namespace {

tbb::spin_mutex& get_global_mutex()
{
	static tbb::spin_mutex mutex;

	return mutex;
}

std::map<std::wstring, AMCPCommandQueue*>& get_instances()
{
	static std::map<std::wstring, AMCPCommandQueue*> queues;

	return queues;
}

}
	
AMCPCommandQueue::AMCPCommandQueue(const std::wstring& name)
	: executor_(L"AMCPCommandQueue " + name)
	, running_command_(false)
{
	tbb::spin_mutex::scoped_lock lock(get_global_mutex());

	get_instances().insert(std::make_pair(name, this));
}

AMCPCommandQueue::~AMCPCommandQueue() 
{
	tbb::spin_mutex::scoped_lock lock(get_global_mutex());

	get_instances().erase(widen(executor_.name()));
}

void AMCPCommandQueue::AddCommand(AMCPCommandPtr pCurrentCommand)
{
	if(!pCurrentCommand)
		return;

	if(executor_.size() > 64)
	{
		try
		{
			CASPAR_LOG(error) << "AMCP Command Queue Overflow.";
			CASPAR_LOG(error) << "Failed to execute command:" << pCurrentCommand->print();
			pCurrentCommand->SetReplyString(L"500 FAILED\r\n");
			pCurrentCommand->SendReply();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
	}
	
	executor_.begin_invoke([=]
	{
		try
		{
			try
			{
				auto print = pCurrentCommand->print();
				auto params = pCurrentCommand->GetParameters().get_original_string();

				{
					tbb::spin_mutex::scoped_lock lock(running_command_mutex_);
					running_command_ = true;
					running_command_name_ = print;
					running_command_params_ = std::move(params);
					running_command_since_.restart();
				}

				if(pCurrentCommand->Execute()) 
					CASPAR_LOG(debug) << "Executed command: " << print;
				else 
					CASPAR_LOG(warning) << "Failed to execute command: " << print << L" on " << widen(executor_.name());
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				CASPAR_LOG(error) << "Failed to execute command:" << pCurrentCommand->print() << L" on " << widen(executor_.name());
				pCurrentCommand->SetReplyString(L"500 FAILED\r\n");
			}
				
			pCurrentCommand->SendReply();
			
			CASPAR_LOG(trace) << "Ready for a new command";

			tbb::spin_mutex::scoped_lock lock(running_command_mutex_);
			running_command_ = false;
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
	});
}

boost::property_tree::wptree AMCPCommandQueue::info() const
{
	boost::property_tree::wptree info;

	auto name = widen(executor_.name());
	info.add(L"name", name);
	auto size = executor_.size();
	info.add(L"queued", std::max(0, size));

	bool running_command;
	std::wstring running_command_name;
	std::wstring running_command_params;
	int64_t running_command_elapsed;

	lock(running_command_mutex_, [&]
	{
		running_command = running_command_;
		
		if (running_command)
		{
			running_command_name = running_command_name_;
			running_command_params = running_command_params_;
			running_command_elapsed = static_cast<int64_t>(
					running_command_since_.elapsed() * 1000.0);
		}
	});

	if (running_command)
	{
		info.add(L"running.command", running_command_name);
		info.add(L"running.params", running_command_params);
		info.add(L"running.elapsed", running_command_elapsed);
	}

	return info;
}

boost::property_tree::wptree AMCPCommandQueue::info_all_queues()
{
	boost::property_tree::wptree info;
	tbb::spin_mutex::scoped_lock lock(get_global_mutex());

	BOOST_FOREACH(auto& queue, get_instances())
	{
		info.add_child(L"queues.queue", queue.second->info());
	}

	return info;
}

}}}