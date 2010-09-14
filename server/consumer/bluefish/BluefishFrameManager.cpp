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
 
#include "..\..\StdAfx.h"

#include <BlueVelvet4.h>
#include "BluefishFrameManager.h"
#include <tbb/scalable_allocator.h>

#define	BUFFER_ID_USER_BASE		(6)

namespace caspar {
namespace bluefish {

//////////////////////
// CardFrameInfo
//
CardFrameInfo::CardFrameInfo(BlueVelvetPtr pSDK, int dataSize, int bufferID) : pSDK_(pSDK), dataSize_(dataSize), pData_(0), bufferID_(bufferID)
{
	if(BLUE_FAIL(pSDK->system_buffer_map(reinterpret_cast<void**>(&pData_), bufferID)))
	{
		throw BluefishException("Failed to map buffer");
	}
}

CardFrameInfo::~CardFrameInfo()
{
	try
	{
		if(pSDK_ != 0 && pData_ != 0)
			pSDK_->system_buffer_unmap(pData_);
	}
	catch(...) {}
}


//////////////////////
// SystemFrameInfo
//
SystemFrameInfo::SystemFrameInfo(int dataSize, int bufferID) : dataSize_(dataSize), pData_(0), bufferID_(bufferID)
{
	pData_ = static_cast<unsigned char*>(::VirtualAlloc(NULL, dataSize_, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	if(pData_ == 0)
	{
		throw BluefishException("Failed to allocate memory for frame");
	}
	if(::VirtualLock(pData_, dataSize_) == 0)
	{
		throw BluefishException("Failed to lock memory for frame");
	}
}

SystemFrameInfo::~SystemFrameInfo()
{
	try
	{
		if(pData_ != 0)
			::VirtualFree(pData_, 0, MEM_RELEASE);
	}
	catch(...) {}
}

////////////////////////
// BluefishVideoFrame
//
BluefishVideoFrame::BluefishVideoFrame(BluefishFrameManager* pFrameManager) : pFrameManager_(pFrameManager)
{
	if(pFrameManager_ != 0)
		pInfo_ = pFrameManager_->GetBuffer();
	if(pInfo_ != nullptr)
	{
		size_ = pInfo_->size();
		data_ = pInfo_->GetPtr();
		buffer_id_ = pInfo_->GetBufferID();
	}
	else
	{
		size_ = pFrameManager->get_frame_format_desc().size;
		data_ = static_cast<unsigned char*>(scalable_aligned_malloc(16, size_));
		buffer_id_ = 0;
	}
}

BluefishVideoFrame::~BluefishVideoFrame()
{
	if(pFrameManager_ != 0 && pInfo_ != 0)
		pFrameManager_->ReturnBuffer(pInfo_);

	if(pInfo_ != nullptr)
	{
	}
	else
	{
		scalable_aligned_free(data_);
	}
}

//////////////////////////////
// BluefishVideoFrameFactory
//
BluefishFrameManager::BluefishFrameManager(BlueVelvetPtr pSDK, frame_format fmt, unsigned long optimalLength) : pSDK_(pSDK), format_(fmt)
{
	//const frame_format_desc& format_desc = frame_format_desc::format_descs[fmt];

	SIZE_T workingSetMinSize = 0, workingSetMaxSize = 0;
	if(::GetProcessWorkingSetSize(::GetCurrentProcess(), &workingSetMinSize, &workingSetMaxSize) != 0)
	{
		CASPAR_LOG(debug) << "WorkingSet size: min = " << workingSetMinSize << TEXT(", max = ") << workingSetMaxSize;
		
		workingSetMinSize += optimalLength * 6;
		workingSetMaxSize += optimalLength * 6;

		if(::SetProcessWorkingSetSize(::GetCurrentProcess(), workingSetMinSize, workingSetMaxSize) == 0)
		{
			CASPAR_LOG(error) << "Failed to set workingset: min = " << workingSetMinSize << TEXT(", max = ") << workingSetMaxSize;
		}
	}

	//for(int cardBufferIndex = 0; cardBufferIndex < 4; ++cardBufferIndex)
	//{
	//	frameBuffers_.push_back(VideoFrameInfoPtr(new CardFrameInfo(pSDK, optimalLength, BUFFER_ID_VIDEO0 + cardBufferIndex)));
	//}
	for(int systemBufferIndex = 0; systemBufferIndex < 6; ++systemBufferIndex)
	{
		frameBuffers_.push_back(VideoFrameInfoPtr(new SystemFrameInfo(optimalLength, BUFFER_ID_USER_BASE + systemBufferIndex)));
	}

	FrameInfoList::const_iterator it = frameBuffers_.begin();
	FrameInfoList::const_iterator end = frameBuffers_.end();
	for(; it != end; ++it)
	{
		if(BLUE_FAIL(pSDK_->system_buffer_assign((*it)->GetPtr(), (*it)->GetBufferID(), (*it)->size(), BUFFER_TYPE_VIDEO)))
		{
			throw BluefishException("Failed to assign buffer");
		}
	}
}

BluefishFrameManager::~BluefishFrameManager()
{
}

std::shared_ptr<BluefishVideoFrame> BluefishFrameManager::CreateFrame()
{
	return std::make_shared<BluefishVideoFrame>(this);
}

const frame_format_desc& BluefishFrameManager::get_frame_format_desc() const
{
	return frame_format_desc::format_descs[format_];
}

VideoFrameInfoPtr BluefishFrameManager::GetBuffer()
{
	tbb::mutex::scoped_lock lock(mutex_);
	VideoFrameInfoPtr pInfo;

	if(frameBuffers_.size() > 0)
	{
		pInfo = frameBuffers_.front();
		frameBuffers_.pop_front();
	}

	return pInfo;
}

void BluefishFrameManager::ReturnBuffer(VideoFrameInfoPtr pInfo)
{
	tbb::mutex::scoped_lock lock(mutex_);
	if(pInfo != 0)
		frameBuffers_.push_back(pInfo);
}

}}