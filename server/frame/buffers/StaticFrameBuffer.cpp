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
 
#include "../../stdafx.h"

#include "StaticFrameBuffer.h"

namespace caspar
{

StaticFrameBuffer::StaticFrameBuffer() : event_(TRUE, FALSE) 
{
}
StaticFrameBuffer::~StaticFrameBuffer() 
{
}

FramePtr StaticFrameBuffer::front() const 
{
	tbb::spin_mutex::scoped_lock lock(mutex_);
	return pFrame_;
}

FrameBufferFetchResult StaticFrameBuffer::pop_front() 
{
	tbb::spin_mutex::scoped_lock lock(mutex_);
	if(pFrame_ == 0)
		return FetchWait;
	else
		return FetchEOF;
}

void StaticFrameBuffer::push_back(FramePtr pFrame) 
{
	tbb::spin_mutex::scoped_lock lock(mutex_);
	pFrame_ = pFrame;
	event_.Set();
}

void StaticFrameBuffer::clear() 
{
	tbb::spin_mutex::scoped_lock lock(mutex_);
	event_.Reset();
	pFrame_.reset();
}

HANDLE StaticFrameBuffer::GetWaitHandle() const 
{
	return event_;
}

}