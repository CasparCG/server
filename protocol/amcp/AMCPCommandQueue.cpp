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

#include "../StdAfx.h"

#include "AMCPCommandQueue.h"

#include <boost/property_tree/ptree.hpp>

#include <cmath>

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
{
	tbb::spin_mutex::scoped_lock lock(get_global_mutex());

	get_instances().insert(std::make_pair(name, this));
}

AMCPCommandQueue::~AMCPCommandQueue()
{
	tbb::spin_mutex::scoped_lock lock(get_global_mutex());

	get_instances().erase(executor_.name());
}

void AMCPCommandQueue::AddCommand(AMCPCommand::ptr_type pCurrentCommand)
{
	if(!pCurrentCommand)
		return;
	
	if(executor_.size() > 128)
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
			caspar::timer timer;

			try
			{
				auto print = pCurrentCommand->print();
				auto params = boost::join(pCurrentCommand->parameters(), L" ");

				{
					tbb::spin_mutex::scoped_lock lock(running_command_mutex_);
					running_command_ = true;
					running_command_name_ = print;
					running_command_params_ = std::move(params);
					running_command_since_.restart();
				}

				if (pCurrentCommand->Execute())
					CASPAR_LOG(debug) << "Executed command: " << print << " " << timer.elapsed();
				else
					CASPAR_LOG(warning) << "Failed to execute command: " << print << " " << timer.elapsed();
			}
			catch (file_not_found&)
			{
				CASPAR_LOG(error) << L"File not found. No match found for parameters. Check syntax.";
				pCurrentCommand->SetReplyString(L"404 " + pCurrentCommand->print() + L" FAILED\r\n");
			}
			catch (std::out_of_range&)
			{
				CASPAR_LOG(error) << L"Missing parameter. Check syntax.";
				pCurrentCommand->SetReplyString(L"402 " + pCurrentCommand->print() + L" FAILED\r\n");
			}
			catch (...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				CASPAR_LOG(warning) << "Failed to execute command:" << pCurrentCommand->print() << " " << timer.elapsed();
				pCurrentCommand->SetReplyString(L"501 " + pCurrentCommand->print() + L" FAILED\r\n");
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

	auto name = executor_.name();
	info.add(L"name", name);
	auto size = executor_.size();
	info.add(L"queued", std::max(0u, size));

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

	for (auto& queue : get_instances())
	{
		info.add_child(L"queues.queue", queue.second->info());
	}

	return info;
}

}}}
