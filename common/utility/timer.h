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
#pragma once

#include <windows.h>

namespace caspar {
	
class high_prec_timer
{
public:
	high_prec_timer()
	{
		QueryPerformanceFrequency(&freq_);
		QueryPerformanceCounter(&time_);
	}

	// Author: Ryan M. Geiss
	// http://www.geisswerks.com/ryan/FAQS/timing.html
	void tick(double interval)
	{     	
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);

		if (time_.QuadPart != 0)
		{
			__int64 ticks_to_wait = static_cast<int>(static_cast<double>(freq_.QuadPart) * interval);
			__int64 done = 0;
			do
			{
				QueryPerformanceCounter(&t);
				
				__int64 ticks_passed = static_cast<__int64>(t.QuadPart) - static_cast<__int64>(time_.QuadPart);
				__int64 ticks_left = ticks_to_wait - ticks_passed;

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
					if (ticks_left > static_cast<__int64>((freq_.QuadPart*2)/1000))
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