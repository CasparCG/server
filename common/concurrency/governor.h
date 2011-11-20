#pragma once

#include <memory>

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>

#include <tbb/atomic.h>

namespace caspar {

typedef std::shared_ptr<void> ticket;

namespace detail
{	
	class governor_impl : public std::enable_shared_from_this<governor_impl>
	{
		boost::mutex				mutex_;
		boost::condition_variable	cond_;
		tbb::atomic<int>			count_;
	public:
		governor_impl(size_t count) 
		{
			count_ = count;
		}

		ticket acquire()
		{
			{
				boost::unique_lock<boost::mutex> lock(mutex_);
				while(count_ < 0)
					cond_.wait(lock);
				--count_;
			}

			auto self = shared_from_this();
			return ticket(nullptr, [self](void*)
			{
				++self->count_;
				self->cond_.notify_one();
			});
		}

		void cancel()
		{
			count_ = std::numeric_limits<int>::max();			
			cond_.notify_all();
		}
	};
}

class governor
{
	std::shared_ptr<detail::governor_impl> impl_;
public:

	governor(size_t count) 
		: impl_(new detail::governor_impl(count))
	{
	}

	ticket acquire()
	{
		return impl_->acquire();
	}

	void cancel()
	{
		impl_->cancel();
	}

};

}