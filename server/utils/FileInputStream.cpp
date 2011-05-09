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
#include "fileinputstream.h"

namespace caspar {
namespace utils	{

FileInputStream::FileInputStream(const tstring& filename) : filename_(filename)
{
}

FileInputStream::~FileInputStream()
{
	Close();
}

bool FileInputStream::Open()
{
	ifs_.open(filename_.c_str(), std::ios::in | std::ios::binary);
	return !ifs_.fail();
}

void FileInputStream::Close()
{
	ifs_.close();
}


unsigned int FileInputStream::Read(unsigned char* buf, size_t length)
{
	if(!ifs_.eof())
	{
		ifs_.read(reinterpret_cast<char*>(buf), (std::streamsize)length);
		return ifs_.gcount();
	}
	return 0;
}

/* removed 2008-03-06 - InputStream should be binary only. Make a separate TextInputStream for text
bool FileInputStream::Readln(tstring& out)
{
	static TCHAR buf[256];
	if(!ifs_.eof())
	{
		ifs_.getline(buf, 256, TEXT('\n'));
		out = buf;
		size_t findPos = out.find_first_of(TEXT('\r'));
		if(findPos != tstring::npos)
			out = out.substr(0,findPos);

		if(ifs_.peek() == TEXT('\r'))
			ifs_.get();
		if(ifs_.peek() == TEXT('\n'))
			ifs_.get();
		
		return true;
	}
	return false;
}
*/

}	//namespace utils
}	//namespace caspar
