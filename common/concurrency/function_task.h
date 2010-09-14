#pragma once

#include <tbb/task.h>
#include <tbb/concurrent_queue.h>
#include <boost/thread/future.hpp>

namespace caspar { namespace common { namespace function_task {
	
namespace detail {
	
	template<typename Func>
	class packaged_task : public tbb::task
	{
	public:
		packaged_task(boost::packaged_task<Func>&& task) : task_(std::forward<boost::packaged_task<Func>>(task)) {}
	
	private:
		tbb::task* execute() 
		{
			task_();
			return nullptr;
		}

		boost::packaged_task<Func> task_;
	};

	template<typename Func>
	class internal_function_task : public tbb::task
	{
	public:
		internal_function_task(Func&& func) : func_(std::forward<Func>(func)) {}
	private:
		tbb::task* execute() 
		{
			func_();
			return nullptr;
		}

		Func func_;
	};
}
		
template <typename Func>
void enqueue(Func&& func)
{
	tbb::task::enqueue(*new(tbb::task::allocate_root()) detail::internal_function_task<Func>(std::forward<Func>(func)));
}

template<typename Func>
auto begin_invoke(Func&& func) -> boost::unique_future<decltype(func())>
{			
	auto task = boost::packaged_task<decltype(func())>(std::forward<Func>(func));
	auto future = task.get_future();
	tbb::task::enqueue(*new(tbb::task::allocate_root()) detail::packaged_task<decltype(func())>(boost::move(task)));		
	return std::move(future);		
}

template<typename Func>
auto invoke(Func&& func) -> decltype(func())
{	
	return function_task::begin_invoke(std::forward<Func>(func)).get();		
}

};

}}