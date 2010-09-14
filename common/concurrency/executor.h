#pragma once

#include <boost/thread.hpp>

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>

#include <functional>

namespace caspar { namespace common {

class executor
{
public:
	executor(const std::function<void()>& run_func = nullptr) : run_func_(run_func)
	{
		is_running_ = false;
		if(run_func_ == nullptr) 
			run_func_ = [=]{default_run();};
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

	bool stop(unsigned int timeout = 5000)
	{
		if(is_running_.fetch_and_store(false))
		{
			execution_queue_.push([](){});
			execute(false);
		}
		return thread_.timed_join(boost::posix_time::milliseconds(timeout));
	}

	void execute(bool block = false)
	{
		std::function<void()> func;
		if(block)		
		{
			execution_queue_.pop(func);	
			func();
		}

		while(execution_queue_.try_pop(func))
			func();		
	}

	template<typename Func>
	auto begin_invoke(Func&& func) -> boost::unique_future<decltype(func())>
	{	
		typedef decltype(func()) result_type; 

		if(!is_running_)
			return boost::packaged_task<result_type>([]{ return result_type(); }).get_future();

		if(boost::this_thread::get_id() == thread_.get_id())
			return boost::packaged_task<result_type>([=]{ return func(); }).get_future();

		auto task = std::make_shared<boost::packaged_task<result_type>>([=]{ return is_running_ ? func() : result_type(); });	
		auto future = task->get_future();

		execution_queue_.push([=]{(*task)();});

		return std::move(future);		
	}
	
	template<typename Func>
	auto invoke(Func&& func) -> decltype(func())
	{
		return begin_invoke(std::forward<Func>(func)).get();
	}
	
private:

	void default_run() throw()
	{
		while(is_running_)
			execute(true);
	}

	std::function<void()> run_func_;
	boost::thread thread_;
	tbb::atomic<bool> is_running_;
	tbb::concurrent_bounded_queue<std::function<void()>> execution_queue_;
};

}}