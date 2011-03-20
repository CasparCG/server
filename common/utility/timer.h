#pragma once

#include <windows.h>

namespace caspar {
	
class timer
{
public:
	timer() : time_(timeGetTime()){}

	double elapsed()
	{
		return static_cast<double>(timeGetTime() - time_)/1000.0;
	}
	
	void reset()
	{
		time_ = timeGetTime();
	}
	
private:	
	DWORD time_;
};

}