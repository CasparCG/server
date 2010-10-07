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
 
#ifndef _CASPAR_FRAMEMEDIACONTROLLER_H__
#define _CASPAR_FRAMEMEDIACONTROLLER_H__

#include "..\MediaController.h"
#include "FrameManager.h"
#include "..\audio\AudioManager.h"

#include <vector>

namespace caspar {

class FrameBuffer;
struct MediaProducerInfo;

typedef std::vector<caspar::audio::SoundBufferWorkerPtr> SoundBufferWorkerList;

class FrameMediaController : public IMediaController
{
	FrameMediaController(const FrameMediaController&);
	FrameMediaController& operator=(const FrameMediaController&);

public:
	FrameMediaController() {}
	virtual ~FrameMediaController() {}

	virtual bool Initialize(FrameManagerPtr pFrameManager) = 0;

	virtual FrameBuffer& GetFrameBuffer() = 0;
	virtual bool GetProducerInfo(MediaProducerInfo*) {
		return false;
	}

	SoundBufferWorkerList& GetSoundBufferWorkers() {
		return soundBufferWorkers_;
	}

	void AddSoundBufferWorker(caspar::audio::SoundBufferWorkerPtr pSoundBufferWorker) {
		soundBufferWorkers_.push_back(pSoundBufferWorker);
	}

private:
	SoundBufferWorkerList soundBufferWorkers_;
};

}	//namespace caspar

#endif	//_CASPAR_FRAMEMEDIACONTROLLER_H__