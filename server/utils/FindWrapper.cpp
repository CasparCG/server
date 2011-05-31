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