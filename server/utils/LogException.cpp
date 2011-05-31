#include "..\StdAfx.h"

#include "LogException.h"

namespace caspar {
namespace utils {

LogException::LogException(const char* message) : std::runtime_error(message)
{
}

LogException::~LogException()
{
}

const char* LogException::GetMassage() const
{
	return std::runtime_error::what();
}

}
}