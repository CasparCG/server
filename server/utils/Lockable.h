#ifndef _CASPAR_LOCKABLE_H__
#define _CASPAR_LOCKABLE_H__

#pragma once

namespace caspar {
namespace utils {

class LockableObject
{
	mutable CRITICAL_SECTION mtx_;

	LockableObject(const LockableObject&);
	LockableObject& operator=(const LockableObject&);

protected:
	LockableObject(unsigned int spincount = 4000)
	{
		::InitializeCriticalSectionAndSpinCount(&mtx_, spincount);
	}

public:
	~LockableObject()
	{
		::DeleteCriticalSection(&mtx_);
	}

	class Lock;
	friend class Lock;

	class Lock
	{
		LockableObject const& host_;

		Lock(const Lock&);
		Lock& operator=(const Lock&);

	public:
		explicit Lock(const LockableObject& host) : host_(host)
		{
			::EnterCriticalSection(&host_.mtx_);
		}

		~Lock()
		{
			::LeaveCriticalSection(&host_.mtx_);
		}
	};
};

}	//namespace utils
}	//namespace caspar

#endif	//_CASPAR_LOCKABLE_H__