#include "..\StdAfx.h"
#include "Process.h"

namespace caspar {
namespace utils {

Process::Process()
{
	hProcess_ = ::GetCurrentProcess();
}

bool Process::SetWorkingSetSize(SIZE_T minSize, SIZE_T maxSize) 
{
	return (::SetProcessWorkingSetSize(hProcess_, minSize, maxSize) != 0);
}

bool Process::GetWorkingSetSize(SIZE_T& minSize, SIZE_T& maxSize) 
{
	return (::GetProcessWorkingSetSize(hProcess_, &minSize, &maxSize) != 0);
}

}
}