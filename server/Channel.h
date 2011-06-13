#pragma once

#include "VideoConsumer.h"
#include "MediaProducer.h"
#include "PlaybackControl.h"
#include "producers\composites\TransitionProducer.h"
#include "TransitionInfo.h"
#include "frame\clipinfo.h"

namespace caspar {

class Channel
{
public:
	explicit Channel(int index, VideoConsumerPtr pVideoConsumer);
	~Channel();

	bool Initialize();	//call from app in main-thread
	void Destroy();		//call from app in main-thread

	const TCHAR* GetFormatDescription() const;
	bool IsPlaybackRunning() const;

	int GetIndex() const {
		return index_;
	}

	CG::ICGControl* GetCGControl();

	bool Load(MediaProducerPtr, bool loop=false);			//call from misc IO threads
	bool LoadBackground(MediaProducerPtr, const TransitionInfo& transitionInfo, bool loop=false);	//call from misc IO threads
	bool Play();	//call from misc IO threads
	bool Stop(bool block = false);	//call from misc IO threads
	bool Param(const tstring& str);	//call from misc IO threads
	bool Clear();
	bool SetVideoFormat(const tstring& strDesiredFrameFormat);

private:
	int index_;
	VideoConsumerPtr	pConsumer_;
};
typedef std::tr1::shared_ptr<Channel> ChannelPtr;

}	//namespace caspar