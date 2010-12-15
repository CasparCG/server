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
	explicit executor(const std::function<void()>& f = nullptr)
	{
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
		if(is_running_.fetch_and_store(false))
		{
			execution_queue_.clear();
			execution_queue_.push([](){});			
		}
		thread_.join();
	}

	void execute() // noexcept
	{
		std::function<void()> func;
		execution_queue_.pop(func);	
		func();
	}

	bool try_execute() // noexcept
	{
		std::function<void()> func;
		if(execution_queue_.try_pop(func))
			func();

		return func != nullptr;
	}

	void clear() // noexcept
	{
		execution_queue_.clear();
	}

	template<typename Func>
	void enqueue(Func&& func) // noexcept
	{
		execution_queue_.push([=]{try{func();}catch(...){CASPAR_LOG_CURRENT_EXCEPTION();}});
	}
	
	template<typename Func>
	auto begin_invoke(Func&& func) -> boost::unique_future<decltype(func())> // noexcept
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
		execution_queue_.push([=]
		{
			try
			{
				(*task)();    
			}
			catch(boost::task_already_started&){}
			catch(...){CASPAR_LOG_CURRENT_EXCEPTION();}
		});

		return std::move(future);		
	}
	
	template<typename Func>
	auto invoke(Func&& func) -> decltype(func())
	{
		if(boost::this_thread::get_id() == thread_.get_id())  // Avoids potential deadlock.
			return func();
		
		return begin_invoke(std::forward<Func>(func)).get();
	}
		
private:

	virtual void run() // noexcept
	{
		win32_exception::install_handler();
		while(is_running_)
			execute();
	}

	std::function<void()> f_;
	boost::thread thread_;
	tbb::atomic<bool> is_running_;
	tbb::concurrent_bounded_queue<std::function<void()>> execution_queue_;
};

}