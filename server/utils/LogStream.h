#ifndef LOGSTREAM_H
#define LOGSTREAM_H

#include "LogLevel.h"

namespace caspar {
namespace utils {

class Logger;

class LogStream
{
	public:
		enum LogStreamEnum { Flush = 1 };

		explicit LogStream(Logger* pLogger);
		LogStream(const LogStream& logStream);
		LogStream& operator=(const LogStream& rhs);

		~LogStream();

		template<typename T>
		LogStream& operator<<(const T& rhs) {
			if (this->level >= this->pLogger->GetLevel()) {
				if(this->pSstream == 0)
					this->pSstream = new tstringstream();

				*pSstream << rhs;
			}

			return *this;
		}

		template<> LogStream& operator<<(const LogLevel::LogLevelEnum& rhs) {
			this->level = rhs;
			return *this;
		}

		template<> LogStream& operator<<(const LogStream::LogStreamEnum& rhs) {
			if (rhs == LogStream::Flush)
				DoFlush();
			return *this;
		}

	private:
		void DoFlush();

		Logger* pLogger;
		tstringstream* pSstream;
		LogLevel::LogLevelEnum level;
};

}
}

#endif
