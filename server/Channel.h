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
 
#pragma once

#include "VideoConsumer.h"
#include "MediaProducer.h"
#include "PlaybackControl.h"
#include "producers\composites\TransitionProducer.h"
#include "TransitionInfo.h"
#include "frame\clipinfo.h"

#ifdef ENABLE_MONITOR
#include "monitor.h"
#endif

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
#ifdef ENABLE_MONITOR
	Monitor& GetMonitor() {
		return monitor_;
	}
#endif

	bool Load(MediaProducerPtr, bool loop=false);			//call from misc IO threads
	bool LoadBackground(MediaProducerPtr, const TransitionInfo& transitionInfo, bool loop=false);	//call from misc IO threads
	bool Play();	//call from misc IO threads
	bool Stop(bool block = false);	//call from misc IO threads
	bool Param(const tstring& str);	//call from misc IO threads
	bool Clear();
	bool SetVideoFormat(const tstring& strDesiredFrameFormat);


private:
#ifdef ENABLE_MONITOR
	Monitor monitor_;
#endif
	int index_;
	VideoConsumerPtr	pConsumer_;
};
typedef std::tr1::shared_ptr<Channel> ChannelPtr;

}	//namespace caspar