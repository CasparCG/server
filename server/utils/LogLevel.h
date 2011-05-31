#ifndef LOGLEVEL_H
#define LOGLEVEL_H

namespace caspar {
namespace utils {

struct LogLevel
{
	enum LogLevelEnum
	{
		Debug = 1,
		Verbose,
		Release,
		Critical
	};
};

}
}

#endif
