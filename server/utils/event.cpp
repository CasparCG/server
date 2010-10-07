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

#include <string>
#include "Event.h"

namespace caspar {
namespace utils {

Event::Event(bool bManualReset, bool bInitialState) : handle_(0)
{
	handle_ = CreateEvent(0, bManualReset, bInitialState, 0);
	if(handle_ == 0) {
		throw std::exception("Failed to create event");
	}
}

void Event::Set() {
	BOOL res = SetEvent(handle_);
	if(res == FALSE) {
		DWORD error = GetLastError();
	}
}
void Event::Reset() {
	ResetEvent(handle_);
}

Event::~Event()
{
	CloseHandle(handle_);
}

}	//namespace utils
}	//namespace caspar