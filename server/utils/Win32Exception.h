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