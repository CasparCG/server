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
 
#ifndef FILEOUTPUTSTREAM_H
#define FILEOUTPUTSTREAM_H

#include "OutputStream.h"

#include <string>
#include <fstream>

namespace caspar {
namespace utils {

class FileOutputStream : public OutputStream, private LockableObject
{
	public:
		FileOutputStream(const TCHAR* filename, bool bAppend = false);
		virtual ~FileOutputStream();

		virtual bool Open();
		virtual void Close();
		virtual void SetTimestamp(bool timestamp);
		virtual void Print(const tstring& message);
		virtual void Println(const tstring& message);
		virtual void Write(const void* buffer, size_t);

	private:
		void WriteTimestamp();

	private:
		bool append;
		bool timestamp;
		tstring filename;
	
		#ifndef _UNICODE
			std::ofstream	outStream;
		#else
			std::wofstream	outStream;
		#endif
};

}
}

#endif