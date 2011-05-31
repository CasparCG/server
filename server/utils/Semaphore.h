#ifndef _CSAPAR_SEMAPHORE_H__
#define _CASPAR_SEMAPHORE_H__

#pragma once

namespace caspar {
namespace utils {

class Semaphore
{
public:
	Semaphore(long initialCount, long maximumCount);
	~Semaphore();

	bool Semaphore::Release(long releaseCount = 1, long* pPrevCount = 0);

	operator const HANDLE() const {
		return handle_;
	}

private:
	HANDLE handle_;
};

typedef std::tr1::shared_ptr<Semaphore> SemaphorePtr;

}	//namespace utils
}	//namespace caspar

#endif	//_CASPAR_SEMAPHORE_H__