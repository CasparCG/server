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