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
 
#ifndef LOGGER_H
#define LOGGER_H

#include "Lockable.h"
#include "LogStream.h"
#include "LogLevel.h"
#include "OutputStream.h"

namespace caspar {
namespace utils {

class Logger : private LockableObject
{
	public:
		~Logger();

		static Logger& GetInstance();
		
		LogLevel::LogLevelEnum GetLevel();
		void SetTimestamp(bool timestamp);
		void WriteLog(const tstring& message);
		void SetLevel(LogLevel::LogLevelEnum Level);
		bool SetOutputStream(OutputStreamPtr pOutputStream);
		LogStream GetStream(LogLevel::LogLevelEnum level = LogLevel::Release);

	private:
		Logger();

	private:
		OutputStreamPtr pOutputStream;
		LogLevel::LogLevelEnum level;
};

}
}

#endif
