#pragma once

#include <functional>
#include <memory>

#include <boost/thread/future.hpp>

namespace caspar {
		
template<typename F>
auto defer(F&& f) -> boost::unique_future<decltype(f())>
{	
	typedef boost::promise<decltype(f())> promise_t;
	auto p = new promise_t();

	auto func = [=](promise_t&) mutable
	{
		std::unique_ptr<promise_t> guard(p);
		p->set_value(f());
	};

	p->set_wait_callback(std::function<void(promise_t&)>(func));

	return p->get_future();
}


}