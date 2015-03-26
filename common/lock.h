#pragma once

namespace caspar {

template<typename T, typename F>
auto lock(T& mutex, F&& func) -> decltype(func())
{
	typename T::scoped_lock lock(mutex);
	return func();
}

}
