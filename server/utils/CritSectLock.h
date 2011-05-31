#ifndef	_CASPAR_CRITSECTLOCK_H__
#define _CASPAR_CRITSECTLOCK_H__

#pragma once

namespace caspar {
namespace utils {

class CritSectLock
{
public:
	CritSectLock(CRITICAL_SECTION* pCritSect) : pCriticalSection_(pCritSect)
	{
		EnterCriticalSection(pCriticalSection_);
	}
	~CritSectLock()
	{
		LeaveCriticalSection(pCriticalSection_);
	}

private:
	CRITICAL_SECTION* pCriticalSection_;
};

}	//namespace utils
}	//namespace caspar

#endif	//_CASPAR_CRITSECTLOCK_H__