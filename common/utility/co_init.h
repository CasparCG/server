#pragma once

#include "assert.h"

#include <Objbase.h>

#include <boost/noncopyable.hpp>

namespace caspar {

class co_init : boost::noncopyable
{
	DWORD threadId_;
public: 
	co_init(DWORD dwCoInit = COINIT_MULTITHREADED)
		: threadId_(GetCurrentThreadId())
	{
		::CoInitializeEx(nullptr, dwCoInit);
	}

	~co_init()
	{
		CASPAR_ASSERT(GetCurrentThreadId() == threadId_);
		::CoUninitialize();
	}
};

}