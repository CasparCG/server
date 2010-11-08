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
 
#include "../../stdafx.h"

#if defined(_MSC_VER)
#pragma warning (push, 1) // TODO: Legacy code, just disable warnings
#pragma warning (disable : 4244)
#endif

#include "DecklinkVideoConsumer.h"
#include "DeckLinkAPI_h.h"

#include "../../video/video_format.h"
#include "../../../common/utility/memory.h"

#include "../../producer/frame_producer_device.h"

#include <tbb/concurrent_queue.h>
#include <boost/thread.hpp>

#pragma warning(push)
#pragma warning(disable : 4996)

	#include <atlbase.h>

	#include <atlcom.h>
	#include <atlhost.h>

#pragma warning(push)

namespace caspar { namespace core { namespace decklink{

struct DecklinkVideoConsumer::Implementation : public IDeckLinkVideoOutputCallback
{
	struct DecklinkFrameManager;
	struct DecklinkVideoFrame
	{
		explicit DecklinkVideoFrame(DecklinkFrameManager* pFactory)
		{
			IDeckLinkMutableVideoFrame* frame = NULL;
			const video_format_desc& format_desc = pFactory->pConsumerImpl_->get_video_format_desc();
			if(pFactory->pConsumerImpl_->pDecklinkOutput_->CreateVideoFrame(format_desc.width, format_desc.height, format_desc.size/format_desc.height, bmdFormat8BitBGRA, bmdFrameFlagDefault, &frame) != S_OK) 
			{
				throw std::exception("DECKLINK: Failed to create frame");
			}
			pDecklinkFrame_ = frame;
			frame->Release();

			if(pDecklinkFrame_->GetBytes((void**)&pBytes_) != S_OK)
				throw std::exception("DECKLINK: Failed to get bytes to frame");
		}

		unsigned char* data() 
		{
			return pBytes_;
		}
		
		size_t width() const { return 0; }
		size_t height() const { return 0; }

		bool valid() const 
		{
			return true;
		}

		unsigned int size() const 
		{
			return pDecklinkFrame_->GetRowBytes() * pDecklinkFrame_->GetHeight();
		}

		IDeckLinkMutableVideoFrame*  meta_data() const 
		{
			return pDecklinkFrame_;
		}

		void* tag() const
		{
			return pFactory_;
		}

		DecklinkFrameManager* pFactory_;
		CComPtr<IDeckLinkMutableVideoFrame> pDecklinkFrame_;
		unsigned char* pBytes_;
	};

	struct DecklinkPlaybackStrategy
	{
		explicit DecklinkPlaybackStrategy(Implementation* pConsumerImpl) : pConsumerImpl_(pConsumerImpl), currentReservedFrameIndex_(0), totalFramesScheduled_(0)
		{
			for(int i = 0; i<4; ++i) 
			{
				reservedFrames_.push_back(pConsumerImpl_->pFrameManager_->CreateReservedFrame());
			}
		}
		
		std::shared_ptr<DecklinkVideoFrame> GetReservedFrame()
		{
			std::shared_ptr<DecklinkVideoFrame> pResult;
			if(reservedFrames_.size() > currentReservedFrameIndex_)
			{
				pResult = reservedFrames_[currentReservedFrameIndex_];
				currentReservedFrameIndex_ = (currentReservedFrameIndex_+1) & 3;
			}
			return pResult;
		}

		void DisplayFrame(const frame_ptr& frame)
		{
			if(frame != NULL) 
			{
				std::shared_ptr<DecklinkVideoFrame> pTempFrame = GetReservedFrame();
				if(pTempFrame && frame->size() == pTempFrame->size())
				{
					common::aligned_parallel_memcpy(pTempFrame->data(), frame->data(), pTempFrame->size());
					DoRender(pTempFrame);
				}
				else
					CASPAR_LOG(error) << "DECKLINK: Failed to get reserved frame";				
			}
			else 
			{
				CASPAR_LOG(error) << "DECKLINK: Tried to render frame with no data";
			}
		}
		void DoRender(const std::shared_ptr<DecklinkVideoFrame>& frame)
		{
			static DWORD lastTime = 0;
			static bool bDoLog = true;

			if(pConsumerImpl_->pDecklinkOutput_->DisplayVideoFrameSync(reinterpret_cast<IDeckLinkMutableVideoFrame*>(frame->meta_data())) != S_OK) 
			{
				if(bDoLog)
				{
					CASPAR_LOG(error) << "DECKLINK: Failed to render frame";
					bDoLog = false;
				}
			}
			else 
			{
				bDoLog = true;
			}
//			lastFrameID_ = frame->ID();
		}

		int totalFramesScheduled_;
		std::vector<std::shared_ptr<DecklinkVideoFrame> > reservedFrames_;
		unsigned int currentReservedFrameIndex_;
		Implementation* pConsumerImpl_;
	};
	friend struct DecklinkPlaybackStrategy;

	struct DecklinkFrameManager
	{
		explicit DecklinkFrameManager(Implementation* pConsumerImpl) : pConsumerImpl_(pConsumerImpl)
		{
		}
		
		std::shared_ptr<DecklinkVideoFrame>  CreateReservedFrame() {
			return std::make_shared<DecklinkVideoFrame>(this);
		}

		const video_format_desc& get_video_format_desc() const {
			return pConsumerImpl_->get_video_format_desc();
		}

		Implementation* pConsumerImpl_;
	};

	typedef std::tr1::shared_ptr<DecklinkFrameManager> DecklinkFrameManagerPtr;

	bool internalKey_;
	CComPtr<IDeckLink> pDecklink_;
	CComQIPtr<IDeckLinkOutput> pDecklinkOutput_;
	CComQIPtr<IDeckLinkKeyer> pDecklinkKeyer_;

	std::shared_ptr<DecklinkPlaybackStrategy> pPlayback_;
	DecklinkFrameManagerPtr pFrameManager_;
	video_format::type currentFormat_;
	video_format_desc format_desc_;

	std::exception_ptr pException_;
	boost::thread thread_;
	tbb::concurrent_bounded_queue<frame_ptr> frameBuffer_;

//	IDeckLinkMutableVideoFrame* pNextFrame_;

	explicit Implementation(const video_format_desc& format_desc, bool internalKey) 
		: format_desc_(format_desc), currentFormat_(video_format::pal), internalKey_(internalKey)
	{
	
		CComPtr<IDeckLinkIterator> pDecklinkIterator;
		HRESULT result = pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator);
		if(FAILED(result))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("No Decklink drivers installed"));

		CComPtr<IDeckLink> pDecklink;
		IDeckLink* pTempDecklink = NULL;
		while(pDecklinkIterator->Next(&pTempDecklink) == S_OK) 
		{
			if(pDecklink == NULL)
				pDecklink = pTempDecklink;

			if(pTempDecklink)
				pTempDecklink->Release();
			pTempDecklink = NULL;
		}

		if(pDecklink == nullptr)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("No Decklink card found"));

		if(!SetupDevice())
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to initialize decklink consumer."));
		
		pDecklink_ = pDecklink;

		frameBuffer_.set_capacity(1),
		thread_ = boost::thread([=]{Run();});
	}

	~Implementation()
	{
		frameBuffer_.push(nullptr);
		thread_.join();
		ReleaseDevice();
	}
	
	void DisplayFrame(const frame_ptr& frame)
	{
		if(frame == nullptr)
			return;		

		if(pException_ != nullptr)
			std::rethrow_exception(pException_);

		frameBuffer_.push(frame);
	}

	void Run()
	{				
		while(true)
		{
			try
			{	
				frame_ptr frame;
				frameBuffer_.pop(frame);
				if(frame == nullptr)
					return;
				
				pPlayback_->DisplayFrame(frame);
			}
			catch(...)
			{
				pException_ = std::current_exception();
			}
		}
	}

	bool SetupDevice()
	{
		if(!pDecklink_)
			return false;

		BSTR pModelName;
		pDecklink_->GetModelName(&pModelName);
		if(pModelName != NULL)
			CASPAR_LOG(info) << "DECKLINK: Modelname: " << pModelName;

		pDecklinkOutput_ = pDecklink_;
		if(pDecklinkOutput_ == NULL) {
			CASPAR_LOG(info) << "DECKLINK: Failed to get IDecklinkOutput interface";
			return false;
		}

		unsigned long decklinkVideoFormat = GetDecklinkVideoFormat(format_desc_.format);
		if(decklinkVideoFormat == ULONG_MAX) {
			CASPAR_LOG(error) << "DECKLINK: Card does not support requested videoformat: " << format_desc_.name;
			return false;
		}
		
		currentFormat_ = format_desc_.format;

		BMDDisplayModeSupport displayModeSupport;
		if(FAILED(pDecklinkOutput_->DoesSupportVideoMode((BMDDisplayMode)decklinkVideoFormat, bmdFormat8BitBGRA, &displayModeSupport))) {
			CASPAR_LOG(error) << "DECKLINK: Card does not support requested videoformat";
			return false;
		}

		pDecklinkOutput_->DisableAudioOutput();
		if(FAILED(pDecklinkOutput_->EnableVideoOutput((BMDDisplayMode)decklinkVideoFormat, bmdVideoOutputFlagDefault))) {
			CASPAR_LOG(error) << "DECKLINK: Could not enable video output";
			return false;
		}

		pFrameManager_.reset(new DecklinkFrameManager(this));

		if(internalKey_) {
			pDecklinkKeyer_ = pDecklink_;
			if(pDecklinkKeyer_) {
				bool bSuccess = true;
				if(FAILED(pDecklinkKeyer_->Enable(FALSE))) {
					CASPAR_LOG(error) << "DECKLINK: Failed to enable internal keyer";
					bSuccess = false;
				}
				if(FAILED(pDecklinkKeyer_->SetLevel(255))) {
					CASPAR_LOG(error) << "DECKLINK: Keyer - Failed to set blend-level to max";
					bSuccess = false;
				}

				if(bSuccess)
					CASPAR_LOG(info) << "DECKLINK: Successfully configured internal keyer";
			}
			else {
				CASPAR_LOG(error) << "DECKLINK: Failed to get keyer-interface";
			}
		}

		pPlayback_ = std::make_shared<DecklinkPlaybackStrategy>(this);

		CASPAR_LOG(info) << "DECKLINK: Successfully initialized decklink for " << format_desc_.name;
		return true;
	}

	bool ReleaseDevice()
	{
		pPlayback_.reset();

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

	virtual HRESULT STDMETHODCALLTYPE ScheduledPlaybackHasStopped() 
	{
		return S_OK;
	}

	const video_format_desc& get_video_format_desc() const 
	{
		return video_format_desc::format_descs[currentFormat_];
	}

	unsigned long GetDecklinkVideoFormat(video_format::type fmt) 
	{
		switch(fmt)
		{
		case video_format::pal:			return bmdModePAL;
		case video_format::ntsc:		return bmdModeNTSC;
		case video_format::x576p2500:	return ULONG_MAX;	//not supported
		case video_format::x720p5000:	return bmdModeHD720p50;
		case video_format::x720p5994:	return bmdModeHD720p5994;
		case video_format::x720p6000:	return bmdModeHD720p60;
		case video_format::x1080p2397:	return bmdModeHD1080p2398;
		case video_format::x1080p2400:	return bmdModeHD1080p24;
		case video_format::x1080i5000:	return bmdModeHD1080i50;
		case video_format::x1080i5994:	return bmdModeHD1080i5994;
		case video_format::x1080i6000:	return bmdModeHD1080i6000;
		case video_format::x1080p2500:	return bmdModeHD1080p25;
		case video_format::x1080p2997:	return bmdModeHD1080p2997;
		case video_format::x1080p3000:	return bmdModeHD1080p30;
		default:						return ULONG_MAX;
		}
	}
};

DecklinkVideoConsumer::DecklinkVideoConsumer(const video_format_desc& format_desc, bool internalKey) : pImpl_(new Implementation(format_desc, internalKey))
{}

void DecklinkVideoConsumer::display(const frame_ptr& frame)
{
	pImpl_->DisplayFrame(frame);
}

const video_format_desc& DecklinkVideoConsumer::get_video_format_desc() const
{
	return pImpl_->format_desc_;
}
	
}	//namespace decklink
}}	//namespace caspar