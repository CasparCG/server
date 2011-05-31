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
