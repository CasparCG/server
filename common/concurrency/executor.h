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

#include "../exception/win32_exception.h"
#include "../exception/exceptions.h"
#include "../utility/string.h"
#include "../utility/move_on_copy.h"
#include "../log/log.h"

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>

#include <boost/thread.hpp>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>

#include <functional>

namespace caspar {

namespace detail {

typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // must be 0x1000
	LPCSTR szName; // pointer to name (in user addr space)
	DWORD dwThreadID; // thread ID (-1=caller thread)
	DWORD dwFlags; // reserved for future use, must be zero
} THREADNAME_INFO;

inline void SetThreadName(DWORD dwThreadID, LPCSTR szThreadName)
{
	THREADNAME_INFO info;
	{
		info.dwType = 0x1000;
		info.szName = szThreadName;
		info.dwThreadID = dwThreadID;
		info.dwFlags = 0;
	}
	__try
	{
		RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (DWORD*)&info );
	}
	__except (EXCEPTION_CONTINUE_EXECUTION){}	
}

}

enum task_priority
{
	high_priority,
	normal_priority,
	priority_count
};

enum thread_priority
{
	high_priority_class,
	above_normal_priority_class,
	normal_priority_class,
	below_normal_priority_class
};

class executor : boost::noncopyable
{
	const std::string name_;
	boost::thread thread_;
	tbb::atomic<bool> is_running_;
	tbb::atomic<bool> execute_rest_;
	
	typedef tbb::concurrent_bounded_queue<std::function<void()>> function_queue;
	function_queue execution_queue_[priority_count];
		
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
		
	explicit executor(const std::wstring& name) : name_(narrow(name)) // noexcept
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
		execution_queue_[normal_priority].set_capacity(capacity);
	}

	void set_priority_class(thread_priority p)
	{
		begin_invoke([=]
		{
			if(p == high_priority_class)
				SetThreadPriority(GetCurrentThread(), HIGH_PRIORITY_CLASS);
			else if(p == above_normal_priority_class)
				SetThreadPriority(GetCurrentThread(), ABOVE_NORMAL_PRIORITY_CLASS);
			else if(p == normal_priority_class)
				SetThreadPriority(GetCurrentThread(), NORMAL_PRIORITY_CLASS);
			else if(p == below_normal_priority_class)
				SetThreadPriority(GetCurrentThread(), BELOW_NORMAL_PRIORITY_CLASS);
		});
	}
	
	void clear()
	{		
		std::function<void()> func;
		while(execution_queue_[normal_priority].try_pop(func));
		while(execution_queue_[high_priority].try_pop(func));
	}
				
	void stop() // noexcept
	{
		execute_rest_ = false;
		is_running_ = false;	
		execution_queue_[normal_priority].try_push([]{}); // Wake the execution thread.
	}
				
	void stop_execute_rest() // noexcept
	{
		execute_rest_ = true;
		is_running_ = false;
		execution_queue_[normal_priority].try_push([]{}); // Wake the execution thread.
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
	auto try_begin_invoke(Func&& func, task_priority priority = normal_priority) -> boost::optional<caspar::move_on_copy<boost::unique_future<decltype(func())>>>
	{	
		if(!is_running_)
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("executor not running."));

		// Create a move on copy adaptor to avoid copying the functor into the queue, tbb::concurrent_queue does not support move semantics.
		auto task_adaptor = make_move_on_copy(create_task(func));

		auto future = task_adaptor.value.get_future();

		// Enable the cancellation of the task if priority is other than
		// normal, because there are two queues to try_push to. Either both
		// succeed or nothing will be executed.
		
		boost::promise<bool> cancelled_promise;
		boost::shared_future<bool> cancelled(cancelled_promise.get_future());

		bool was_enqueued = execution_queue_[priority].try_push([=]() mutable
		{
			// Wait until we know if we should cancel execution or not.
			if (cancelled.get())
				return;

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

		if (!was_enqueued)
			return boost::optional<caspar::move_on_copy<boost::unique_future<decltype(func())>>>();

		if (priority != normal_priority)
		{
			was_enqueued = execution_queue_[normal_priority].try_push(nullptr);

			if (was_enqueued)
			{
				// Now we know that both enqueue operations has succeeded.
				cancelled_promise.set_value(false);
			}
			else
			{
				cancelled_promise.set_value(true); // The actual task has already been
				                                   // queued so we cancel it.

				return boost::optional<caspar::move_on_copy<boost::unique_future<decltype(func())>>>();
			}
		}
		else
		{
			cancelled_promise.set_value(false);
		}

		return caspar::make_move_on_copy(std::move(future));
	}
				
	template<typename Func>
	auto begin_invoke(Func&& func, task_priority priority = normal_priority) -> boost::unique_future<decltype(func())> // noexcept
	{	
		if(!is_running_)
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("executor not running."));

		// Create a move on copy adaptor to avoid copying the functor into the queue, tbb::concurrent_queue does not support move semantics.
		auto task_adaptor = make_move_on_copy(create_task(func));

		auto future = task_adaptor.value.get_future();

		execution_queue_[priority].push([=]
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

		if(priority != normal_priority)
			execution_queue_[normal_priority].push(nullptr);
					
		return std::move(future);		
	}
	
	template<typename Func>
	auto invoke(Func&& func, task_priority prioriy = normal_priority) -> decltype(func()) // noexcept
	{
		if(boost::this_thread::get_id() == thread_.get_id())  // Avoids potential deadlock.
			return func();
		
		return begin_invoke(std::forward<Func>(func), prioriy).get();
	}
	
	void yield() // noexcept
	{
		if(boost::this_thread::get_id() != thread_.get_id())  // Only yield when calling from execution thread.
			return;

		std::function<void()> func;
		while(execution_queue_[high_priority].try_pop(func))
		{
			if(func)
				func();
		}	
	}
	
	function_queue::size_type capacity() const /*noexcept*/ { return execution_queue_[normal_priority].capacity();	}
	function_queue::size_type size() const /*noexcept*/ { return execution_queue_[normal_priority].size();	}
	bool empty() const /*noexcept*/	{ return execution_queue_[normal_priority].empty();	}
	bool is_running() const /*noexcept*/ { return is_running_; }	
		
private:
	
	void execute() // noexcept
	{
		std::function<void()> func;
		execution_queue_[normal_priority].pop(func);	

		yield();

		if(func)
			func();
	}

	void execute_rest(task_priority priority) // noexcept
	{
		std::function<void()> func;

		while (execution_queue_[priority].try_pop(func))
			if (func)
				func();
	}

	void run() // noexcept
	{
		win32_exception::install_handler();		
		detail::SetThreadName(GetCurrentThreadId(), name_.c_str());
		while(is_running_)
		{
			try
			{
				execute();
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		}

		if (execute_rest_)
		{
			execute_rest(high_priority);
			execute_rest(normal_priority);
		}
	}	
};

}