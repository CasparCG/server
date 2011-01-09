#pragma once

#include <windows.h>

namespace caspar {
	
class timer
{
public:
	timer()
	{
		QueryPerformanceFrequency(&freq_);
		QueryPerformanceCounter(&time_);
	}

	// Author: Ryan M. Geiss
	// http://www.geisswerks.com/ryan/FAQS/timing.html
	void wait(double fps)
	{     	
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);

		if (time_.QuadPart != 0)
		{
			int ticks_to_wait = static_cast<int>(static_cast<double>(freq_.QuadPart) / fps);
			int done = 0;
			do
			{
				QueryPerformanceCounter(&t);
				
				int ticks_passed = static_cast<int>(static_cast<__int64>(t.QuadPart) - static_cast<__int64>(time_.QuadPart));
				int ticks_left = ticks_to_wait - ticks_passed;

				if (t.QuadPart < time_.QuadPart)    // time wrap
					done = 1;
				if (ticks_passed >= ticks_to_wait)
					done = 1;
				
				if (!done)
				{
					// if > 0.002s left, do Sleep(1), which will actually sleep some 
					//   steady amount, probably 1-2 ms,
					//   and do so in a nice way (cpu meter drops; laptop battery spared).
					// otherwise, do a few Sleep(0)'s, which just give up the timeslice,
					//   but don't really save cpu or battery, but do pass a tiny
					//   amount of time.
					if (ticks_left > static_cast<int>((freq_.QuadPart*2)/1000))
						Sleep(1);
					else                        
						for (int i = 0; i < 10; ++i) 
							Sleep(0);  // causes thread to give up its timeslice
				}
			}
			while (!done);            
		}

		time_ = t;
	}		
private:	
	LARGE_INTEGER freq_;
	LARGE_INTEGER time_;
};

}