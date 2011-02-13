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
 
#include "../../StdAfx.h"

#include "MotionFrameBuffer.h"

namespace caspar
{

struct MotionFrameBuffer::Implementation : private utils::LockableObject
{
	Implementation() : event_(TRUE, FALSE), writeWaitEvent_(TRUE, TRUE), maxLength_(3)
	{}

	Implementation(unsigned int maxQueueLength) : event_(TRUE, FALSE), writeWaitEvent_(TRUE, TRUE), maxLength_(maxQueueLength)
	{}

	~Implementation() 
	{}

	FramePtr front() const 
	{
		Lock lock(*this);

		FramePtr result;
		if(frameQueue_.size() != 0)
			result = frameQueue_.front();
		return result;
	}

	FrameBufferFetchResult pop_front() 
	{
		Lock lock(*this);

		if(frameQueue_.size() == 0)
		{
			event_.Reset();
			return FetchWait;
		}

		if(frameQueue_.front() != 0)
		{
			frameQueue_.pop_front();
			
			if(frameQueue_.size() < maxLength_)
				writeWaitEvent_.Set();

			if(frameQueue_.size() == 0)
			{
				event_.Reset();
				return FetchWait;
			}

			if(frameQueue_.front() == 0)
				return FetchEOF;

			return FetchDataAvailible;
		}
		return FetchEOF;
	}

	void push_back(FramePtr pFrame) 
	{
		Lock lock(*this);

		//assert(frameQueue_.size() <= maxLength_);

		frameQueue_.push_back(pFrame);
		event_.Set();

		if(frameQueue_.size() >= maxLength_)
			writeWaitEvent_.Reset();
	}

	void clear() 
	{
		Lock lock(*this);

		frameQueue_.clear();
		writeWaitEvent_.Set();
		event_.Reset();
	}

	HANDLE GetWaitHandle() const 
	{
		return event_;
	}

	HANDLE GetWriteWaitHandle() 
	{
		return writeWaitEvent_;
	}

	int size() const
	{
		Lock lock(*this);
		return frameQueue_.size();
	}

	void SetCapacity(size_t capacity)
	{
		Lock lock(*this);
		maxLength_ = capacity;
		if(frameQueue_.size() < maxLength_)
			writeWaitEvent_.Set();
	}

	unsigned int maxLength_;
	utils::Event event_;
	std::list<FramePtr> frameQueue_;
	utils::Event writeWaitEvent_; 
};
	
MotionFrameBuffer::MotionFrameBuffer() : pImpl_(new Implementation())
{}

MotionFrameBuffer::MotionFrameBuffer(unsigned int maxQueueLength) : pImpl_(new Implementation(maxQueueLength))
{}

FramePtr MotionFrameBuffer::front() const 
{
	return pImpl_->front();
}

int MotionFrameBuffer::size() const 
{
	return pImpl_->size();
}

FrameBufferFetchResult MotionFrameBuffer::pop_front() 
{	
	return pImpl_->pop_front();
}

void MotionFrameBuffer::push_back(FramePtr pFrame) 
{	
	pImpl_->push_back(pFrame);
}

void MotionFrameBuffer::clear() 
{
	pImpl_->clear();
}

HANDLE MotionFrameBuffer::GetWaitHandle() const 
{
	return pImpl_->GetWaitHandle();
}

HANDLE MotionFrameBuffer::GetWriteWaitHandle() 
{
	return pImpl_->GetWriteWaitHandle();
}

void MotionFrameBuffer::SetCapacity(size_t capacity)
{
	pImpl_->SetCapacity(capacity);
}

}