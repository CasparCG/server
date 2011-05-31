#include "..\StdAfx.h"

#include "Semaphore.h"

namespace caspar {
namespace utils {

Semaphore::Semaphore(long initialCount, long maximumCount) : handle_(0)
{
	handle_ = CreateSemaphore(0, initialCount, maximumCount, 0);
	if(handle_ == 0) {
		throw std::exception("Failed to create semaphore");
	}
}
bool Semaphore::Release(long releaseCount, long* pPrevCount) {
	return ReleaseSemaphore(handle_, releaseCount, pPrevCount) != 0;
}

Semaphore::~Semaphore()
{
	CloseHandle(handle_);
}

}	//namespace utils
}	//namespace caspar