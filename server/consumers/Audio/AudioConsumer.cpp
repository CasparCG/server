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