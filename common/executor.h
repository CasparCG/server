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

#include <tbb/atomic.h>
#include <tbb/concurrent_priority_queue.h>
#include <tbb/concurrent_queue.h>

#include <boost/thread.hpp>

#include <functional>

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

class executor
{	
	struct priority_function
	{
		int						priority;
		std::function<void()>	func;

		priority_function()
		{
		}

		template<typename F>
		priority_function(int priority, F&& func)
			: priority(priority)
			, func(std::forward<F>(func))
		{
		}

		void operator()()
		{
			func();
		}

		bool operator<(const priority_function& other) const
		{
			return priority < other.priority;
		}
	};

	executor(const executor&);
	executor& operator=(const executor&);
	
	typedef tbb::concurrent_priority_queue<priority_function>	function_queue_t;

	tbb::atomic<bool>											is_running_;
	boost::thread												thread_;	
	function_queue_t											execution_queue_;
	tbb::concurrent_bounded_queue<int>							semaphore_;
		
public:		
	executor(const std::wstring& name) // noexcept
	{
		name; // TODO: Use to set thread name.
		is_running_ = true;
		thread_ = boost::thread([this]{run();});
	}
	
	virtual ~executor() // noexcept
	{
		stop();
		thread_.join();
	}
						
	template<typename Func>
	auto begin_invoke(Func&& func, task_priority priority = task_priority::normal_priority) -> boost::unique_future<decltype(func())> // noexcept
	{	
		if(execution_queue_.size() > 256)
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("executor overflow."));

		if(!is_running_)
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("executor not running."));
				
		typedef typename std::remove_reference<Func>::type	function_type;
		typedef decltype(func())							result_type;
		typedef boost::packaged_task<result_type>			task_type;
								
		std::unique_ptr<task_type> task;

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
		
		task->set_wait_callback(std::function<void(task_type&)>([=](task_type& my_task) // The std::function wrapper is required in order to add ::result_type to functor class.
		{
			try
			{
				if(is_current())  // Avoids potential deadlock.
					my_task();
			}
			catch(boost::task_already_started&){}
		}));
				
		auto future = task->get_future();

		auto raw_task = task.release();
		priority_function prio_func(priority.value(), [raw_task]
		{
			std::unique_ptr<task_type> task(raw_task);
			try
			{
				(*task)();
			}
			catch(boost::task_already_started&){}
		});

		execution_queue_.push(prio_func);
		semaphore_.push(0);
							
		return std::move(future);		
	}
	
	template<typename Func>
	auto invoke(Func&& func, task_priority prioriy = task_priority::normal_priority) -> decltype(func()) // noexcept
	{
		if(is_current())  // Avoids potential deadlock.
			return func();
		
		return begin_invoke(std::forward<Func>(func), prioriy).get();
	}

	void yield() // noexcept
	{
		if(!is_current())
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("Executor can only yield inside of thread context."));

		int dummy;
		semaphore_.pop(dummy);

		priority_function func;
		if(execution_queue_.try_pop(func))
			func();
	}

	void set_capacity(std::size_t capacity) // noexcept
	{
		semaphore_.set_capacity(capacity);
	}

	std::size_t capacity() const
	{
		return semaphore_.capacity();
	}
	
	void clear()
	{		
		priority_function func;
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

	void run() // noexcept
	{
		win32_exception::install_handler();		
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