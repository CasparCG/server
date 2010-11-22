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