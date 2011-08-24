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
 
#ifndef _FINDWRAPPER_H__
#define _FINDWRAPPER_H__

#include <io.h>
#include <string>

namespace caspar {
namespace utils {

typedef std::wstring tstring;

	class FindWrapper
	{
		FindWrapper(const FindWrapper&);
		FindWrapper& operator=(const FindWrapper&);
	public:
		FindWrapper(const tstring&, WIN32_FIND_DATA*);
		~FindWrapper();

		bool FindNext(WIN32_FIND_DATA*);

		bool Success() {
			return (hFile_ != INVALID_HANDLE_VALUE);
		}

	private:
		HANDLE	hFile_;
	};

}	//namespace utils
}	//namespace caspar

#endif