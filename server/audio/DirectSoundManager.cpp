#include "..\StdAfx.h"

#include "..\utils\thread.h"
#include "..\utils\lockable.h"
#include <queue>

#include <mmsystem.h>
#include <dsound.h>

#include "..\MediaProducerInfo.h"
#include "..\frame\FrameMediaController.h"
#include "DirectSoundManager.h"

namespace caspar {
namespace directsound {

using namespace audio;
using namespace utils;

///////////////////////////////
// DirectSoundBufferWorker
///////////////////////////////
class DirectSoundBufferWorker : public caspar::audio::ISoundBufferWorker, private utils::IRunnable, private utils::LockableObject
{
	friend class DirectSoundManager;

	DirectSoundBufferWorker(const DirectSoundBufferWorker&);
	DirectSoundBufferWorker& operator=(const DirectSoundBufferWorker&);

public:
	static const int BufferLengthInFrames;

	virtual ~DirectSoundBufferWorker();

	void Start() {
		worker_.Start(this);
	}
	void Stop() {
		worker_.Stop();
	}

	bool PushChunk(AudioDataChunkPtr);

	HANDLE GetWaitHandle() {
		return writeEvent_;
	}

private:
	AudioDataChunkPtr GetNextChunk();

private:
	explicit DirectSoundBufferWorker(LPDIRECTSOUNDBUFFER8 pDirectSound);
	HRESULT InitSoundBuffer(WORD channels, WORD bits, DWORD samplesPerSec, DWORD fps);

	virtual void Run(HANDLE stopEvent);
	virtual bool OnUnhandledException(const std::exception&) throw();
	utils::Thread worker_;


	void WriteChunkToBuffer(int offset, AudioDataChunkPtr pChunk);
	int IncreaseFrameIndex(int frameIndex) {
		return (frameIndex+1)%BufferLengthInFrames;
	}

	HANDLE notificationEvents_[2];

	int bytesPerFrame_;
	bool bIsRunning_;

	LPDIRECTSOUNDBUFFER8 pSoundBuffer_;

	std::queue<AudioDataChunkPtr> chunkQueue_;
	utils::Event writeEvent_;
	utils::Event startPlayback_;

	int soundBufferLoadIndex_;
	int lastPlayIndex_;
};
typedef std::tr1::shared_ptr<DirectSoundBufferWorker> DirectSoundBufferWorkerPtr;


///////////////////////////////
//
// DirectSoundManager
//
///////////////////////////////
DirectSoundManager::DirectSoundManager() : pDirectSound_(0)
{
}

DirectSoundManager::~DirectSoundManager()
{
	Destroy();
}

bool DirectSoundManager::Initialize(HWND hWnd, DWORD channels, DWORD samplesPerSec, DWORD bitsPerSample) {
#ifndef DISABLE_AUDIO
	HRESULT hr = E_FAIL;
	hr = DirectSoundCreate8(NULL, &pDirectSound_, NULL);
	if(FAILED(hr)) {
		LOG << TEXT("DirectSound: Failed to create device.");
		return false;
	}

	hr = pDirectSound_->SetCooperativeLevel(hWnd, DSSCL_PRIORITY);
	if(FAILED(hr)) {
		LOG << TEXT("DirectSound: Failed to set CooperativeLevel.");
		return false;
	}

	hr = SetPrimaryBufferFormat(channels, samplesPerSec, bitsPerSample);
	if(FAILED(hr)) {
		LOG << TEXT("DirectSound: Failed to set Primarybuffer format.");
		return false;
	}
#endif
	return true;
}

void DirectSoundManager::Destroy() {
	if(pDirectSound_ != 0) {
		pDirectSound_->Release();
		pDirectSound_ = 0;
	}
}

bool DirectSoundManager::CueAudio(FrameMediaController* pController) 
{
	MediaProducerInfo clipInfo;
#ifndef DISABLE_AUDIO
	if(pController->GetProducerInfo(&clipInfo) && clipInfo.HaveAudio) {
		caspar::audio::SoundBufferWorkerPtr pSBW = CreateSoundBufferWorker(clipInfo.AudioChannels, clipInfo.BitsPerAudioSample, clipInfo.AudioSamplesPerSec, 25);
		if(pSBW) {
			pController->AddSoundBufferWorker(pSBW);
			return true;
		}
	}
#endif
	return false;
}

bool DirectSoundManager::StartAudio(FrameMediaController* pController)
{
#ifndef DISABLE_AUDIO
	SoundBufferWorkerList& sbwList = pController->GetSoundBufferWorkers();
	SoundBufferWorkerList::iterator it = sbwList.begin();
	SoundBufferWorkerList::iterator end = sbwList.end();
	for(;it != end; ++it)
	{
		(*it)->Start();
	}
#endif
	return true;
}

bool DirectSoundManager::StopAudio(FrameMediaController* pController)
{
#ifndef DISABLE_AUDIO
	SoundBufferWorkerList& sbwList = pController->GetSoundBufferWorkers();
	SoundBufferWorkerList::iterator it = sbwList.begin();
	SoundBufferWorkerList::iterator end = sbwList.end();
	for(;it != end; ++it)
	{
		(*it)->Stop();
	}
#endif
	return true;
}

bool DirectSoundManager::PushAudioData(FrameMediaController* pController, FramePtr pFrame)
{
#ifndef DISABLE_AUDIO
	SoundBufferWorkerList& sbwList = pController->GetSoundBufferWorkers();
	AudioDataChunkList data = pFrame->GetAudioData();

	SoundBufferWorkerList::iterator it = sbwList.begin();
	SoundBufferWorkerList::iterator end = sbwList.end();

	for(int dataIndex = 0;it != end && dataIndex < data.size(); ++it, ++dataIndex) {
		if(dataIndex < pFrame->GetAudioData().size())
			(*it)->PushChunk(pFrame->GetAudioData()[dataIndex]);
		else
			break;
	}
#endif
	return true;
}

SoundBufferWorkerPtr DirectSoundManager::CreateSoundBufferWorker(WORD channels, WORD bits, DWORD samplesPerSec, DWORD fps) {
	SoundBufferWorkerPtr result;

	if(pDirectSound_ != 0) {
		WAVEFORMATEX wfx;
		DSBUFFERDESC bufferDesc;
		LPDIRECTSOUNDBUFFER pSoundBuffer;
		LPDIRECTSOUNDBUFFER8 pSoundBuffer8;

		HRESULT hr;

		ZeroMemory(&wfx, sizeof(wfx));
		wfx.cbSize = sizeof(wfx);
		wfx.wFormatTag = WAVE_FORMAT_PCM;
		wfx.nChannels = channels;
		wfx.wBitsPerSample = bits;
		wfx.nSamplesPerSec = samplesPerSec;
		wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;
		wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

		int bytesPerFrame = wfx.nAvgBytesPerSec / fps;

		ZeroMemory(&bufferDesc, sizeof(bufferDesc));
		bufferDesc.dwSize = sizeof(bufferDesc);
		bufferDesc.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME;
		bufferDesc.dwBufferBytes = bytesPerFrame * DirectSoundBufferWorker::BufferLengthInFrames;
		bufferDesc.lpwfxFormat = &wfx;

		hr = pDirectSound_->CreateSoundBuffer(&bufferDesc, &pSoundBuffer, NULL);
		if(SUCCEEDED(hr)) {
			hr = pSoundBuffer->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*) &pSoundBuffer8);
			pSoundBuffer->Release();
			if(FAILED(hr)) {
				LOG << TEXT("DirectSound: Failed to create SoundBuffer.");
				return result;
			}
		}
		else {
			LOG << TEXT("DirectSound: Failed to create SoundBuffer.");
			return result;
		}

		DirectSoundBufferWorkerPtr pSBW(new DirectSoundBufferWorker(pSoundBuffer8));
		if(FAILED(pSBW->InitSoundBuffer(channels, bits, samplesPerSec, fps))) {
			LOG << TEXT("DirectSound: Failed to init SoundBuffer.");
			return result;
		}
		pSBW->Start();
		result = pSBW;
	}

	return result;
}

HRESULT DirectSoundManager::SetPrimaryBufferFormat(DWORD dwPrimaryChannels, DWORD dwPrimaryFreq,  DWORD dwPrimaryBitRate)
{
	HRESULT hr;
	LPDIRECTSOUNDBUFFER pDSBPrimary = NULL;

	if(pDirectSound_ == NULL )
		return CO_E_NOTINITIALIZED;

	// Get the primary buffer 
	DSBUFFERDESC dsbd;
	ZeroMemory( &dsbd, sizeof(DSBUFFERDESC) );
	dsbd.dwSize        = sizeof(DSBUFFERDESC);
	dsbd.dwFlags       = DSBCAPS_PRIMARYBUFFER;
	dsbd.dwBufferBytes = 0;
	dsbd.lpwfxFormat   = NULL;

	hr = pDirectSound_->CreateSoundBuffer( &dsbd, &pDSBPrimary, NULL );
	if(FAILED(hr))
		return hr;

	WAVEFORMATEX wfx;
	ZeroMemory( &wfx, sizeof(WAVEFORMATEX) ); 
	wfx.wFormatTag		= (WORD) WAVE_FORMAT_PCM; 
	wfx.nChannels		= (WORD) dwPrimaryChannels; 
	wfx.nSamplesPerSec	= (DWORD) dwPrimaryFreq; 
	wfx.wBitsPerSample	= (WORD) dwPrimaryBitRate; 
	wfx.nBlockAlign		= (WORD) (wfx.wBitsPerSample / 8 * wfx.nChannels);
	wfx.nAvgBytesPerSec	= (DWORD) (wfx.nSamplesPerSec * wfx.nBlockAlign);

	hr = pDSBPrimary->SetFormat(&wfx);
	if(FAILED(hr)) {
		pDSBPrimary->Release();
		pDSBPrimary = 0;
		return hr;
	}

	pDSBPrimary->Release();
	pDSBPrimary = 0;

	return S_OK;
}

/////////////////////////////////
//
// DirectSoundBufferWorker Impl
//
/////////////////////////////////
const int DirectSoundBufferWorker::BufferLengthInFrames = 3;

DirectSoundBufferWorker::DirectSoundBufferWorker(LPDIRECTSOUNDBUFFER8 pSoundBuffer) : writeEvent_(TRUE, TRUE), bytesPerFrame_(0), pSoundBuffer_(pSoundBuffer), bIsRunning_(false), startPlayback_(FALSE, FALSE), soundBufferLoadIndex_(0), lastPlayIndex_(0)
{
	//reserve the first event for the stopEvent
	notificationEvents_[0] = 0;
	notificationEvents_[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
}

DirectSoundBufferWorker::~DirectSoundBufferWorker(void) {
	Stop();

	pSoundBuffer_->Release();
	pSoundBuffer_ = 0;

	CloseHandle(notificationEvents_[1]);
	notificationEvents_[1] = 0;
}

void DirectSoundBufferWorker::Run(HANDLE stopEvent) {
	bool bQuit = false;
	notificationEvents_[0] = stopEvent;

	{
		HANDLE waitEvents[2] = {stopEvent, startPlayback_};
		HRESULT waitResult = WAIT_TIMEOUT;
		while(waitResult == WAIT_TIMEOUT || waitResult == WAIT_OBJECT_0)
		{
			waitResult = WaitForMultipleObjects(2, waitEvents, FALSE, 2500);
			if(waitResult == WAIT_OBJECT_0)
				goto workerloop_end;
		}
	}

	bIsRunning_ = true;
	HRESULT hr = pSoundBuffer_->Play(0, 0, DSBPLAY_LOOPING);
	while(!bQuit) {
		DWORD waitResult = WaitForMultipleObjects(2, notificationEvents_, FALSE, 2500);
		switch(waitResult) {
			case WAIT_OBJECT_0:	//stopEvent
				bQuit = true;
				break;

			case WAIT_TIMEOUT:
				break;

			case WAIT_FAILED:
				bQuit = true;
				break;

			default:
				{
					DWORD currentPlayCursor = 0, currentWriteCursor = 0;
					HRESULT hr = pSoundBuffer_->GetCurrentPosition(&currentPlayCursor, &currentWriteCursor);
					if(SUCCEEDED(hr)) {
						int currentPlayCursorIndex = currentPlayCursor / bytesPerFrame_;
						if(currentPlayCursorIndex != lastPlayIndex_) {
							AudioDataChunkPtr pChunk = GetNextChunk();
							int offset = lastPlayIndex_ * bytesPerFrame_;

							WriteChunkToBuffer(offset, pChunk);

							lastPlayIndex_ = currentPlayCursorIndex;
						}
					}
				}
				break;
		}
	}

workerloop_end:
	pSoundBuffer_->Stop();
	bIsRunning_ = false;
}

bool DirectSoundBufferWorker::OnUnhandledException(const std::exception&) throw() {
	try {
		if(pSoundBuffer_ != 0)
			pSoundBuffer_->Stop();

		LOG << TEXT("UNEXPECTED EXCEPTION in SoundBufferWorker.");
	}
	catch(...) 
	{}

	return false;
}

void DirectSoundBufferWorker::WriteChunkToBuffer(int offset, AudioDataChunkPtr pChunk) {
	void* pPtr;
	DWORD len;

	HRESULT hr = pSoundBuffer_->Lock(offset, bytesPerFrame_, &pPtr, &len, NULL, NULL, 0); 
	if(SUCCEEDED(hr)) {
		if(pChunk != 0) {
			//len and pChunk-length SHOULD be the same, but better safe than sorry
			memcpy(pPtr, pChunk->GetDataPtr(), min(len, pChunk->GetLength()));
		}
		else {
			memset(pPtr, 0, len);
		}

		pSoundBuffer_->Unlock(pPtr, len, NULL, 0);
	}
}

bool DirectSoundBufferWorker::PushChunk(AudioDataChunkPtr pChunk) {
	Lock lock(*this);

	//WaitForSingleObject(writeEvent_, 200);

	if(!bIsRunning_) {
		if(soundBufferLoadIndex_ < 3) {
			WriteChunkToBuffer(bytesPerFrame_ * soundBufferLoadIndex_, pChunk);
			++soundBufferLoadIndex_;
			return true;
		}
		else
			startPlayback_.Set();
	}

	chunkQueue_.push(pChunk);

	if(chunkQueue_.size() >= 5)
		writeEvent_.Reset();

	return true;
}

AudioDataChunkPtr DirectSoundBufferWorker::GetNextChunk() {
	Lock lock(*this);
	AudioDataChunkPtr pChunk;

	if(chunkQueue_.size() > 0) {
		pChunk = chunkQueue_.front();
		chunkQueue_.pop();
	}

	if(chunkQueue_.size() < 5)
		writeEvent_.Set();

	return pChunk;
}

HRESULT DirectSoundBufferWorker::InitSoundBuffer(WORD channels, WORD bits, DWORD samplesPerSec, DWORD fps) {

	bytesPerFrame_ = samplesPerSec * channels * (bits/8) / fps;
	DWORD bufferSize = bytesPerFrame_ * BufferLengthInFrames;
	
	LPDIRECTSOUNDNOTIFY8 pBufferNotify;
	HRESULT hr = pSoundBuffer_->QueryInterface(IID_IDirectSoundNotify8, (LPVOID*) &pBufferNotify);
	if(SUCCEEDED(hr)) {
		DSBPOSITIONNOTIFY pPositionNotifies[BufferLengthInFrames];

		for(int i=0; i < BufferLengthInFrames; ++i) {
			pPositionNotifies[i].dwOffset = (i+1)*bytesPerFrame_ - 1;
			pPositionNotifies[i].hEventNotify = notificationEvents_[1];
		}

		hr = pBufferNotify->SetNotificationPositions(BufferLengthInFrames, &(pPositionNotifies[0]));
		pBufferNotify->Release();
	}
	else 
		return hr;

	//Init the buffer to silence
	void* pPtr = 0;
	DWORD len = 0;
	hr = pSoundBuffer_->Lock(0, bufferSize, &pPtr, &len, NULL, NULL, 0); 
	if(SUCCEEDED(hr) && pPtr != 0) {
		memset(pPtr, 0, len);
		pSoundBuffer_->Unlock(pPtr, len, NULL, 0);
	}

	return hr;
}

}	//namespace directsound
}	//namespace caspar