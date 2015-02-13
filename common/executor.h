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
* Author: Robert Nagy, ronag89@gmail.com
*/

#pragma once

#include "except.h"
#include "enum_class.h"
#include "log.h"
#include "blocking_bounded_queue_adapter.h"
#include "blocking_priority_queue.h"
#include "future.h"

#include <tbb/atomic.h>
#include <tbb/concurrent_priority_queue.h>

#include <boost/thread.hpp>
#include <boost/optional.hpp>

#include <functional>
#include <future>

namespace caspar {
		
struct task_priority_def
{
	enum type
	{
		lowest_priority = 0,
		lower_priority,
		low_priority,
		normal_priority,
		high_priority,
		higher_priority
	};
};
typedef enum_class<task_priority_def> task_priority;

class executor /* final */
{	
	executor(const executor&);
	executor& operator=(const executor&);
	
	typedef blocking_priority_queue<std::function<void()>, task_priority>	function_queue_t;
	
	const std::wstring											name_;
	tbb::atomic<bool>											is_running_;
	boost::thread												thread_;	
	function_queue_t											execution_queue_;
		
public:		
	executor(const std::wstring& name)
		: name_(name)
		, execution_queue_(512, std::vector<task_priority> {
			task_priority::lowest_priority,
			task_priority::lower_priority,
			task_priority::low_priority,
			task_priority::normal_priority,
			task_priority::high_priority,
			task_priority::higher_priority 
		})
	{
		is_running_ = true;
		thread_ = boost::thread([this]{run();});
	}
	
	~executor()
	{
		try
		{
			internal_begin_invoke([=]
			{
				is_running_ = false;
			}).wait();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
		
		thread_.join();
	}

	template<typename Func>
	auto begin_invoke(Func&& func, task_priority priority = task_priority::normal_priority) -> std::future<decltype(func())> // noexcept
	{	
		if(!is_running_)
			CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("executor not running.") << source_info(name_));
				
		return internal_begin_invoke(std::forward<Func>(func), priority);	
	}
	
	template<typename Func>
	auto invoke(Func&& func, task_priority prioriy = task_priority::normal_priority) -> decltype(func()) // noexcept
	{
		if(is_current())  // Avoids potential deadlock.
			return func();
		
		return begin_invoke(std::forward<Func>(func), prioriy).get();
	}

	void yield(task_priority minimum_priority)
	{
		if(!is_current())
			CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("Executor can only yield inside of thread context.")  << source_info(name_));

		std::function<void ()> func;

		while (execution_queue_.try_pop(func, minimum_priority))
			func();
	}

	void set_capacity(function_queue_t::size_type capacity)
	{
		execution_queue_.set_capacity(capacity);
	}

	function_queue_t::size_type capacity() const
	{
		return execution_queue_.capacity();
	}

	bool is_full() const
	{
		return execution_queue_.space_available() == 0;
	}
	
	void clear()
	{		
		std::function<void ()> func;
		while(execution_queue_.try_pop(func));
	}
				
	void stop()
	{
		invoke([this]
		{
			is_running_ = false;
		});
	}

	void wait()
	{
		invoke([]{}, task_priority::lowest_priority);
	}
		
	function_queue_t::size_type size() const 
	{
		return execution_queue_.size();	
	}
		
	bool is_running() const
	{
		return is_running_; 
	}	

	bool is_current() const
	{
		return boost::this_thread::get_id() == thread_.get_id();
	}
		
private:	

	std::wstring print() const
	{
		return L"executor[" + name_ + L"]";
	}
	
	template<typename Func>
	auto internal_begin_invoke(
		Func&& func,
		task_priority priority = task_priority::normal_priority) -> std::future<decltype(func())> // noexcept
	{					
		typedef typename std::remove_reference<Func>::type	function_type;
		typedef decltype(func())							result_type;
		typedef std::packaged_task<result_type()>			task_type;
								
		std::shared_ptr<task_type> task;

		// Use pointers since the boost thread library doesn't fully support move semantics.

		auto raw_func2 = new function_type(std::forward<Func>(func));
		try
		{
			task.reset(new task_type([raw_func2]() -> result_type
			{
				std::unique_ptr<function_type> func2(raw_func2);
				return (*func2)();
			}));
		}
		catch(...)
		{
			delete raw_func2;
			throw;
		}
				
		auto future = task->get_future().share();
		auto function = [task]
		{
			try
			{
				(*task)();
			}
			catch(std::future_error&){}
		};

		if (!execution_queue_.try_push(priority, function))
		{
			CASPAR_LOG(debug) << print() << L" Overflow. Blocking caller.";
			execution_queue_.push(priority, function);
		}

		return std::async(std::launch::deferred, [=]() mutable -> result_type
		{
			if (!is_ready(future) && is_current()) // Avoids potential deadlock.
			{
				function();
			}

			return future.get();
		});
	}

	void run() // noexcept
	{
		win32_exception::install_handler();		
		while(is_running_)
		{
			try
			{
				std::function<void ()> func;
				execution_queue_.pop(func);
				func();
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		}
	}	
};

}