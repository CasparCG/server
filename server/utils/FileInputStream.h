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
 
#ifndef CASPAR_FILEINPUTSTREAM_H__
#define CASPAR_FILEINPUTSTREAM_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "inputstream.h"
#include <fstream>

namespace caspar {
namespace utils	{

class FileInputStream : public InputStream
{
public:
	FileInputStream(const tstring& filename);
	virtual ~FileInputStream();

	virtual bool Open();

	virtual unsigned int Read(unsigned char*, size_t);
	virtual void Close();

private:
	tstring	filename_;
	std::ifstream	ifs_;
};

}	//namespace utils
}	//namespace caspar

#endif	//CASPAR_FILEINPUTSTREAM_H__