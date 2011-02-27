#pragma once

#include "../exception/win32_exception.h"
#include "../log/log.h"

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>

#include <boost/thread.hpp>
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
	__except (EXCEPTION_CONTINUE_EXECUTION)
	{
	}	
}

}

class executor : boost::noncopyable
{
public:
		
	explicit executor(const std::wstring& name = L"executor") : name_(narrow(name))
	{
		is_running_ = false;
	}
	
	virtual ~executor()
	{
		stop();
		clear();
		if(boost::this_thread::get_id() != thread_.get_id())
			thread_.join();
	}

	void set_capacity(size_t capacity)
	{
		execution_queue_.set_capacity(capacity);
	}

	void start() // noexcept
	{
		if(is_running_.fetch_and_store(true))
			return;
		clear();
		thread_ = boost::thread([this]{run();});
	}
			
	void stop() // noexcept
	{
		is_running_ = false;	
		execution_queue_.try_push([]{});
	}
	
	void clear()
	{
		std::function<void()> func;
		auto size = execution_queue_.size();
		for(int n = 0; n < size; ++n)
		{
			try
			{
				if(!execution_queue_.try_pop(func))
					return;
			}
			catch(boost::broken_promise&){}
		}
	}
			
	template<typename Func>
	auto begin_invoke(Func&& func) -> boost::unique_future<decltype(func())> // noexcept
	{	
		typedef boost::packaged_task<decltype(func())> task_type;
				
		auto task = task_type(std::forward<Func>(func));
		auto future = task.get_future();
		
		task.set_wait_callback(std::function<void(task_type&)>([=](task_type& my_task) // The std::function wrapper is required in order to add ::result_type to functor class.
		{
			try
			{
				if(boost::this_thread::get_id() == thread_.get_id())  // Avoids potential deadlock.
					my_task();
			}
			catch(boost::task_already_started&){}
		}));
				
		struct task_adaptor_t
		{
			task_adaptor_t(const task_adaptor_t& other) : task(std::move(other.task)){}
			task_adaptor_t(task_type&& task) : task(std::move(task)){}
			void operator()() const { task(); }
			mutable task_type task;
		} task_adaptor(std::move(task));

		execution_queue_.push([=]
		{
			try{task_adaptor();}
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
	
	tbb::concurrent_bounded_queue<std::function<void()>>::size_type capacity() const { return execution_queue_.capacity();	}
	tbb::concurrent_bounded_queue<std::function<void()>>::size_type size() const { return execution_queue_.size();	}
	bool empty() const		{ return execution_queue_.empty();	}
	bool is_running() const { return is_running_;				}	
		
private:
	
	void execute() // noexcept
	{
		std::function<void()> func;
		execution_queue_.pop(func);	
		func();
	}

	void run() // noexcept
	{
		win32_exception::install_handler();		
		detail::SetThreadName(GetCurrentThreadId(), name_.c_str());
		while(is_running_)
			execute();
	}
	
	std::string name_;
	boost::thread thread_;
	tbb::atomic<bool> is_running_;
	tbb::concurrent_bounded_queue<std::function<void()>> execution_queue_;
};

}