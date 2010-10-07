#ifndef CASPAR_FUNCTION_TASK_H_

#define CASPAR_FUNCTION_TASK_H_

#include <tbb/task.h>
#include <cassert>
#include <tbb/concurrent_queue.h>
#include <tbb/atomic.h>
#include <functional>

namespace caspar
{
	class function_task : tbb::task
	{
	public:
		function_task(std::function<void()>&& func) : func_(std::move(func))
		{
			assert(func_);
		}

		tbb::task* execute()
		{
			func_();
			return nullptr;
		}

		inline static void enqueue(std::function<void()>&& func)
		{
			tbb::task::enqueue(*new(tbb::task::allocate_root()) function_task(std::forward<std::function<void()>>(func)));
		}

	private:
		std::function<void()> func_;
	};

	class function_task_serializer
	{
	public:
		function_task_serializer()
		{
			count_ = 0;
		}

		void enqueue(std::function<void()>&& func)
		{
			assert(func);
			if(!func)
				return;

			queue_.push(std::forward<std::function<void()>>(func));
			if(++count_ == 1)
				function_task::enqueue([=]{execute();});
		}

	private:
		void execute()
		{
			std::function<void()> func;
			if(queue_.try_pop(func))
			{
				func();
				note_completion();
			}
		}

		void note_completion()
		{
			if(--count_ != 0)
				function_task::enqueue([=]{execute();});
		}

		tbb::concurrent_bounded_queue<std::function<void()>> queue_;
		tbb::atomic<int> count_;
	};
}

#endif
