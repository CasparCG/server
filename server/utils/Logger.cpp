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