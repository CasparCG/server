/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
#ifndef _CASPAR_WIN32EXCEPTION_H__
#define _CASPAR_WIN32EXCEPTION_H__

#pragma once

#include <exception>

class Win32Exception : public std::exception 
{
public:
	typedef const void* Address;
	static void InstallHandler();

	Address Location() const {
		return location_;
	}
	unsigned int ErrorCode() const {
		return errorCode_;
	}
	virtual const char* what() const {
		return message_;
	}

protected:
	Win32Exception(const EXCEPTION_RECORD& info);
	static void Handler(unsigned int errorCode, EXCEPTION_POINTERS* pInfo);

private:
	const char* message_;

	Address location_;
	unsigned int errorCode_;
};

class Win32AccessViolationException : public Win32Exception
{
	mutable char messageBuffer_[256];

public:
	bool IsWrite() const {
		return isWrite_;
	}
	Address BadAddress() const {
		return badAddress_;
	}
	virtual const char* what() const;

protected:
	Win32AccessViolationException(const EXCEPTION_RECORD& info);
	friend void Win32Exception::Handler(unsigned int errorCode, EXCEPTION_POINTERS* pInfo);

private:
	bool isWrite_;
	Address badAddress_;
};

#endif	//_CASPAR_WIN32EXCEPTION_H__