#ifndef __FILEINFO_H__
#define __FILEINFO_H__

namespace caspar {

class FileInfo
{
public:
	FileInfo() : length(0), resolution(0), size(0), filetype(TEXT("")), filename(TEXT("")), encoding(TEXT("")), type(TEXT(""))
	{}
	tstring filename;
	tstring filetype;
	tstring encoding;
	tstring type;
	int length;
	int resolution;
	unsigned long long size;
};

}	//namespace caspar

#endif __FILEINFO_H__