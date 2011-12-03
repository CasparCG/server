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

#include "../stdafx.h"

#include "win32_exception.h"

#include "../log/log.h"

namespace caspar {

void win32_exception::install_handler() 
{
//#ifndef _DEBUG
	_set_se_translator(win32_exception::Handler);
//#endif
}

void win32_exception::Handler(unsigned int errorCode, EXCEPTION_POINTERS* pInfo) {
	switch(errorCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		throw win32_access_violation(*(pInfo->ExceptionRecord));
		break;

	default:
		throw win32_exception(*(pInfo->ExceptionRecord));
	}
}

win32_exception::win32_exception(const EXCEPTION_RECORD& info) : message_("Win32 exception"), location_(info.ExceptionAddress), errorCode_(info.ExceptionCode)
{
	switch(info.ExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
		message_ = "Access violation";
		break;
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
		message_ = "Divide by zero";
		break;
	}
}

win32_access_violation::win32_access_violation(const EXCEPTION_RECORD& info) : win32_exception(info), isWrite_(false), badAddress_(0) 
{
	isWrite_ = info.ExceptionInformation[0] == 1;
	badAddress_ = reinterpret_cast<win32_exception::address>(info.ExceptionInformation[1]);
}

const char* win32_access_violation::what() const
{
	sprintf_s<>(messageBuffer_, "Access violation at %p, trying to %s %p", location(), isWrite_?"write":"read", badAddress_);

	return messageBuffer_;
}

}