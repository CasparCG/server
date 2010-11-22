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
#include "LogStream.h"

namespace caspar {
namespace utils {

LogStream::LogStream(Logger* pLogger) : pLogger(pLogger), pSstream(0)
{
}

LogStream::LogStream(const LogStream& logStream) : pSstream(0),
												   pLogger(logStream.pLogger), level(logStream.level)
{
	if(logStream.pSstream != 0)
		this->pSstream = new tstringstream(logStream.pSstream->str());
}

LogStream& LogStream::operator=(const LogStream& rhs)
{
	if(rhs.pSstream != 0)
		this->pSstream = new tstringstream(rhs.pSstream->str());

	this->pLogger = rhs.pLogger;
	this->level = rhs.level;

	return *this;
}

LogStream::~LogStream()
{
	if (this->pSstream != NULL)
	{
		DoFlush();
		delete this->pSstream;
	}
}

void LogStream::DoFlush()
{
	if(this->pSstream != 0 && this->pSstream->str().length() > 0)
	{
		this->pLogger->WriteLog(this->pSstream->str());
		this->pSstream->str(TEXT(""));
	}
}

}
}