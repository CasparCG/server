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