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
 
#ifndef _FRAME_BUFFER_H_
#define _FRAME_BUFFER_H_

#include "../Frame.h"

namespace caspar
{

enum FrameBufferFetchResult 
{
	FetchDataAvailible,
	FetchWait,
	FetchEOF
};

class FrameBuffer
{
public:
	virtual ~FrameBuffer() 
	{}

	virtual FramePtr front() const = 0;
	virtual FrameBufferFetchResult pop_front() = 0;

	virtual void push_back(FramePtr) = 0;

	virtual void clear() = 0;

	virtual HANDLE GetWaitHandle() const = 0;
};

}

#endif