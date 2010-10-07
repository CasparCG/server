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
 
#ifndef _STATIC_FRAME_BUFFER_
#define _STATIC_FRAME_BUFFER_

#include "FrameBuffer.h"
#include "../Frame.h"

#include <tbb/spin_mutex.h>

namespace caspar
{

class StaticFrameBuffer : public FrameBuffer, private utils::Noncopyable
{	
public:
	StaticFrameBuffer();
	virtual ~StaticFrameBuffer();

	virtual FramePtr front() const;
	virtual FrameBufferFetchResult pop_front();
	virtual void push_back(FramePtr);
	virtual void clear();
	virtual HANDLE GetWaitHandle() const;

private:
	utils::Event event_;
	FramePtr pFrame_;
	mutable tbb::spin_mutex mutex_;
};

}

#endif