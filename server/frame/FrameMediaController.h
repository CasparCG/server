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