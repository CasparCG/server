#include "..\stdafx.h"

#include "FileExists.h"
#include "FindWrapper.h"

namespace caspar {
namespace utils {

bool exists(const tstring& name, bool isDirectory) {
	WIN32_FIND_DATA findInfo;
	FindWrapper findWrapper(name, &findInfo);
	return findWrapper.Success() && (!isDirectory || (findInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
}

}	//namespace utils
}	//namespace caspar