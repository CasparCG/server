#include "..\..\StdAfx.h"

#include <BlueVelvet.h>
#include "..\..\application.h"
#include "BlueFishVideoConsumer.h"
#include "..\..\frame\FramePlaybackControl.h"
#include "BluefishPlaybackStrategy.h"

#include <stdlib.h>
#include <stdio.h>

namespace caspar {
namespace bluefish {

///////////////////////////////////////////
// BlueFishVideoConsumer::EnumerateDevices
// RETURNS: Number of identified bluefish-cards
int BlueFishVideoConsumer::EnumerateDevices()
{
	BlueVelvetPtr pSDK(BlueVelvetFactory());

	if(pSDK != 0) {
		int deviceCount = 0;
		pSDK->device_enumerate(deviceCount);
		return deviceCount;
	}
	else
		return 0;
}


///////////////////////////////////////////
// BlueFishVideoConsumer::Create
// PARAMS: deviceIndex(index of the card that is to be wrapped in a consumer)
// RETURNS: a new BlueFishVideoConsumer-object for the specified card
// COMMENT: Creates and initializes a consumer that outputs video to a bluefish-card
VideoConsumerPtr BlueFishVideoConsumer::Create(unsigned int deviceIndex)
{
	BlueFishVideoConsumerPtr card(new BlueFishVideoConsumer());
	if(card != 0 && card->SetupDevice(deviceIndex) == false)
		card.reset();

	return card;
}

////////////////////////////////////////
// BlueFishVideoConsumer constructor
BlueFishVideoConsumer::BlueFishVideoConsumer() : pSDK_(BlueVelvetFactory()), currentFormat_(FFormatPAL), _deviceIndex(0)
{}

////////////////////////////////////////
// BlueFishVideoConsumer destructor
BlueFishVideoConsumer::~BlueFishVideoConsumer()
{
	ReleaseDevice();
}

/*******************/
/**    METHODS    **/
/*******************/

IPlaybackControl* BlueFishVideoConsumer::GetPlaybackControl() const
{
	return pPlaybackControl_.get();
}

unsigned long BlueFishVideoConsumer::GetVideoFormat(const tstring& strVideoMode) {
	currentFormat_ = FFormatPAL;
	int index = FFormatPAL;
	for(; index < FrameFormatCount; ++index)
	{
		const FrameFormatDescription& fmtDesc = FrameFormatDescription::FormatDescriptions[index];
		if(strVideoMode == fmtDesc.name) {
			currentFormat_ = (FrameFormat)index;
			break;
		}
	}

	return VidFmtFromFrameFormat(currentFormat_);
}

unsigned long BlueFishVideoConsumer::VidFmtFromFrameFormat(FrameFormat fmt) {
	switch(fmt)
	{
	case FFormatPAL:
		return VID_FMT_PAL;

	case FFormatNTSC:
		return VID_FMT_NTSC;

	case FFormat576p2500:
		return ULONG_MAX;	//not supported

	case FFormat720p5000:
		return VID_FMT_720P_5000;

	case FFormat720p5994:
		return VID_FMT_720P_5994;

	case FFormat720p6000:
		return VID_FMT_720P_6000;

	case FFormat1080p2397:
		return VID_FMT_1080P_2397;

	case FFormat1080p2400:
		return VID_FMT_1080P_2400;

	case FFormat1080i5000:
		return VID_FMT_1080I_5000;

	case FFormat1080i5994:
		return VID_FMT_1080I_5994;

	case FFormat1080i6000:
		return VID_FMT_1080I_6000;

	case FFormat1080p2500:
		return VID_FMT_1080P_2500;

	case FFormat1080p2997:
		return VID_FMT_1080P_2997;

	case FFormat1080p3000:
		return VID_FMT_1080P_3000;
	}

	return ULONG_MAX;
}

bool BlueFishVideoConsumer::SetupDevice(unsigned int deviceIndex)
{
	_deviceIndex = deviceIndex;

	unsigned long memFmt = MEM_FMT_ARGB_PC, updFmt = UPD_FMT_FRAME, vidFmt = VID_FMT_PAL, resFmt = RES_FMT_NORMAL;
	unsigned long desiredVideoFormat = VID_FMT_PAL;
	int iDummy;

	int bufferIndex=0;	//Bufferindex used when initializing the buffers

	tstring strDesiredFrameFormat = caspar::GetApplication()->GetSetting(TEXT("videomode"));
	if(strDesiredFrameFormat.size() == 0)
		strDesiredFrameFormat = TEXT("PAL");

	desiredVideoFormat = GetVideoFormat(strDesiredFrameFormat);
	if(desiredVideoFormat == ULONG_MAX) {
		LOG << TEXT("BLUECARD ERROR: Unsupported videomode: ") << strDesiredFrameFormat << TEXT(". (device") << _deviceIndex << TEXT(")");
		return false;
	}

	if(BLUE_FAIL(pSDK_->device_attach(_deviceIndex, FALSE))) {
		LOG << TEXT("BLUECARD ERROR: Failed to attach device") << _deviceIndex;
		return false;
	}

	if(desiredVideoFormat != VID_FMT_PAL) {
		int videoModeCount = pSDK_->count_video_mode();
		for(int videoModeIndex=1; videoModeIndex <= videoModeCount; ++videoModeIndex) {
			EVideoMode videoMode = pSDK_->enum_video_mode(videoModeIndex);
			if(videoMode == desiredVideoFormat) {
				vidFmt = videoMode;
			}
		}
	}

	if(vidFmt == VID_FMT_PAL) {
		strDesiredFrameFormat = TEXT("PAL");
		currentFormat_ = FFormatPAL;
	}

	if(BLUE_FAIL(pSDK_->set_video_framestore_style(vidFmt, memFmt, updFmt, resFmt))) {
		LOG << TEXT("BLUECARD ERROR: Failed to set videomode to ") << strDesiredFrameFormat << TEXT(". (device ") << _deviceIndex << TEXT(")");
		return false;
	}

	LOG << TEXT("BLUECARD INFO: Successfully configured bluecard for ") << strDesiredFrameFormat << TEXT(". (device ") << _deviceIndex << TEXT(")");

	if (pSDK_->has_output_key()) {
		iDummy = TRUE;
		int v4444 = FALSE, invert = FALSE, white = FALSE;
		pSDK_->set_output_key(iDummy, v4444, invert, white);
	}

	if(pSDK_->GetHDCardType(_deviceIndex) != CRD_HD_INVALID) {
		pSDK_->Set_DownConverterSignalType((vidFmt == VID_FMT_PAL) ? SD_SDI : HD_SDI);
	}


	iDummy = FALSE;
	pSDK_->set_vertical_flip(iDummy);

	// Get framestore parameters
	if(BLUE_OK(pSDK_->render_buffer_sizeof(m_bufferCount, m_length, m_actual, m_golden))) {
		LOG << TEXT("BLUECARD INFO: Buffers: ") << m_bufferCount << TEXT(", \"Length\": ") << m_length << TEXT(", Buffer size: ") << m_actual << TEXT(" (device ") << _deviceIndex << TEXT(")") << utils::LogStream::Flush;
	}
	else {
		LOG << TEXT("BLUECARD ERROR: Failed to get framestore parameters (device ") << _deviceIndex << TEXT(")");
	}

	pFrameManager_ = BluefishFrameManagerPtr(new BluefishFrameManager(pSDK_, currentFormat_, m_golden));

	iDummy = TRUE;
	pSDK_->set_output_video(iDummy);

	// Now specify video output buffer
	pSDK_->render_buffer_update(0);

	pPlaybackControl_.reset(new FramePlaybackControl(FramePlaybackStrategyPtr(new BluefishPlaybackStrategy(this))));
	pPlaybackControl_->Start();

	LOG << TEXT("BLUECARD INFO: Successfully initialized device ") << _deviceIndex;
	return true;
}

bool BlueFishVideoConsumer::ReleaseDevice()
{
	pPlaybackControl_.reset();

	pFrameManager_.reset();

	if(pSDK_) {
		int iDummy = FALSE;
		pSDK_->set_output_video(iDummy);

		pSDK_->device_detach();
	}

	LOG << TEXT("BLUECARD INFO: Successfully released device ") << _deviceIndex << utils::LogStream::Flush;
	return true;
}

void BlueFishVideoConsumer::EnableVideoOutput()
{
	//Need sync. protection?
	if(pSDK_)
	{
		int iDummy = TRUE;
		pSDK_->set_output_video(iDummy);
	}
}

void BlueFishVideoConsumer::DisableVideoOutput()
{
	//Need sync. protection?
	if(pSDK_)
	{
		int iDummy = FALSE;
		pSDK_->set_output_video(iDummy);
	}
}

/*
bool BlueFishVideoConsumer::UploadBuffer(FramePtr pSource, bool bContinuePlayback, bool bForceOutput)
{
	bool bReturnValue = false;

    if (pSDK_ && pSource->GetDataSize()<=m_golden && pSource != 0)
    {
		utils::BufferInfo* pBufferInfo = bufferPool_.GetNextFreeBuffer(TIMEOUT);
        if(pBufferInfo != 0)
        {

//#ifdef _DEBUG
//			char logString[256];
//			sprintf_s(logString, "BLUEFISH CONSUMER: GOT FREE BUFFER %d", pBufferInfo->id_);
//			LOG(logString, nbase::utils::LogLevelDebug);
//#endif

			unsigned char *pBuffer = (unsigned char*)(pBufferInfo->pBuffer_);
			if(pBuffer != 0) {
				memcpy(pBuffer, pSource->GetDataPtr(), pSource->GetDataSize());
				bReturnValue = true;
			}

			bufferPool_.PutFilledBuffer(*pBufferInfo, bContinuePlayback);
//#ifdef _DEBUG
//			sprintf_s(logString, "BLUEFISH CONSUMER: PUT FILLED BUFFER %d, CONTINUEPLAYBACK_FLAG=%s", pBufferInfo->id_, bContinuePlayback ? "true" : "false");
//			LOG(logString, nbase::utils::LogLevelDebug);
//#endif
			if(bForceOutput || bufferPool_.IsFull()) {
				LOG(TEXT("BLUEFISH CONSUMER: ENABLE PLAYBACK IN DMALOOP"), caspar::utils::LogLevelDebug);
				pDmaLoop_->StartPlayback();
			}
		}
		else {
			//What to do if no buffer is free? Something is obviously wrong since TIMEOUT is 1000 ms, during which a buffer should have been freed multiple times
			LOG(TEXT("BLUEFISH CONSUMER: FAILED TO GET NEXT FREE BUFFER"), caspar::utils::LogLevelDebug);
		}
    }
	return bReturnValue;
}*/

}	//namespace bluefish
}	//namespace caspar