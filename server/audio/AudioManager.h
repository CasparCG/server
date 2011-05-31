#pragma once

#include "..\frame\Frame.h"

namespace caspar {

class FrameMediaController;
struct MediaProducerInfo;

namespace audio {

class AudioDataChunk 
{
public:
	AudioDataChunk(int len) : length_(len), pData_(new char[len])
	{}

	~AudioDataChunk() {
		if(pData_ != 0)
			delete[] pData_;

		pData_ = 0;
		length_ = 0;
	}

	char* GetDataPtr() {
		return pData_;
	}
	int GetLength() {
		return length_;
	}

private:
	char* pData_;
	int length_;
};
typedef std::tr1::shared_ptr<AudioDataChunk> AudioDataChunkPtr;

class ISoundBufferWorker 
{
public:
	virtual ~ISoundBufferWorker()
	{}

	virtual void Start() = 0;
	virtual void Stop() = 0;

	virtual bool PushChunk(AudioDataChunkPtr) = 0;
	virtual HANDLE GetWaitHandle() = 0;
};
typedef std::tr1::shared_ptr<ISoundBufferWorker> SoundBufferWorkerPtr;

class IAudioManager 
{
public:
	virtual ~IAudioManager()
	{}

	virtual bool CueAudio(FrameMediaController*) = 0;
	virtual bool StartAudio(FrameMediaController*) = 0;
	virtual bool StopAudio(FrameMediaController*) = 0;
	virtual bool PushAudioData(FrameMediaController*, FramePtr) = 0;
};

}	//namespace audio
}	//namespace caspar