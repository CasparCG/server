#pragma once

#include "../memory/safe_ptr.h"

#include <concrt.h>
#include <concurrent_queue.h>
#include <tbb/atomic.h>

#include <boost/noncopyable.hpp>

#include <vector>

namespace caspar {
	
//namespace Concurrency
//{	
//	class semaphore
//	{
//	public:
//		explicit semaphore(LONG capacity)
//			  : _semaphore_count(capacity)
//		{
//		}
//
//		// Acquires access to the semaphore.
//		void acquire()
//		{
//			// The capacity of the semaphore is exceeded when the semaphore count 
//			// falls below zero. When this happens, add the current context to the 
//			// back of the wait queue and block the current context.
//			if (InterlockedDecrement(&_semaphore_count) < 0)
//			{
//				_waiting_contexts.push(Concurrency::Context::CurrentContext());
//				Concurrency::Context::Block();
//			}
//		}
//
//		// Releases access to the semaphore.
//		void release()
//		{
//		  // If the semaphore count is negative, unblock the first waiting context.
//			if (InterlockedIncrement(&_semaphore_count) <= 0)
//			{
//			 // A call to acquire might have decremented the counter, but has not
//			 // yet finished adding the context to the queue. 
//			 // Create a spin loop that waits for the context to become available.
//				Concurrency:: Context* waiting = NULL;
//				while(!_waiting_contexts.try_pop(waiting))
//				{
//					Concurrency::wait(0);
//				}
//
//				// Unblock the context.
//				 waiting->Unblock();
//			}
//		}
//
//		void release_all()
//		{
//			Concurrency:: Context* waiting = NULL;
//			while(_waiting_contexts.try_pop(waiting))
//			{
//				InterlockedIncrement(&_semaphore_count);
//				waiting->Unblock();
//			}
//		}
//
//	private:
//		// The semaphore count.
//		LONG _semaphore_count;
//
//		// A concurrency-safe queue of contexts that must wait to 
//		// acquire the semaphore.
//		Concurrency::concurrent_queue<Concurrency::Context*> _waiting_contexts;
//	   
//		semaphore const &operator =(semaphore const&);  // no assignment operator
//		semaphore(semaphore const &);                   // no copy constructor
//	};
//}
	
#undef Yield

typedef std::vector<safe_ptr<int>> ticket_t;

class governor : boost::noncopyable
{
	tbb::atomic<int> count_;
	Concurrency::concurrent_queue<Concurrency::Context*> waiting_contexts_;

	void acquire_ticket()
	{
		if(count_ < 1)
			Concurrency::Context::Yield();

		if (--count_ < 0)
		{
			auto context = Concurrency::Context::CurrentContext();
			waiting_contexts_.push(context);
			context->Block();
		}
	}

	void release_ticket()
	{
		if(++count_ <= 0)
		{
			Concurrency:: Context* waiting = NULL;
			while(!waiting_contexts_.try_pop(waiting))
				Concurrency::Context::Yield();
			waiting->Unblock();
		}
	}

public:

	governor(size_t capacity) 
	{
		count_ = capacity;
	}
	
	ticket_t acquire()
	{
		acquire_ticket();
		
		ticket_t ticket;
		ticket.push_back(safe_ptr<int>(new int, [this](int* p)
		{
			delete p;
			release_ticket();
		}));
		return ticket;
	}

	void cancel()
	{
		while(count_ < 0)
			release_ticket();
	}
};

}