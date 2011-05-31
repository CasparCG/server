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
