#include "..\..\stdafx.h"

#include <math.h>
#include <algorithm>
#include <functional>

#include "TransitionProducer.h"
#include "..\..\application.h"
#include "..\..\utils\pixmapdata.h"
#include "..\..\utils\image\image.hpp"
#include "..\..\utils\thread.h"
#include "..\..\frame\clipinfo.h"
#include "..\..\transitioninfo.h"
#include "..\..\utils\taskqueue.h"

namespace caspar
{

using namespace utils;

struct TransitionProducer::Implementation
{
	TransitionProducer*		self_;

	FrameManagerPtr			pResultFrameManager_;
	SystemFrameManagerPtr	pIntermediateFrameManager_;
	ClipInfo				sourceClip_;
	ClipInfo				destinationClip_;
	std::vector<HANDLE>		readWaitHandles_;

	unsigned short			totalFrames_, currentFrame_;

	std::tr1::function<void (Implementation*, unsigned char*, unsigned char*, unsigned char*)> generateFrameFun_;

	TransitionInfo			transitionInfo_;
	unsigned long			borderColorValue_;
	utils::PixmapDataPtr	pBorderImage_;

	utils::Thread worker_;
	utils::TaskQueue taskQueue_;
	MotionFrameBuffer frameBuffer_;

	Implementation(TransitionProducer*	self, MediaProducerPtr pDest, const TransitionInfo& transitionInfo, const FrameFormatDescription& fmtDesc)
	:	
	self_(self), 
	totalFrames_(transitionInfo.duration_+1), 
	currentFrame_(1),
	transitionInfo_(transitionInfo), 
	borderColorValue_(0)
	{
		pIntermediateFrameManager_.reset(new SystemFrameManager(fmtDesc));

		if(pDest != 0) {
			FrameMediaController* pDestFrameController = dynamic_cast<FrameMediaController*>(pDest->QueryController(TEXT("FrameController")));
			destinationClip_ = ClipInfo(pDest, pDestFrameController);
			if(!destinationClip_.IsEmpty())
				readWaitHandles_.push_back(destinationClip_.pFrameController_->GetFrameBuffer().GetWaitHandle());
		}

		switch(transitionInfo_.type_) {
		case Slide:
		case Push:
		case Wipe:
			generateFrameFun_ = &Implementation::GenerateWipeFrame;
			break;
		case Mix:
		default:
			generateFrameFun_ = &Implementation::GenerateMixFrame;
			break;
		};
	}

	~Implementation() 
	{
		worker_.Stop();
	}


	IMediaController* QueryController(const tstring& id) 
	{
		//The transition only supports FrameController if its destination also supports it
		if(!destinationClip_.IsEmpty()) {
			if(id == TEXT("FrameController")) {
				return self_;
			}
		}
		return NULL;
	}

	MediaProducerPtr GetFollowingProducer() 
	{
		return destinationClip_.pFP_;
	}

	bool Initialize(FrameManagerPtr pFrameManager)
	{
		if(pFrameManager) {
			if(!worker_.IsRunning()) {
				pResultFrameManager_ = pFrameManager;
				if(pResultFrameManager_ != 0 && !destinationClip_.IsEmpty()) {
					//Cue audio for destination clip
					GetApplication()->GetAudioManager()->CueAudio(destinationClip_.pFrameController_);

					return destinationClip_.pFrameController_->Initialize(pIntermediateFrameManager_);
				}
			}
			else {
				//Create a task that replace pResultFrameManager_ but is executed in the worker thread
				taskQueue_.push_back(bind(&TransitionProducer::Implementation::DoUpdateFrameManager, this, pFrameManager));
				return true;
			}
		}
		return false;
	}

	void DoUpdateFrameManager(FrameManagerPtr pFrameManager) {
		pResultFrameManager_ = pFrameManager;
	}

	FrameBuffer& GetFrameBuffer() 
	{
		return frameBuffer_;
	}

	bool Start(const ClipInfo& srcClipInfo) 
	{
		sourceClip_ = srcClipInfo;
		if(!sourceClip_.IsEmpty() && !destinationClip_.IsEmpty()) {
			if(sourceClip_.pFrameController_->Initialize(pIntermediateFrameManager_)) {
				readWaitHandles_.push_back(sourceClip_.pFrameController_->GetFrameBuffer().GetWaitHandle());

				//AUDIO
				{
					//copy all workers from the source to this
					self_->GetSoundBufferWorkers().insert(self_->GetSoundBufferWorkers().end(), sourceClip_.pFrameController_->GetSoundBufferWorkers().begin(), sourceClip_.pFrameController_->GetSoundBufferWorkers().end());

					//copy all workers from the destination to this
					self_->GetSoundBufferWorkers().insert(self_->GetSoundBufferWorkers().end(), destinationClip_.pFrameController_->GetSoundBufferWorkers().begin(), destinationClip_.pFrameController_->GetSoundBufferWorkers().end());
				}

				return worker_.Start(self_);
			}
		}
		return false;
	}

	void Run(HANDLE stopEvent)
	{
		LOG << LogLevel::Verbose << TEXT("Transition: readAhead thread started");
		currentFrame_ = 1;

		HANDLE waitHandles[3] = { stopEvent, taskQueue_.GetWaitEvent(), frameBuffer_.GetWriteWaitHandle() };
		DWORD waitHandlesCount = sizeof(waitHandles) / sizeof(HANDLE);

		bool bQuit = false;
		while(bQuit == false && currentFrame_ <= totalFrames_) {
			HRESULT waitResult = WaitForMultipleObjects(waitHandlesCount, waitHandles, FALSE, 2000);
			switch(waitResult) {
				case WAIT_OBJECT_0:
					LOG << LogLevel::Debug << TEXT("Transition: Recieved stopEvent");
					bQuit = true;
					break;

				case WAIT_OBJECT_0+1:
					taskQueue_.pop_and_execute_front();
					break;

				case WAIT_OBJECT_0+2:
					{
						bool bWriteSuccess = false;
						if(WriteFrame(bWriteSuccess)) {
							if(bWriteSuccess)
								++currentFrame_;
						}
						else {
							LOG << TEXT("Transition: WriteFrame returned false. Abort transition");
							bQuit = true;
						}
					}
					break;

				case WAIT_TIMEOUT:
					break;

				default:
					LOG << LogLevel::Critical << TEXT("Transition: write-wait failed. Aborting");
					bQuit = true;
					break;
			}
		}

		//ALMOST SAME BUT OLD: This does not include a complete frame from the destination as the last frame
/*		for(currentFrame_ = 1; currentFrame_< totalFrames_; ++currentFrame_) {
			HRESULT waitResult = WaitForMultipleObjects(waitHandlesCount, waitHandles, FALSE, 2000);
			if(waitResult == (WAIT_OBJECT_0+2)) {
				if(!WriteFrame()) {
					LOG << LogLevel::Debug << TEXT("Transition: WriteFrame returned false. Abort transition");
					break;
				}
			}
			else if(waitResult == WAIT_OBJECT_0 + 1) {
				--currentFrame_;

				Task task;
				taskQueue_.pop_front(task);
				if(task)
					task();
			}
			else if(waitResult == WAIT_OBJECT_0) {
				LOG << LogLevel::Debug << TEXT("Transition: Recieved stopEvent");
				break;
			}
			else if(waitResult == WAIT_TIMEOUT) {
				//retry the same frame
				--currentFrame_;
			}
			else {
				//Iiik, Critical error
				LOG << LogLevel::Critical << TEXT("Transition: write-wait failed. Aborting");
				break;
			}
		}*/

		FramePtr pNullFrame;
		frameBuffer_.push_back(pNullFrame);
		LOG << LogLevel::Verbose << TEXT("Transition: readAhead thread ended");
	}

	bool OnUnhandledException(const std::exception& ex) throw() 
	{
		try 
		{
			FramePtr pNullFrame;
			frameBuffer_.push_back(pNullFrame);

			LOG << LogLevel::Critical << TEXT("UNHANDLED EXCEPTION in transitionthread. Message: ") << ex.what();
		}
		catch(...)
		{}

		return false;
	}

	bool WriteFrame(bool &bWriteSuccess) 
	{
		//Wait for source-feeds
		HRESULT waitResult = WAIT_OBJECT_0;
		if(readWaitHandles_.size() > 0)
			waitResult = WaitForMultipleObjects(static_cast<DWORD>(readWaitHandles_.size()), &(readWaitHandles_[0]), TRUE, 250);

		switch(waitResult) 
		{
			case WAIT_OBJECT_0:			
				if(DoWriteFrame())
					bWriteSuccess = true;
				else
					return false;
				break;

			case WAIT_TIMEOUT:
				Sleep(0);
				break;

			default:	//An error occured
				LOG << LogLevel::Critical << TEXT("Transition: read-wait failed. Aborting");
				return false;
		}

		return true;
	}

	bool DoWriteFrame()
	{
		FrameBuffer& srcFrameBuffer = sourceClip_.pFrameController_->GetFrameBuffer();
		FrameBuffer& destFrameBuffer = destinationClip_.pFrameController_->GetFrameBuffer();

		//Make member of ClipInfo
		FramePtr pSrcFrame =  (sourceClip_.lastFetchResult_ == FetchEOF) ? sourceClip_.pLastFrame_ : srcFrameBuffer.front();
		FramePtr pDestFrame = (destinationClip_.lastFetchResult_ == FetchEOF)? destinationClip_.pLastFrame_ : destFrameBuffer.front();

		//Bail if no data is availible
		if(pSrcFrame == 0 || pDestFrame == 0) 
		{
			LOG << LogLevel::Debug << TEXT("Transition: WriteFrame(): GetFrameBuffer().front() returned null");
			return false;
		}

		FramePtr pResultFrame = pResultFrameManager_->CreateFrame();
		if(pResultFrame != 0 && pResultFrame->GetDataPtr() != 0) 
		{
			generateFrameFun_(this, pResultFrame->GetDataPtr(), pSrcFrame->GetDataPtr(), pDestFrame->GetDataPtr());
			//AUDIO
			{
				//copy all sounddatachunks from source
				pResultFrame->GetAudioData().insert(pResultFrame->GetAudioData().end(), pSrcFrame->GetAudioData().begin(), pSrcFrame->GetAudioData().end());

				//copy sounddatachunk from destination
				pResultFrame->GetAudioData().insert(pResultFrame->GetAudioData().end(), pDestFrame->GetAudioData().begin(), pDestFrame->GetAudioData().end());
			}

			frameBuffer_.push_back(pResultFrame);

			if(sourceClip_.lastFetchResult_ != FetchEOF) 
			{
				sourceClip_.pLastFrame_			= pSrcFrame;
				sourceClip_.lastFetchResult_	= srcFrameBuffer.pop_front();

				//remove from readWaitHandles
				if(sourceClip_.lastFetchResult_ == FetchEOF) 							
					readWaitHandles_.erase(std::find(readWaitHandles_.begin(), readWaitHandles_.end(), srcFrameBuffer.GetWaitHandle()));		
			}

			if(destinationClip_.lastFetchResult_ != FetchEOF)
			{
				destinationClip_.pLastFrame_		= pDestFrame;
				destinationClip_.lastFetchResult_	= destFrameBuffer.pop_front();

				//remove from readWaitHandles
				if(destinationClip_.lastFetchResult_ == FetchEOF) 							
					readWaitHandles_.erase(std::find(readWaitHandles_.begin(), readWaitHandles_.end(), destFrameBuffer.GetWaitHandle()));									
			}
		}
		else 
		{
			Sleep(0);
		}		

		return true;
	}

	/////////////////////////////
	// Frame-generating functions
	void GenerateMixFrame(unsigned char* pResultData, unsigned char* pSourceData, unsigned char* pDestData)
	{
		image::Lerp(pResultData, pSourceData, pDestData, 1.0f-static_cast<float>(currentFrame_)/static_cast<float>(totalFrames_), pResultFrameManager_->GetFrameFormatDescription().size);
	}

	// TODO: Move into "image" library, seperate push, slide, wipe? (R.N)
	void GenerateWipeFrame(unsigned char* pResultData, unsigned char* pSourceData, unsigned char* pDestData)
	{
		const FrameFormatDescription& fmtDesc = pResultFrameManager_->GetFrameFormatDescription();

		if(currentFrame_ < totalFrames_) {
			int totalWidth = fmtDesc.width + transitionInfo_.borderWidth_;
			
			float fStep   = totalWidth / (float)totalFrames_;
			float fOffset = fStep * (float)currentFrame_;

			int halfStep = static_cast<int>(fStep/2.0);
			int offset   = static_cast<int>(fOffset+0.5f);
			
			//read source to buffer
			for(int row = 0, even = 0; row < fmtDesc.height; ++row, even ^= 1)
			{
				int fieldCorrectedOffset = offset + (halfStep*even);
				if(fieldCorrectedOffset < fmtDesc.width)
				{
					if(transitionInfo_.direction_ != FromLeft)
					{
						if(transitionInfo_.type_ == Push)
							memcpy(&(pResultData[4*row*fmtDesc.width]), &(pSourceData[4*(row*fmtDesc.width+fieldCorrectedOffset)]), (fmtDesc.width-fieldCorrectedOffset)*4);
						else	//Slide | Wipe
							memcpy(&(pResultData[4*row*fmtDesc.width]), &(pSourceData[4*row*fmtDesc.width]), (fmtDesc.width-fieldCorrectedOffset)*4);
					}
					else // if (direction == LEFT)
					{				
						if(transitionInfo_.type_ == Push)
							memcpy(&(pResultData[4*(row*fmtDesc.width+fieldCorrectedOffset)]), &(pSourceData[4*(row*fmtDesc.width)]), (fmtDesc.width-fieldCorrectedOffset)*4);
						else	//slide eller wipe
							memcpy(&(pResultData[4*(row*fmtDesc.width+fieldCorrectedOffset)]), &(pSourceData[4*(row*fmtDesc.width+fieldCorrectedOffset)]), (fmtDesc.width-fieldCorrectedOffset)*4);
					}
				}
			}

			//write border to buffer
			if(transitionInfo_.borderWidth_ > 0)
			{
				for(int row = 0, even = 0; row < fmtDesc.height; ++row, even ^= 1)
				{
					int fieldCorrectedOffset = offset + (halfStep*even);
					int length = transitionInfo_.borderWidth_;
					int start = 0;

					if(transitionInfo_.direction_ != FromLeft)
					{
						if(fieldCorrectedOffset > fmtDesc.width)
						{
							length -= fieldCorrectedOffset-fmtDesc.width;
							start += fieldCorrectedOffset-fmtDesc.width;
							fieldCorrectedOffset = fmtDesc.width;
						}
						else if(fieldCorrectedOffset < length)
						{
							length = fieldCorrectedOffset;
						}

						if(pBorderImage_ != 0)
						{
							unsigned char* pBorderImageData = pBorderImage_->GetDataPtr();
							memcpy(&(pResultData[4*(row*fmtDesc.width+fmtDesc.width-fieldCorrectedOffset)]), &(pBorderImageData[4*(row*pBorderImage_->width+start)]), length*4);
						}
						else
						{
							for(int i=0;i<length;++i)
								memcpy(&(pResultData[4*(row*fmtDesc.width+fmtDesc.width-fieldCorrectedOffset+i)]), &borderColorValue_, 4);
						}
					}
					else // if (direction == LEFT)
					{
						if(fieldCorrectedOffset > fmtDesc.width)
						{
							length -= fieldCorrectedOffset-fmtDesc.width;
							start = 0;
							fieldCorrectedOffset -= transitionInfo_.borderWidth_-length;
						}
						else if(fieldCorrectedOffset < length)
						{
							length = fieldCorrectedOffset;
							start = transitionInfo_.borderWidth_-fieldCorrectedOffset;
						}

						if(pBorderImage_ != 0 && length > 0)
						{
							unsigned char* pBorderImageData = pBorderImage_->GetDataPtr();
							memcpy(&(pResultData[4*(row*fmtDesc.width+fieldCorrectedOffset-length)]), &(pBorderImageData[4*(row*pBorderImage_->width+start)]), length*4);
						}
						else
						{
							for(int i=0;i<length;++i)
								memcpy(&(pResultData[4*(row*fmtDesc.width+fieldCorrectedOffset-length+i)]), &borderColorValue_, 4);
						}
					}

				}
			}

			//read dest to buffer
			offset -= transitionInfo_.borderWidth_;
			if(offset > 0)
			{
				for(int row = 0, even = 0; row < fmtDesc.height; ++row, even ^= 1)
				{
					int fieldCorrectedOffset = offset + (halfStep*even);

					if(transitionInfo_.direction_ != FromLeft)
					{
						if(transitionInfo_.type_ == Wipe)
							memcpy(&(pResultData[4*(row*fmtDesc.width+fmtDesc.width-fieldCorrectedOffset)]), &(pDestData[4*(row*fmtDesc.width+fmtDesc.width-fieldCorrectedOffset)]), fieldCorrectedOffset*4);
						else
							memcpy(&(pResultData[4*(row*fmtDesc.width+fmtDesc.width-fieldCorrectedOffset)]), &(pDestData[4*row*fmtDesc.width]), fieldCorrectedOffset*4);
					}
					else // if (direction == LEFT)
					{				
						if(transitionInfo_.type_ == Wipe)
							memcpy(&(pResultData[4*(row*fmtDesc.width)]), &(pDestData[4*(row*fmtDesc.width)]), fieldCorrectedOffset*4);
						else
							memcpy(&(pResultData[4*(row*fmtDesc.width)]), &(pDestData[4*(row*fmtDesc.width+fmtDesc.width-fieldCorrectedOffset)]), fieldCorrectedOffset*4);	
					}
				}
			}
		}
		else {
			//currentFrame_ == totalFrames_
			image::Copy(pResultData, pDestData, fmtDesc.size);
		}
	}
};


TransitionProducer::TransitionProducer(MediaProducerPtr pDest, const TransitionInfo& transitionInfo, const FrameFormatDescription& fmtDesc) : pImpl_(new Implementation(this, pDest, transitionInfo, fmtDesc)){}
TransitionProducer::~TransitionProducer() {}

IMediaController* TransitionProducer::QueryController(const tstring& id)
{
	return pImpl_->QueryController(id);
}

MediaProducerPtr TransitionProducer::GetFollowingProducer() 
{
	return pImpl_->GetFollowingProducer();
}

bool TransitionProducer::Initialize(FrameManagerPtr pFrameManager) 
{
	return pImpl_->Initialize(pFrameManager);
}

FrameBuffer& TransitionProducer::GetFrameBuffer() 
{
	return pImpl_->GetFrameBuffer();
}

bool TransitionProducer::Start(const ClipInfo& srcClipInfo) 
{
	return pImpl_->Start(srcClipInfo);
}

void TransitionProducer::Run(HANDLE stopEvent)
{
	pImpl_->Run(stopEvent);
}

bool TransitionProducer::OnUnhandledException(const std::exception& ex) throw()
{
	return pImpl_->OnUnhandledException(ex);
}

}	//namespace caspar