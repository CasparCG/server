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

#include "AudioConsumer.h"
#include "..\..\frame\FramePlaybackControl.h"
#include "..\..\frame\SystemFrameManager.h"
#include "..\..\frame\Frame.h"
#include "..\..\frame\FramePlaybackStrategy.h"
#include "..\..\utils\image\Image.hpp"

namespace caspar {
namespace audio {


struct AudioConsumer::Implementation
{
	struct AudioPlaybackStrategy : public IFramePlaybackStrategy
	{
		explicit AudioPlaybackStrategy(Implementation* pConsumerImpl) : pConsumerImpl_(pConsumerImpl)
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

		void DisplayFrame(Frame* pFrame)
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

			lastFrameID_ = pFrame->ID();
		}

		Implementation* pConsumerImpl_;
		DWORD lastTime_;	
		utils::ID lastFrameID_;
	};

	explicit Implementation(const FrameFormatDescription& fmtDesc) : fmtDesc_(fmtDesc)
	{	
		SetupDevice();
	}

	~Implementation()
	{
		ReleaseDevice();
	}

	bool SetupDevice()
	{
		pFrameManager_.reset(new SystemFrameManager(fmtDesc_));
		pPlaybackControl_.reset(new FramePlaybackControl(FramePlaybackStrategyPtr(new AudioPlaybackStrategy(this))));

		pPlaybackControl_->Start();
		return true;
	}

	bool ReleaseDevice()
	{
		pPlaybackControl_->Stop();
		return true;
	}

	FramePlaybackControlPtr pPlaybackControl_;
	SystemFrameManagerPtr pFrameManager_;

	FrameFormatDescription fmtDesc_;
};

AudioConsumer::AudioConsumer(const FrameFormatDescription& fmtDesc) : pImpl_(new Implementation(fmtDesc))
{}

AudioConsumer::~AudioConsumer()
{}

IPlaybackControl* AudioConsumer::GetPlaybackControl() const
{
	return pImpl_->pPlaybackControl_.get();
}

bool AudioConsumer::SetupDevice(unsigned int deviceIndex)
{
	return pImpl_->SetupDevice();
}

bool AudioConsumer::ReleaseDevice()
{
	return pImpl_->ReleaseDevice();
}

void AudioConsumer::EnableVideoOutput(){}
void AudioConsumer::DisableVideoOutput(){}

const FrameFormatDescription& AudioConsumer::GetFrameFormatDescription() const {
	return pImpl_->fmtDesc_;
}
const TCHAR* AudioConsumer::GetFormatDescription() const {
	return pImpl_->fmtDesc_.name;
}


}	//namespace audio
}	//namespace caspar