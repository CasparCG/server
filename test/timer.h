#pragma once

#include <windows.h>

class timer
{
public:
	timer()
	{
		QueryPerformanceFrequency(&freq_);
	}

	void start()
	{
		QueryPerformanceCounter(&start_);
	}

	double time()
	{
		LARGE_INTEGER time_;
		QueryPerformanceCounter(&time_);
		return static_cast<double>(time_.QuadPart - start_.QuadPart) / static_cast<double>(freq_.QuadPart);
	}

private:
	LARGE_INTEGER freq_;
	LARGE_INTEGER start_;
};