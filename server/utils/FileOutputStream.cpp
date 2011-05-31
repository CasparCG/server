#include "..\StdAfx.h"

#include "FileOutputStream.h"
#include "LogException.h"

#include <time.h>

namespace caspar {
namespace utils {

FileOutputStream::FileOutputStream(const TCHAR* filename, bool append) : filename(filename), append(append), timestamp(true)
{
	Open();
}

FileOutputStream::~FileOutputStream()
{
	Close();
}

bool FileOutputStream::Open()
{
	this->outStream.open(this->filename.c_str(), this->append ? std::ios_base::out | std::ios_base::app : std::ios_base::out | std::ios_base::trunc);
	if(this->outStream.fail())
		return false;

	return true;
}

void FileOutputStream::Close()
{
	this->outStream.close();
}

void FileOutputStream::SetTimestamp(bool timestamp)
{
	this->timestamp = timestamp;
}

void FileOutputStream::WriteTimestamp()
{
	if (this->timestamp)
	{
		
		TCHAR timeBuffer[30];
		__time64_t ltime;
		_time64(&ltime);
		_tctime64_s<30>(timeBuffer, &ltime);

		tstring logString = TEXT("");
		logString = TEXT("[");
		logString += timeBuffer;
		logString.resize(logString.size()-1);
		logString += TEXT("] ");

		this->outStream << logString;
	}
}

void FileOutputStream::Println(const tstring& message)
{
	try
	{
		Lock lock(*this);
		if (this->timestamp)
			WriteTimestamp();

		this->outStream << message <<  TEXT(" (Thread: ") << GetCurrentThreadId() << TEXT(")") << std::endl;

		// HACK: Because not all unicode chars can be converted to the current locale, the above line could cause the stream to be flagged
		// as beeing in a "bad"-state. just clear the state and ignore. That particular line won't be logged, but subsequent logging will work.
		this->outStream.clear();
		this->outStream.flush();
	}
	catch (LogException& ex)
	{
		new LogException(ex.what());
	}
}

void FileOutputStream::Print(const tstring& message)
{
	try
	{
		Lock lock(*this);
		if (this->timestamp)
			WriteTimestamp();

		this->outStream << message;

		// HACK: Because not all unicode chars can be converted to the current locale, the above line could cause the stream to be flagged
		// as beeing in a "bad"-state. just clear the state and ignore. That particular line won't be logged, but subsequent logging will work.
		this->outStream.clear();
		this->outStream.flush();
	}
	catch (LogException& ex)
	{
		new LogException(ex.what());
	}
}

void FileOutputStream::Write(const void* buffer, size_t size)
{
	try
	{
		Lock lock(*this);
		if (this->timestamp)
			WriteTimestamp();

		this->outStream.write((const TCHAR*)buffer, static_cast<std::streamsize>(size));
		this->outStream.flush();
	}
	catch (LogException& ex)
	{
		new LogException(ex.what());
	}
}

}
}