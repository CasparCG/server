#pragma once

namespace caspar {

template<typename T, typename F>
auto lock(T& mutex, F&& func) -> decltype(func())
{
	boost::lock_guard<T> lock(mutex);
	return func();
}

}
