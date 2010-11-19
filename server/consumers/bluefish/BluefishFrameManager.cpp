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
#include "..\..\utils\Process.h"
#include "BluefishFrameManager.h"

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
	{
		factoryID_ = pFrameManager_->ID();
		pInfo_ = pFrameManager_->GetBuffer();
	}
}

BluefishVideoFrame::~BluefishVideoFrame()
{
	if(pFrameManager_ != 0 && pInfo_ != 0)
		pFrameManager_->ReturnBuffer(pInfo_);
}

//////////////////////////////
// BluefishVideoFrameFactory
//
BluefishFrameManager::BluefishFrameManager(BlueVelvetPtr pSDK, FrameFormat fmt, unsigned long optimalLength) : pSDK_(pSDK), format_(fmt)
{
	const FrameFormatDescription& fmtDesc = FrameFormatDescription::FormatDescriptions[fmt];
	pBackupFrameManager_.reset(new SystemFrameManager(fmtDesc));

	SIZE_T workingSetMinSize = 0, workingSetMaxSize = 0;
	if(utils::Process::GetCurrentProcess().GetWorkingSetSize(workingSetMinSize, workingSetMaxSize))
	{
		LOG << utils::LogLevel::Debug << TEXT("WorkingSet size: min = ") << workingSetMinSize << TEXT(", max = ") << workingSetMaxSize;
		
		workingSetMinSize += optimalLength * 6;
		workingSetMaxSize += optimalLength * 6;

		if(!utils::Process::GetCurrentProcess().SetWorkingSetSize(workingSetMinSize, workingSetMaxSize))
		{
			LOG << utils::LogLevel::Critical << TEXT("Failed to set workingset: min = ") << workingSetMinSize << TEXT(", max = ") << workingSetMaxSize;
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

	/*FrameInfoList::const_iterator it = frameBuffers_.begin();
	FrameInfoList::const_iterator end = frameBuffers_.end();
	for(; it != end; ++it)
	{
		if(BLUE_FAIL(pSDK_->system_buffer_assign((*it)->GetPtr(), (*it)->GetBufferID(), (*it)->GetDataSize(), BUFFER_TYPE_VIDEO)))
		{
			throw BluefishException("Failed to assign buffer");
		}
	}*/
}

BluefishFrameManager::~BluefishFrameManager()
{
}

FramePtr BluefishFrameManager::CreateFrame()
{
	FramePtr pBluefishFrame(new BluefishVideoFrame(this));
	if(pBluefishFrame->HasValidDataPtr())
		return pBluefishFrame;
	else
		return pBackupFrameManager_->CreateFrame();
}

const FrameFormatDescription& BluefishFrameManager::GetFrameFormatDescription() const
{
	return FrameFormatDescription::FormatDescriptions[format_];
}

VideoFrameInfoPtr BluefishFrameManager::GetBuffer()
{
	Lock lock(*this);
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
	Lock lock(*this);
	if(pInfo != 0)
		frameBuffers_.push_back(pInfo);
}

}	//namespace bluefish
}	//namespace caspar