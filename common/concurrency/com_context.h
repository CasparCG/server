/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#pragma once

#include "executor.h"

#include "../log/log.h"
#include "../exception/exceptions.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include <boost/noncopyable.hpp>
#include <boost/thread/future.hpp>

#include <functional>

namespace caspar {

template<typename T>
class com_context : public executor
{
	std::unique_ptr<T> instance_;
public:
	com_context(const std::wstring& name) : executor(name)
	{
		executor::begin_invoke([]
		{
			::CoInitialize(nullptr);
		});
	}

	~com_context()
	{
		if(!executor::begin_invoke([&]
		{
			instance_.reset(nullptr);
			::CoUninitialize();
		}).timed_wait(boost::posix_time::milliseconds(500)))
		{
			CASPAR_LOG(error) << L"[com_contex] Timer expired, deadlock detected and released, leaking resources.";
		}
	}
	
	void reset(const std::function<T*()>& factory = nullptr)
	{
		executor::invoke([&]
		{
			instance_.reset();
			if(factory)
				instance_.reset(factory());
		});
	}

	T& operator*() const 
	{
		if(instance_ == nullptr)
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("Tried to access null context."));

		return *instance_.get();
	}  // noexcept

	T* operator->() const 
	{
		if(instance_ == nullptr)
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("Tried to access null context."));
		return instance_.get();
	}  // noexcept

	T* get() const
	{
		return instance_.get();
	}  // noexcept

	operator bool() const {return get() != nullptr;}
};

}