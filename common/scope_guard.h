#pragma once

namespace caspar {

template<typename F>
class scope_guard
{
	F func_;
public:

	template<typename U>
	scope_guard(U&& func)
		: func_(std::forward<U>(func))
	{
	}

	~scope_guard()
	{
		func_();
	}
};

template<typename F>
scope_guard<F> make_scope_guard(F&& func)
{
	return scope_guard<F>(std::forward<F>(func));
}

}