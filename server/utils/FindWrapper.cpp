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
 
#include "..\stdafx.h"

#include "FindWrapper.h"

namespace caspar {
namespace utils {

	FindWrapper::FindWrapper(const tstring& filename, WIN32_FIND_DATA* pFindData) : hFile_(INVALID_HANDLE_VALUE) {
		hFile_ = FindFirstFile(filename.c_str(), pFindData);
	}

	FindWrapper::~FindWrapper() {
		if(hFile_ != INVALID_HANDLE_VALUE) {
			FindClose(hFile_);
			hFile_ = INVALID_HANDLE_VALUE;
		}
	}

	bool FindWrapper::FindNext(WIN32_FIND_DATA* pFindData)
	{
		return FindNextFile(hFile_, pFindData) != 0;
	}

}	//namespace utils
}	//namespace caspar