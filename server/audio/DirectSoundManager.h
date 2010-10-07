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