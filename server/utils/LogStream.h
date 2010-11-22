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
