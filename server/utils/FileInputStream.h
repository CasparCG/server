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