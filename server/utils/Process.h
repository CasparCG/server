#ifndef _CASPARUTILS_PROCESS_H__
#define _CASPARUTILS_PROCESS_H__

#pragma once

namespace caspar {
namespace utils {

class Process
{
	Process();
	Process(const Process&);
	Process& operator=(const Process&);

public:
	static Process& GetCurrentProcess() {
		static Process instance;
		return instance;
	}

	bool SetWorkingSetSize(SIZE_T minSize, SIZE_T maxSize);
	bool GetWorkingSetSize(SIZE_T& minSize, SIZE_T& maxSize);

private:
	HANDLE hProcess_;
};

}	//namespace utils
}	//namespace caspar
#endif	//_CASPARUTILS_PROCESS_H__