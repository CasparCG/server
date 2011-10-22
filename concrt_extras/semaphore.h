//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: concrt_extras.h
//
//  Implementation of ConcRT helpers
//
//--------------------------------------------------------------------------

#pragma once

#include <concrt.h>
#include <concurrent_queue.h>

namespace Concurrency
{	
	class semaphore
	{
	public:
		explicit semaphore(LONG capacity)
			  : _semaphore_count(capacity)
		{
		}

		// Acquires access to the semaphore.
		void acquire()
		{
			// The capacity of the semaphore is exceeded when the semaphore count 
			// falls below zero. When this happens, add the current context to the 
			// back of the wait queue and block the current context.
			if (InterlockedDecrement(&_semaphore_count) < 0)
			{
				_waiting_contexts.push(Concurrency::Context::CurrentContext());
				Concurrency::Context::Block();
			}
		}

		// Releases access to the semaphore.
		void release()
		{
		  // If the semaphore count is negative, unblock the first waiting context.
			if (InterlockedIncrement(&_semaphore_count) <= 0)
			{
			 // A call to acquire might have decremented the counter, but has not
			 // yet finished adding the context to the queue. 
			 // Create a spin loop that waits for the context to become available.
				Concurrency:: Context* waiting = NULL;
				while(!_waiting_contexts.try_pop(waiting))
				{
					Concurrency::wait(0);
				}

				// Unblock the context.
				 waiting->Unblock();
			}
		}

	private:
		// The semaphore count.
		LONG _semaphore_count;

		// A concurrency-safe queue of contexts that must wait to 
		// acquire the semaphore.
		Concurrency::concurrent_queue<Concurrency::Context*> _waiting_contexts;
	   
		semaphore const &operator =(semaphore const&);  // no assignment operator
		semaphore(semaphore const &);                   // no copy constructor
	};
}