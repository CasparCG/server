#include "..\StdAfx.h"

#include "Logger.h"
#include "LogException.h"
#include "LogStream.h"
#include "OutputStream.h"

namespace caspar {
namespace utils {

Logger::Logger() : level(LogLevel::Release)
{
}

Logger::~Logger()
{
}

Logger& Logger::GetInstance()
{
	static Logger logger;
	return logger;
}

LogStream Logger::GetStream(LogLevel::LogLevelEnum level)
{
	LogStream logStream(this);
	logStream << level;
	return logStream;
}

void Logger::WriteLog(const tstring& message)
{
	try
	{
		this->pOutputStream->Println(message);
	}
	catch(const std::exception& ex)
	{
		throw LogException(ex.what());
	}
}

void Logger::SetTimestamp(bool timestamp)
{
	if (pOutputStream != NULL)
		this->pOutputStream->SetTimestamp(timestamp);
}

bool Logger::SetOutputStream(OutputStreamPtr pOutputStream)
{
	try
	{
		if (pOutputStream != NULL)
		{
			Lock lock(*this);
			this->pOutputStream = pOutputStream;

			return true;
		}

		return false;
	}
	catch(const std::exception& ex)
	{
		throw LogException(ex.what());
	}
}

LogLevel::LogLevelEnum Logger::GetLevel()
{
	return this->level;
}

void Logger::SetLevel(LogLevel::LogLevelEnum level)
{
	this->level = level;
}

}
}