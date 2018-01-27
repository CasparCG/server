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

#include "os/general_protection_fault.h"
#include "except.h"
#include "future.h"

#include <atomic>
#include <functional>
#include <future>
#include <thread>
#include <deque>
#include <mutex>
#include <condition_variabl>

class executor final
{
	typedef std::shared_ptr<std::function<void()>> func_t;

	const std::wstring  	 name_;
	std::atomic<bool>   	 running_;
	std::deque<func_t>		 queue_;
	std::atomic<std::size_t> queue_capacity_ = std::numeric_limits<std::size_t>::max();
	std::mutex          	 queue_mutex_;
	std::condition_variable  queue_cond_;
	std::thread         	 thread_;

public:
	executor(const std::wstring& name)
		: name_(name)
		, thread_([this]{run();}))
	{
		running_ = true;
	}

	executor(const executor&) = delete;

	~executor()
	{
		running_ = false;
		thread_.join();
	}

	executor& operator=(const executor&) = delete;

	template<typename Func>
	auto begin_invoke(Func&& func)
	{
		if(!running_) {
			CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("executor not running.") << source_info(name_));
		}

		return internal_begin_invoke(std::forward<Func>(func), priority);
	}

	template<typename Func>
	auto invoke(Func&& func)
	{
		if(is_current()) {
			return func();
		}

		return begin_invoke(std::forward<Func>(func)).get();
	}

	void set_capacity(std::size_t capacity)
	{
		queue_capacity_ = capacity;
	}

	std::size_t capacity() const
	{
		return queue_capacity_;
	}

	void clear()
	{
		{
			std::lock_guard<std::mutex> lock(queue_mutex_);
			queue_.clear();
		}
		queue_cond_.notify_all();
	}

	void stop()
	{
		running_ = false;
		queue_cond_.notify_one();
	}

	void join()
	{
		thread_.join();
	}

	void wait()
	{
		invoke([]{});
	}

	std::size_t size() const
	{
		std::lock_guard<std::mutex> lock(queue_mutex_);
		return queue_.size();
	}

	bool is_running() const
	{
		return running_;
	}

	bool is_current() const
	{
		return std::this_thread::get_id() == thread_.get_id();
	}

	std::wstring name() const
	{
		return name_;
	}

private:

	std::wstring print() const
	{
		return L"executor[" + name_ + L"]";
	}

	template<typename Func>
	auto internal_begin_invoke(Func&& func)
	{
		typedef decltype(func())                  result_type;
		typedef std::packaged_task<result_type()> task_type;

 		auto task 	= task_type(std::forward<Func>(func));
		auto future = task.get_future();
		auto func 	= std::make_shared<func_t>(std::move(task));

		{
			std::unique_lock<std::mutex> lock(queue_mutex_);
			queue_cond_.wait([&] { return queue_.size() < queue_capacity_ || !running_; });			
			queue_.push(func);
		}
		queue_cond_.notify_one();

		return std::async(std::launch::deferred, [=, func = std::move(func), future = std::move(future)]
		{
			if (is_current() && !is_ready(future)) {
				{
					std::lock_guard<std::mutex> lock(queue_mutex_);
					(*func)();
					queue_.erase(std::find(queue_.begin(), queue_.end(), func));
				}
				queue_cond_.notify_one();
			}

			return future.get();
		});
	}

	void run() noexcept
	{
		try {
			ensure_gpf_handler_installed_for_thread(u8(name_).c_str());

			while (true) {
				func_t func;
				{
					std::unique_lock<std::mutex> lock(queue_mutex_);
					queue_cond_.wait([&] { return !queue_.empty() || !running_; });
					if (!queue_.empty()) {
						func = std::move(queue_.front());
						queue_.pop();	
					}				
				}

				if (!func) {
					break;
				}

				queue_cond_.notify_one();

				(*func)();
			}
		} catch (...) {
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
	}
};
}
