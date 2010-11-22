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