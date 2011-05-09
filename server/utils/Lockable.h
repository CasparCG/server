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