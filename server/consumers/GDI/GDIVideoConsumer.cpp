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
 
#include "..\..\stdafx.h"

#include "GDIVideoConsumer.h"
#include "..\..\frame\FramePlaybackControl.h"
#include "..\..\frame\BitmapFrameManager.h"
#include "..\..\frame\Frame.h"
#include "..\..\frame\FramePlaybackStrategy.h"
#include "..\..\utils\DCWrapper.h"
#include "..\..\utils\BitmapHolder.h"
#include "..\..\utils\image\Image.hpp"

namespace caspar {
namespace gdi {


struct GDIVideoConsumer::Implementation
{
	struct GDIPlaybackStrategy : public IFramePlaybackStrategy
	{
		GDIPlaybackStrategy(Implementation* pConsumerImpl) : pConsumerImpl_(pConsumerImpl), pTempBitmapData_(new BitmapHolder(pConsumerImpl_->hWnd_, pConsumerImpl_->fmtDesc_.width, pConsumerImpl_->fmtDesc_.height))
		{
			lastTime_ = timeGetTime();
		}

		FrameManagerPtr GetFrameManager()
		{
			return pConsumerImpl_->pFrameManager_;
		}
		FramePtr GetReservedFrame()
		{
			return pConsumerImpl_->pFrameManager_->CreateFrame();
		}

		void DisplayFrame(FramePtr pFrame)
		{
			DWORD timediff = timeGetTime() - lastTime_;
			if(timediff < 30) {
				Sleep(40 - timediff);
				lastTime_ += 40;
			}
			else
				lastTime_ = timeGetTime();

			if(pFrame == NULL || pFrame->ID() == lastFrameID_)
				return;

			DCWrapper hDC(pConsumerImpl_->hWnd_);

			RECT rect;
			GetClientRect(pConsumerImpl_->hWnd_, &rect);
			if(this->GetFrameManager()->Owns(*pFrame))
				BitBlt(hDC, rect.left, rect.top, rect.right, rect.bottom, reinterpret_cast<HDC>(pFrame->GetMetadata()), 0, 0, SRCCOPY);
			else {
				utils::image::Copy(pTempBitmapData_->GetPtr(), pFrame->GetDataPtr(), pFrame->GetDataSize());
				BitBlt(hDC, rect.left, rect.top, rect.right, rect.bottom, pTempBitmapData_->GetDC(), 0, 0, SRCCOPY);
			}

			lastFrameID_ = pFrame->ID();
		}

		Implementation* pConsumerImpl_;
		BitmapHolderPtr pTempBitmapData_;		
		DWORD lastTime_;	
		utils::ID lastFrameID_;
	};

	Implementation(HWND hWnd, const FrameFormatDescription& fmtDesc) : hWnd_(hWnd), fmtDesc_(fmtDesc)
	{	
		SetupDevice();
	}

	~Implementation()
	{
		ReleaseDevice();
	}

	bool SetupDevice()
	{
		pFrameManager_.reset(new BitmapFrameManager(fmtDesc_, hWnd_));
		pPlaybackControl_.reset(new FramePlaybackControl(FramePlaybackStrategyPtr(new GDIPlaybackStrategy(this))));

		pPlaybackControl_->Start();
		return true;
	}

	bool ReleaseDevice()
	{
		pPlaybackControl_->Stop();
		return true;
	}

	FramePlaybackControlPtr pPlaybackControl_;
	BitmapFrameManagerPtr pFrameManager_;

	HWND	hWnd_;
	FrameFormatDescription fmtDesc_;
};

GDIVideoConsumer::GDIVideoConsumer(HWND hWnd, const FrameFormatDescription& fmtDesc) : pImpl_(new Implementation(hWnd, fmtDesc))
{}

GDIVideoConsumer::~GDIVideoConsumer()
{}

IPlaybackControl* GDIVideoConsumer::GetPlaybackControl() const
{
	return pImpl_->pPlaybackControl_.get();
}

bool GDIVideoConsumer::SetupDevice(unsigned int deviceIndex)
{
	return pImpl_->SetupDevice();
}

bool GDIVideoConsumer::ReleaseDevice()
{
	return pImpl_->ReleaseDevice();
}

void GDIVideoConsumer::EnableVideoOutput(){}
void GDIVideoConsumer::DisableVideoOutput(){}

const FrameFormatDescription& GDIVideoConsumer::GetFrameFormatDescription() const {
	return pImpl_->fmtDesc_;
}
const TCHAR* GDIVideoConsumer::GetFormatDescription() const {
	return pImpl_->fmtDesc_.name;
}


}	//namespace gdi
}	//namespace caspar