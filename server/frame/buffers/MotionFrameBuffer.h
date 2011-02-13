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
 
#ifndef _MOTION_FRAME_BUFFER_H_
#define _MOTION_FRAME_BUFFER_H_

#include "FrameBuffer.h"
#include "../Frame.h"

#include "../../utils/event.h"

#include <memory>

namespace caspar
{

class MotionFrameBuffer : public FrameBuffer, private utils::Noncopyable
{
public:
	MotionFrameBuffer();
	explicit MotionFrameBuffer(unsigned int);
	virtual ~MotionFrameBuffer(){}

	int size() const;

	virtual FramePtr front() const;
	virtual FrameBufferFetchResult pop_front();
	virtual void push_back(FramePtr);
	virtual void clear();
	void SetCapacity(size_t capacity);

	HANDLE GetWriteWaitHandle();
	virtual HANDLE GetWaitHandle() const;

private:
	struct Implementation;
	std::tr1::shared_ptr<Implementation> pImpl_;
};

}

#endif