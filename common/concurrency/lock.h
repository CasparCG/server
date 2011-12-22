#pragma once

namespace caspar {

template<typename T, typename F>
void lock(T& mutex, F&& func)
{
	T::scoped_lock lock(mutex);
	func();
}

}