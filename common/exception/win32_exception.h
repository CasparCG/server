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

#include "exceptions.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

namespace caspar {

class win32_exception : public std::exception
{
public:
	typedef const void* address;
	static void install_handler();

	address location() const { return location_; }
	unsigned int error_code() const { return errorCode_; }
	virtual const char* what() const { return message_;	}

protected:
	win32_exception(const EXCEPTION_RECORD& info);
	static void Handler(unsigned int errorCode, EXCEPTION_POINTERS* pInfo);

private:
	const char* message_;

	address location_;
	unsigned int errorCode_;
};

class win32_access_violation : public win32_exception
{
	mutable char messageBuffer_[256];

public:
	bool is_write() const { return isWrite_; }
	address bad_address() const { return badAddress_;}
	virtual const char* what() const;

protected:
	win32_access_violation(const EXCEPTION_RECORD& info);
	friend void win32_exception::Handler(unsigned int errorCode, EXCEPTION_POINTERS* pInfo);

private:
	bool isWrite_;
	address badAddress_;
};

}