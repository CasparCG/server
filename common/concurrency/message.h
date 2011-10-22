#pragma once

#include "../memory/safe_ptr.h"

#include <agents.h>
#include <semaphore.h>

#include <boost/noncopyable.hpp>

namespace caspar {
	
class token : boost::noncopyable
{
	std::shared_ptr<Concurrency::semaphore> semaphore_;
public:

	token(const safe_ptr<Concurrency::semaphore>& semaphore)
		: semaphore_(semaphore)
	{
		semaphore_->acquire();
	}

	token(token&& other)
		: semaphore_(std::move(other.semaphore_))
	{
	}

	~token()
	{
		if(semaphore_)
			semaphore_->release();
	}
};

template<typename T>
class message
{
	T payload_;
	token token_;
public:
	message(const T& payload, token&& token)
		: payload_(payload)
		, token_(std::move(token))
	{
	}
	
	template<typename U>
	safe_ptr<message<U>> transfer(const U& payload)
	{
		return make_safe<message<U>>(payload, std::move(token_));
	}

	T& value() {return payload_;}
	const T& value() const {return payload_;}
};

}