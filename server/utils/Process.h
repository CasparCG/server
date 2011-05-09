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
 
#ifndef _CASPARUTILS_PROCESS_H__
#define _CASPARUTILS_PROCESS_H__

#pragma once

namespace caspar {
namespace utils {

class Process
{
	Process();
	Process(const Process&);
	Process& operator=(const Process&);

public:
	static Process& GetCurrentProcess() {
		static Process instance;
		return instance;
	}

	bool SetWorkingSetSize(SIZE_T minSize, SIZE_T maxSize);
	bool GetWorkingSetSize(SIZE_T& minSize, SIZE_T& maxSize);

private:
	HANDLE hProcess_;
};

}	//namespace utils
}	//namespace caspar
#endif	//_CASPARUTILS_PROCESS_H__