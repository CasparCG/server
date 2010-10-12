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
 
#include "..\..\stdafx.h"

#include "..\..\Application.h"
#include "DecklinkVideoConsumer.h"
#include "..\..\frame\FramePlaybackControl.h"
#include "..\..\frame\Frame.h"
#include "..\..\frame\FramePlaybackStrategy.h"
#include "DeckLinkAPI_h.h"
#include "..\..\utils\image\Image.hpp"
#include "..\..\utils\event.h"

#include <queue>

namespace caspar {
namespace decklink {


struct DecklinkVideoConsumer::Implementation : public IDeckLinkVideoOutputCallback
{
	//struct DecklinkFrameQueue : private caspar::utils::LockableObject
	//{
	//	std::queue<FramePtr> availibleFrames;
	//	std::queue<FramePtr> scheduledFrames;

	//	void add_free(FramePtr pFrame) {
	//		availibleFrames.push(pFrame);
	//	}
	//};

	struct DecklinkFrameManager;
	struct DecklinkVideoFrame : public Frame {
		explicit DecklinkVideoFrame(DecklinkFrameManager* pFactory) : factoryID_(pFactory->ID()) {
			IDeckLinkMutableVideoFrame* pFrame = NULL;
			const FrameFormatDescription& fmtDesc = pFactory->pConsumerImpl_->GetFrameFormatDescription();
			if(pFactory->pConsumerImpl_->pDecklinkOutput_->CreateVideoFrame(fmtDesc.width, fmtDesc.height, fmtDesc.size/fmtDesc.height, bmdFormat8BitBGRA, bmdFrameFlagDefault, &pFrame) != S_OK) {
				throw std::exception("DECKLINK: Failed to create frame");
			}
			pDecklinkFrame_ = pFrame;
			pFrame->Release();

			if(pDecklinkFrame_->GetBytes((void**)&pBytes_) != S_OK)
				throw std::exception("DECKLINK: Failed to get bytes to frame");
		}
		virtual unsigned char* GetDataPtr() const {
			HasVideo(true);
			return pBytes_;
		}
		virtual bool HasValidDataPtr() const {
			return true;
		}

		virtual unsigned int GetDataSize() const {
			return pDecklinkFrame_->GetRowBytes() * pDecklinkFrame_->GetHeight();
		}
		virtual FrameMetadata GetMetadata() const {
			IDeckLinkMutableVideoFrame* pFramePtr = pDecklinkFrame_;
			return reinterpret_cast<FrameMetadata>(pFramePtr);
		}
		const utils::ID& FactoryID() const {
			return factoryID_;
		}

		utils::ID factoryID_;
		CComPtr<IDeckLinkMutableVideoFrame> pDecklinkFrame_;
		unsigned char* pBytes_;
	};

	struct DecklinkPlaybackStrategy : public IFramePlaybackStrategy
	{
		explicit DecklinkPlaybackStrategy(Implementation* pConsumerImpl) : pConsumerImpl_(pConsumerImpl), currentReservedFrameIndex_(0), totalFramesScheduled_(0)
		{
			for(int i = 0; i<4; ++i) {
				reservedFrames_.push_back(pConsumerImpl_->pFrameManager_->CreateReservedFrame());
			}
		}

		FrameManagerPtr GetFrameManager()
		{
			return pConsumerImpl_->pFrameManager_;
		}

		FramePtr GetReservedFrame()
		{
			FramePtr pResult;
			if(reservedFrames_.size() > currentReservedFrameIndex_) {
				pResult = reservedFrames_[currentReservedFrameIndex_];
				currentReservedFrameIndex_ = (currentReservedFrameIndex_+1) & 3;
			}
			return pResult;
		}

		void DisplayFrame(Frame* pFrame)
		{
			if(pFrame != NULL && pFrame->HasValidDataPtr()) {
				if(GetFrameManager()->Owns(*pFrame)) {
					DoRender(pFrame);
				}
				else {
					FramePtr pTempFrame = GetReservedFrame();
					if(pTempFrame && pFrame->GetDataSize() == pTempFrame->GetDataSize()) {
						utils::image::Copy(pTempFrame->GetDataPtr(), pFrame->GetDataPtr(), pTempFrame->GetDataSize());
						DoRender(pTempFrame.get());
					}
					else
						LOG << TEXT("DECKLINK: Failed to get reserved frame");
				}
			}
			else {
				LOG << TEXT("DECKLINK: Tried to render frame with no data");
			}
		}
		void DoRender(Frame* pFrame) {
			static DWORD lastTime = 0;
			static bool bDoLog = true;
			DWORD timediff = timeGetTime() - lastTime;
			if(timediff < 30) {
				Sleep(40 - timediff);
				lastTime += 40;
			}
			else
				lastTime = timeGetTime();

			if(pConsumerImpl_->pDecklinkOutput_->DisplayVideoFrameSync(reinterpret_cast<IDeckLinkMutableVideoFrame*>(pFrame->GetMetadata())) != S_OK) {
				if(bDoLog) {
					LOG << TEXT("DECKLINK: Failed to render frame");
					bDoLog = false;
				}
			}
			else {
				bDoLog = true;
			}
//			lastFrameID_ = pFrame->ID();
		}

		int totalFramesScheduled_;
		std::vector<FramePtr> reservedFrames_;
		unsigned int currentReservedFrameIndex_;
		Implementation* pConsumerImpl_;
	};
	friend struct DecklinkPlaybackStrategy;

	struct DecklinkFrameManager : public caspar::FrameManager
	{
		explicit DecklinkFrameManager(Implementation* pConsumerImpl) : pConsumerImpl_(pConsumerImpl)
		{
			pFrameManager_.reset(new SystemFrameManager(pConsumerImpl_->GetFrameFormatDescription()));
		}

		FramePtr CreateFrame() {
			return pFrameManager_->CreateFrame();
		}

		FramePtr CreateReservedFrame() {
			return FramePtr(new DecklinkVideoFrame(this));
		}

		const FrameFormatDescription& GetFrameFormatDescription() const {
			return pConsumerImpl_->GetFrameFormatDescription();
		}

		Implementation* pConsumerImpl_;
		SystemFrameManagerPtr pFrameManager_;
	};

	typedef std::tr1::shared_ptr<DecklinkFrameManager> DecklinkFrameManagerPtr;

	CComPtr<IDeckLink> pDecklink_;
	CComQIPtr<IDeckLinkOutput> pDecklinkOutput_;
	CComQIPtr<IDeckLinkKeyer> pDecklinkKeyer_;

	FramePlaybackControlPtr pPlaybackControl_;
	DecklinkFrameManagerPtr pFrameManager_;
	FrameFormat currentFormat_;

//	IDeckLinkMutableVideoFrame* pNextFrame_;

	explicit Implementation(CComPtr<IDeckLink> pDecklink) : pDecklink_(pDecklink), currentFormat_(FFormatPAL)//, pNextFrame_(NULL)
	{}

	~Implementation()
	{
		ReleaseDevice();
	}

	bool SetupDevice()
	{
		if(!pDecklink_)
			return false;

		BSTR pModelName;
		pDecklink_->GetModelName(&pModelName);
		if(pModelName != NULL)
			LOG << TEXT("DECKLINK: Modelname: ") << pModelName;

		pDecklinkOutput_ = pDecklink_;
		if(pDecklinkOutput_ == NULL) {
			LOG << TEXT("DECKLINK: Failed to get IDecklinkOutput interface");
			return false;
		}

		tstring strDesiredFrameFormat = caspar::GetApplication()->GetSetting(TEXT("videomode"));
		if(strDesiredFrameFormat.size() == 0)
			strDesiredFrameFormat = TEXT("PAL");
		FrameFormat casparVideoFormat = caspar::GetVideoFormat(strDesiredFrameFormat);
		unsigned long decklinkVideoFormat = GetDecklinkVideoFormat(casparVideoFormat);
		if(decklinkVideoFormat == ULONG_MAX) {
			LOG << "DECKLINK: Card does not support requested videoformat: " << strDesiredFrameFormat;
			return false;
		}
		
		currentFormat_ = casparVideoFormat;

		BMDDisplayModeSupport displayModeSupport;
		if(FAILED(pDecklinkOutput_->DoesSupportVideoMode((BMDDisplayMode)decklinkVideoFormat, bmdFormat8BitBGRA, &displayModeSupport))) {
			LOG << TEXT("DECKLINK: Card does not support requested videoformat");
			return false;
		}

		pDecklinkOutput_->DisableAudioOutput();
		if(FAILED(pDecklinkOutput_->EnableVideoOutput((BMDDisplayMode)decklinkVideoFormat, bmdVideoOutputFlagDefault))) {
			LOG << TEXT("DECKLINK: Could not enable video output");
			return false;
		}

		pFrameManager_.reset(new DecklinkFrameManager(this));

		if(GetApplication()->GetSetting(TEXT("internalkey")) == TEXT("true")) {
			pDecklinkKeyer_ = pDecklink_;
			if(pDecklinkKeyer_) {
				bool bSuccess = true;
				if(FAILED(pDecklinkKeyer_->Enable(FALSE))) {
					LOG << TEXT("DECKLINK: Failed to enable internal keyer");
					bSuccess = false;
				}
				if(FAILED(pDecklinkKeyer_->SetLevel(255))) {
					LOG << TEXT("DECKLINK: Keyer - Failed to set blend-level to max");
					bSuccess = false;
				}

				if(bSuccess)
					LOG << TEXT("DECKLINK: Successfully configured internal keyer");
			}
			else {
				LOG << TEXT("DECKLINK: Failed to get keyer-interface");
			}
		}

		pPlaybackControl_.reset(new FramePlaybackControl(FramePlaybackStrategyPtr(new DecklinkPlaybackStrategy(this))));
		pPlaybackControl_->Start();

		LOG << TEXT("DECKLINK: Successfully initialized decklink for ") << strDesiredFrameFormat;
		return true;
	}

	bool ReleaseDevice()
	{
		if(pPlaybackControl_)
			pPlaybackControl_->Stop();

		if(pDecklinkKeyer_) {
			pDecklinkKeyer_.Release();
		}

		if(pDecklinkOutput_) {
			BOOL bIsRunning = FALSE;
			pDecklinkOutput_->IsScheduledPlaybackRunning(&bIsRunning);
			if(bIsRunning)
				pDecklinkOutput_->StopScheduledPlayback(0, NULL, 0);

			pDecklinkOutput_->DisableVideoOutput();
		}

		return true;
	}

	//void DoScheduleNextFrame() {
	//	static int frame = 0;
	//	static bool bLog = true;
	//	if(pDecklinkOutput_->ScheduleVideoFrame(pNextFrame_, frame++, 1, 25) != S_OK) {
	//		if(bLog) {
	//			LOG << TEXT("DECKLINK: Failed to display frame");
	//			bLog = false;
	//		}
	//	}
	//	else {
	//		if(((frame-1) % 25) == 0)
	//			LOG << TEXT("DECKLINK: Scheduled frame ") << (frame-1);
	//		bLog = true;
	//	}
	//}

	// IUnknown needs o a dummy implementation
	virtual HRESULT STDMETHODCALLTYPE	QueryInterface(REFIID iid, LPVOID *ppv) 
	{
		if(ppv != NULL) {
			if(iid == IID_IUnknown) {
				(*ppv) = this;
				return S_OK;
			}
			if(iid == IID_IDeckLinkVideoOutputCallback_v7_1) {
				(*ppv) = (IDeckLinkVideoOutputCallback_v7_1*)this;
				return S_OK;
			}
			if(iid == IID_IDeckLinkVideoOutputCallback) {
				(*ppv) = (IDeckLinkVideoOutputCallback*)this;
				return S_OK;
			}
		}
		return E_NOINTERFACE;
	}
	virtual ULONG STDMETHODCALLTYPE		AddRef() {return 1;}
	virtual ULONG STDMETHODCALLTYPE		Release() {return 1;}

	virtual HRESULT STDMETHODCALLTYPE ScheduledFrameCompleted(IDeckLinkVideoFrame* completedFrame, BMDOutputFrameCompletionResult result) {
		//DoScheduleNextFrame();
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ScheduledPlaybackHasStopped() {
		return S_OK;
	}

	const FrameFormatDescription& GetFrameFormatDescription() const {
		return FrameFormatDescription::FormatDescriptions[currentFormat_];
	}

	unsigned long GetDecklinkVideoFormat(FrameFormat fmt) {
		switch(fmt)
		{
		case FFormatPAL:
			return bmdModePAL;

		case FFormatNTSC:
			return bmdModeNTSC;

		case FFormat576p2500:
			return ULONG_MAX;	//not supported

		case FFormat720p5000:
			return bmdModeHD720p50;

		case FFormat720p5994:
			return bmdModeHD720p5994;

		case FFormat720p6000:
			return bmdModeHD720p60;

		case FFormat1080p2397:
			return bmdModeHD1080p2398;

		case FFormat1080p2400:
			return bmdModeHD1080p24;

		case FFormat1080i5000:
			return bmdModeHD1080i50;

		case FFormat1080i5994:
			return bmdModeHD1080i5994;

		case FFormat1080i6000:
			return bmdModeHD1080i6000;

		case FFormat1080p2500:
			return bmdModeHD1080p25;

		case FFormat1080p2997:
			return bmdModeHD1080p2997;

		case FFormat1080p3000:
			return bmdModeHD1080p30;
		}

		return ULONG_MAX;
	}
};

DecklinkVideoConsumer::DecklinkVideoConsumer(ImplementationPtr pImpl) : pImpl_(pImpl)
{}

DecklinkVideoConsumer::~DecklinkVideoConsumer()
{}

VideoConsumerPtr DecklinkVideoConsumer::Create() {
	VideoConsumerPtr pResult;

	CComPtr<IDeckLinkIterator> pDecklinkIterator;
	HRESULT result = pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator);
	if(FAILED(result)) {
		LOG << TEXT("No Decklink drivers installed");
		return pResult;
	}

	CComPtr<IDeckLink> pDecklink;
	IDeckLink* pTempDecklink = NULL;
	while(pDecklinkIterator->Next(&pTempDecklink) == S_OK) {
		if(pDecklink == NULL)
			pDecklink = pTempDecklink;

		if(pTempDecklink)
			pTempDecklink->Release();
		pTempDecklink = NULL;
	}

	if(pDecklink == NULL) {
		LOG << TEXT("No Decklink card found");
		return pResult;
	}

	ImplementationPtr pImpl(new Implementation(pDecklink));
	pResult.reset(new DecklinkVideoConsumer(pImpl));

	if(pResult != 0 && pResult->SetupDevice(0) == false)
		pResult.reset();

	return pResult;
}

IPlaybackControl* DecklinkVideoConsumer::GetPlaybackControl() const
{
	return pImpl_->pPlaybackControl_.get();
}

bool DecklinkVideoConsumer::SetupDevice(unsigned int deviceIndex)
{
	return (pImpl_) ? pImpl_->SetupDevice() : false;
}

bool DecklinkVideoConsumer::ReleaseDevice()
{
	return (pImpl_) ? pImpl_->ReleaseDevice() : false;
}

void DecklinkVideoConsumer::EnableVideoOutput(){}
void DecklinkVideoConsumer::DisableVideoOutput(){}

const FrameFormatDescription& DecklinkVideoConsumer::GetFrameFormatDescription() const {
	return pImpl_->GetFrameFormatDescription();
}
const TCHAR* DecklinkVideoConsumer::GetFormatDescription() const {
	return pImpl_->GetFrameFormatDescription().name;
}


}	//namespace decklink
}	//namespace caspar