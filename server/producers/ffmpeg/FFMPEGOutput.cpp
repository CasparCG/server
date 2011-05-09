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

#include "FFMPEGOutput.h"

#include "FFMPEGProducer.h"
#include "FFMPEGPacket.h"
#include "video/FFMPEGVideoDecoder.h"
#include "audio/FFMPEGAudioDecoder.h"

#include "../../frame/buffers/MotionFrameBuffer.h"
#include "../../utils/image/Image.hpp"
#include "../../utils/Event.h"

#include <tbb/concurrent_queue.h>

#include <memory>
#include <queue>

namespace caspar
{
	namespace ffmpeg
	{

struct OutputFilter::Implementation : public FFMPEGFilterImpl<FFMPEGVideoFrame>
{
	Implementation() : master_(tbb::this_tbb_thread::get_id()), stopEvent_(true, false)
	{
		isRunning_ = true;
		isBuffered_ = false;
		SetCapacity(FFMPEGProducer::DEFAULT_BUFFER_SIZE);
	}
		
	void* process(FFMPEGVideoFrame* pVideoFrame)
	{				
		if(isRunning_)
		{						
			deferred_.push(pVideoFrame->pFrame);
		
			if(isBuffered_)
				SendFrames();					
			else
				isBuffered_ =  bufferTarget_ < deferred_.size();		
		}

		delete pVideoFrame;

		return nullptr;
	}

	void SendFrames()
	{		
		HANDLE handles[2] = { stopEvent_, frameBuffer.GetWriteWaitHandle() };
		DWORD timeout = tbb::this_tbb_thread::get_id() == master_ ? INFINITE : 0; // Only block on master thread
		while(!deferred_.empty() && WaitForMultipleObjects(2, handles, FALSE, timeout) == WAIT_OBJECT_0 + 1 && isRunning_)
		{
			frameBuffer.push_back(deferred_.front());
			deferred_.pop();
		}
	}
	
	void Stop()
	{
		if(isRunning_.fetch_and_store(false))
			frameBuffer.push_back(nullptr);
		stopEvent_.Set();
	}
			
	void SetCapacity(size_t capacity)
	{
		if(capacity < 2)
			throw std::exception("[FFMPEGOutput::SetCapacity] Invalid Argument: capacity");
		frameBuffer.SetCapacity(capacity);
		bufferTarget_ = min(FFMPEGProducer::LOAD_TARGET_BUFFER_SIZE, capacity);
	}	

	void SetMaster(tbb::tbb_thread::id master)
	{
		master_ = master;
	}

	void Flush() // Flush frames that were not sent by the master thread while running
	{			
		SendFrames();
		if(isRunning_.fetch_and_store(false))
			frameBuffer.push_back(nullptr);
	}
	
	FrameBuffer& GetFrameBuffer()
	{
		return frameBuffer;
	}
				
	MotionFrameBuffer frameBuffer;

	utils::Event stopEvent_;
	std::queue<FramePtr> deferred_;
	tbb::tbb_thread::id master_;
	tbb::atomic<int> bufferTarget_;
	tbb::atomic<bool> isRunning_;
	tbb::atomic<bool> isBuffered_;
};

OutputFilter::OutputFilter() : tbb::filter(serial_in_order), pImpl_(new Implementation())
{
}

void* OutputFilter::operator()(void* item)
{
	return (*pImpl_)(item);
}

void OutputFilter::Stop()
{
	pImpl_->Stop();
}

void OutputFilter::SetCapacity(size_t capacity)
{
	pImpl_->SetCapacity(capacity); 
}

void OutputFilter::SetMaster(tbb::tbb_thread::id master)
{
	pImpl_->SetMaster(master);
}

void OutputFilter::Flush()
{
	pImpl_->Flush();
}

FrameBuffer& OutputFilter::GetFrameBuffer()
{
	return pImpl_->GetFrameBuffer();
}


	}
}