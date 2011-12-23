#pragma once

namespace caspar {

template<typename T, typename F>
auto lock(T& mutex, F&& func) -> decltype(func())
{
	T::scoped_lock lock(mutex);
	return func();
}

}