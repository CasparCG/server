#pragma once

#include "AudioManager.h"

struct IDirectSound8;
typedef IDirectSound8*	LPDIRECTSOUND8;

namespace caspar {
namespace directsound {

class DirectSoundManager : public caspar::audio::IAudioManager
{
	DirectSoundManager(const DirectSoundManager&);
	DirectSoundManager& operator=(const DirectSoundManager&);
	DirectSoundManager();

public:
	static DirectSoundManager* GetInstance() {
		static DirectSoundManager instance;
		return &instance;
	}

	~DirectSoundManager();

	bool Initialize(HWND hWnd, DWORD channels, DWORD samplesPerSec, DWORD bitsPerSample);
	void Destroy();

	virtual bool CueAudio(FrameMediaController*);
	virtual bool StartAudio(FrameMediaController*);
	virtual bool StopAudio(FrameMediaController*);
	virtual bool PushAudioData(FrameMediaController*, FramePtr);

private:
	caspar::audio::SoundBufferWorkerPtr CreateSoundBufferWorker(WORD channels, WORD bits, DWORD samplesPerSec, DWORD fps);
	HRESULT SetPrimaryBufferFormat(DWORD dwPrimaryChannels, DWORD dwPrimaryFreq,  DWORD dwPrimaryBitRate);

	IDirectSound8* pDirectSound_;
};

}	//namespace directsound
}	//namespace caspar