/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "stdafx.h"

#include "prec_timer.h"

#include "os/windows/windows.h"

#include <Mmsystem.h>

namespace caspar {
	
prec_timer::prec_timer()
	: time_(0)
{
}

// Author: Ryan M. Geiss
// http://www.geisswerks.com/ryan/FAQS/timing.html
void prec_timer::tick(double interval)
{     	
	auto t = ::timeGetTime();

	if (time_ != 0)
	{
		auto ticks_to_wait = static_cast<DWORD>(interval*1000.0);
		bool done = 0;
		do
		{				
			auto ticks_passed = t - time_;
			auto ticks_left	  = ticks_to_wait - ticks_passed;

			if (t < time_)    // time wrap
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
				if (ticks_left > 2)
					Sleep(1);
				else                        
					for (int i = 0; i < 10; ++i) 
						Sleep(0);  // causes thread to give up its timeslice
			}

			t = ::timeGetTime();
		}
		while (!done);            
	}

	time_ = t;
}	

}