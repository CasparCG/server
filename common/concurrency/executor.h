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

#include "../except.h"
#include "../log.h"
#include "../enum_class.h"
#include "../utility/move_on_copy.h"
#include "../os/windows/windows.h"

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>

#include <boost/thread.hpp>

#include <functional>

namespace caspar {
	
struct task_priority_def
{
	enum type
	{
		high_priority,
		normal_priority,
		priority_count
	};
};
typedef enum_class<task_priority_def> task_priority;

struct thread_priority_def
{
	enum type
	{
		high_priority_class,
		above_normal_priority_class,
		normal_priority_class,
		below_normal_priority_class
	};
};
typedef enum_class<thread_priority_def> thread_priority;

class executor
{
	executor(const executor&);
	executor& operator=(const executor&);

	const std::string name_;
	boost::thread thread_;
	tbb::atomic<bool> is_running_;
	
	typedef tbb::concurrent_bounded_queue<std::function<void()>> function_queue;
	function_queue execution_queue_[task_priority::priority_count];
		
	template<typename Func>
	auto create_task(Func&& func) -> boost::packaged_task<decltype(func())> // noexcept
	{	
		typedef boost::packaged_task<decltype(func())> task_type;
				
		auto task = task_type(std::forward<Func>(func));
		
		task.set_wait_callback(std::function<void(task_type&)>([=](task_type& my_task) // The std::function wrapper is required in order to add ::result_type to functor class.
		{
			try
			{
				if(boost::this_thread::get_id() == thread_.get_id())  // Avoids potential deadlock.
					my_task();
			}
			catch(boost::task_already_started&){}
		}));
				
		return std::move(task);
	}

public:
		
	explicit executor(const std::wstring& name) : name_(u8(name)) // noexcept
	{
		is_running_ = true;
		thread_ = boost::thread([this]{run();});
	}
	
	virtual ~executor() // noexcept
	{
		stop();
		join();
	}

	void set_capacity(size_t capacity) // noexcept
	{
		execution_queue_[task_priority::normal_priority].set_capacity(capacity);
	}

	void set_priority_class(thread_priority p)
	{
		begin_invoke([=]
		{
			if(p == thread_priority::high_priority_class)
				SetThreadPriority(GetCurrentThread(), HIGH_PRIORITY_CLASS);
			else if(p == thread_priority::above_normal_priority_class)
				SetThreadPriority(GetCurrentThread(), ABOVE_NORMAL_PRIORITY_CLASS);
			else if(p == thread_priority::normal_priority_class)
				SetThreadPriority(GetCurrentThread(), NORMAL_PRIORITY_CLASS);
			else if(p == thread_priority::below_normal_priority_class)
				SetThreadPriority(GetCurrentThread(), BELOW_NORMAL_PRIORITY_CLASS);
		});
	}
	
	void clear()
	{		
		std::function<void()> func;
		while(execution_queue_[task_priority::normal_priority].try_pop(func));
		while(execution_queue_[task_priority::high_priority].try_pop(func));
	}
				
	void stop() // noexcept
	{
		is_running_ = false;	
		execution_queue_[task_priority::normal_priority].try_push([]{}); // Wake the execution thread.
	}

	void wait() // noexcept
	{
		invoke([]{});
	}

	void join()
	{
		if(boost::this_thread::get_id() != thread_.get_id())
			thread_.join();
	}
				
	template<typename Func>
	auto begin_invoke(Func&& func, task_priority priority = task_priority::normal_priority) -> boost::unique_future<decltype(func())> // noexcept
	{	
		if(!is_running_)
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("executor not running."));

		// Create a move on copy adaptor to avoid copying the functor into the queue, tbb::concurrent_queue does not support move semantics.
		auto task_adaptor = make_move_on_copy(create_task(func));

		auto future = task_adaptor.value.get_future();

		execution_queue_[priority.value()].push([=]
		{
			try
			{
				task_adaptor.value();
			}
			catch(boost::task_already_started&)
			{
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		});

		if(priority != task_priority::normal_priority)
			execution_queue_[task_priority::normal_priority].push(nullptr);
					
		return std::move(future);		
	}
	
	template<typename Func>
	auto invoke(Func&& func, task_priority prioriy = task_priority::normal_priority) -> decltype(func()) // noexcept
	{
		if(boost::this_thread::get_id() == thread_.get_id())  // Avoids potential deadlock.
			return func();
		
		return begin_invoke(std::forward<Func>(func), prioriy).get();
	}

	void yield() // noexcept
	{
		if(boost::this_thread::get_id() != thread_.get_id())
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("Executor can only yield inside of thread context."));

		std::function<void()> func;
		execution_queue_[task_priority::normal_priority].pop(func);	
		
		std::function<void()> func2;
		while(execution_queue_[task_priority::high_priority].try_pop(func2))
		{
			if(func2)
				func2();
		}	

		if(func)
			func();
	}
		
	function_queue::size_type size() const /*noexcept*/
	{
		return execution_queue_[task_priority::normal_priority].size() + execution_queue_[task_priority::high_priority].size();	
	}
		
	bool is_running() const /*noexcept*/ { return is_running_; }	
		
private:	

	void run() // noexcept
	{
		win32_exception::install_handler();		
		//detail::SetThreadName(GetCurrentThreadId(), name_.c_str());
		while(is_running_)
		{
			try
			{
				yield();
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		}
	}	
};

}