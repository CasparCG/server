#pragma once

#include "../exception/exceptions.h"
#include "../exception/win32_exception.h"

#include <boost/thread.hpp>

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>

#include <functional>

namespace caspar { namespace common {

class executor
{
public:
	explicit executor(const std::function<void()>& run_func = nullptr)
	{
		is_running_ = false;
		run_func_ = run_func != nullptr ? run_func : [=]{run();};
	}

	virtual ~executor()
	{
		stop();
	}

	void start()
	{
		if(is_running_.fetch_and_store(true))
			return;
		thread_ = boost::thread(run_func_);
	}

	bool is_running() const
	{
		return is_running_;
	}

	void stop()
	{
		if(is_running_.fetch_and_store(false))
		{
			execution_queue_.clear();
			execution_queue_.push([](){});			
		}
		thread_.join();
	}

	void execute()
	{
		std::function<void()> func;
		execution_queue_.pop(func);	
		func();
	}

	bool try_execute()
	{
		std::function<void()> func;
		if(execution_queue_.try_pop(func))
			func();

		return func != nullptr;
	}

	void clear()
	{
		execution_queue_.clear();
	}

	template<typename Func>
	void enqueue(Func&& func)
	{
		execution_queue_.push([=]{try{func();}catch(...){CASPAR_LOG_CURRENT_EXCEPTION();}});
	}

	template<typename Func>
	auto begin_invoke(Func&& func) -> boost::unique_future<decltype(func())>
	{	
		typedef decltype(func()) result_type; 
				
		auto task = std::make_shared<boost::packaged_task<result_type>>(std::forward<Func>(func));	
		auto future = task->get_future();
		
		if(boost::this_thread::get_id() != thread_.get_id())
			execution_queue_.push([=]{(*task)();});
		else
			(*task)();

		return std::move(future);		
	}
	
	template<typename Func>
	auto invoke(Func&& func) -> decltype(func())
	{
		return begin_invoke(std::forward<Func>(func)).get();
	}
	
private:

	virtual void run()
	{
		win32_exception::install_handler();
		while(is_running_)
			execute();
	}

	std::function<void()> run_func_;
	boost::thread thread_;
	tbb::atomic<bool> is_running_;
	tbb::concurrent_bounded_queue<std::function<void()>> execution_queue_;
};

}}