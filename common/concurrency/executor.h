#pragma once

#include "../exception/exceptions.h"
#include "../exception/win32_exception.h"

#include <boost/thread.hpp>

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>

#include <functional>

namespace caspar {

class executor
{
public:

	enum priority
	{
		low_priority = 0,
		normal_priority,
		high_priority
	};

	explicit executor(const std::function<void()>& f = nullptr)
	{
		size_ = 0;
		is_running_ = false;
		f_ = f != nullptr ? f : [this]{run();};
	}

	virtual ~executor()
	{
		stop();
	}

	void start() // noexcept
	{
		if(is_running_.fetch_and_store(true))
			return;
		thread_ = boost::thread(f_);
	}

	bool is_running() const // noexcept
	{
		return is_running_;
	}
	
	void stop() // noexcept
	{
		is_running_ = false;		
		thread_.join();
	}

	void execute() // noexcept
	{
		boost::unique_lock<boost::mutex> lock(mut_);
		while(size_ < 1)		
			cond_.wait(lock);
		
		try_execute();
	}

	bool try_execute() // noexcept
	{
		std::function<void()> func;
		if(execution_queue_[high_priority].try_pop(func) || execution_queue_[normal_priority].try_pop(func) || execution_queue_[low_priority].try_pop(func))
		{
			func();
			--size_;
		}

		return func != nullptr;
	}
			
	template<typename Func>
	auto begin_invoke(Func&& func, priority p = normal_priority) -> boost::unique_future<decltype(func())> // noexcept
	{	
		typedef decltype(func()) result_type; 
				
		auto task = std::make_shared<boost::packaged_task<result_type>>(std::forward<Func>(func)); // boost::packaged_task cannot be moved into lambda, need to used shared_ptr.
		auto future = task->get_future();
		
		task->set_wait_callback(std::function<void(decltype(*task)& task)>([=](decltype(*task)& task) // The std::function wrapper is required in order to add ::result_type to functor class.
		{
			try
			{
				if(boost::this_thread::get_id() == thread_.get_id())  // Avoids potential deadlock.
					task();
			}
			catch(boost::task_already_started&){}
		}));
		execution_queue_[p].push([=]
		{
			try
			{
				(*task)();    
			}
			catch(boost::task_already_started&){}
			catch(...){CASPAR_LOG_CURRENT_EXCEPTION();}
		});
		++size_;
		cond_.notify_one();

		return std::move(future);		
	}
	
	template<typename Func>
	auto invoke(Func&& func, priority p = normal_priority) -> decltype(func())
	{
		if(boost::this_thread::get_id() == thread_.get_id())  // Avoids potential deadlock.
			return func();
		
		return begin_invoke(std::forward<Func>(func), p).get();
	}
		
private:

	virtual void run() // noexcept
	{
		win32_exception::install_handler();
		while(is_running_)
			execute();
	}

	tbb::atomic<size_t> size_;
	boost::condition_variable cond_;
	boost::mutex mut_;

	std::function<void()> f_;
	boost::thread thread_;
	tbb::atomic<bool> is_running_;
	std::array<tbb::concurrent_bounded_queue<std::function<void()>>, 3> execution_queue_;
};

}