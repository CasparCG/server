#pragma once

#include "scope_exit.h"

namespace caspar {

template<typename T, typename F>
auto lock(T& mutex, F&& func) -> decltype(func())
{
	mutex.lock();
	CASPAR_SCOPE_EXIT
	{
		mutex.unlock();
	};

	return func();
}

template<typename T, typename F>
auto unlock(T& mutex, F&& func) -> decltype(func())
{
	mutex.unlock();
	CASPAR_SCOPE_EXIT
	{
		mutex.lock();
	};
	return func();
}

}