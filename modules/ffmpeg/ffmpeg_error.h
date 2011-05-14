#pragma once

#include <string>

#pragma warning(push, 1)

extern "C" 
{
#include <libavutil/error.h>
}

#pragma warning(pop)

namespace caspar {

static std::string av_error_str(int errn)
{
	char buf[256];
	memset(buf, 0, 256);
	if(av_strerror(errn, buf, 256) < 0)
		return "";
	return std::string(buf);
}

}