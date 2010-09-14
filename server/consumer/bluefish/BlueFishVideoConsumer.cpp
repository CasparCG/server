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
 
#include "..\..\StdAfx.h"

#ifndef DISABLE_BLUEFISH

#include <BlueVelvet4.h>
#include "BlueFishVideoConsumer.h"
#include "BluefishPlaybackStrategy.h"

#include <stdlib.h>
#include <stdio.h>

#if defined(_MSC_VER)
#pragma warning (push, 1) // TODO: Legacy code, just disable warnings
#endif

namespace caspar {
namespace bluefish {

///////////////////////////////////////////
// BlueFishVideoConsumer::EnumerateDevices
// RETURNS: Number of identified bluefish-cards
int BlueFishVideoConsumer::EnumerateDevices()
{
	CASPAR_LOG(info) << "Bleufhsi SDK version: " << BlueVelvetVersion;
	BlueVelvetPtr pSDK(BlueVelvetFactory4());

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
frame_consumer_ptr BlueFishVideoConsumer::Create(const frame_format_desc& format_desc, unsigned int deviceIndex)
{
	BlueFishFrameConsumerPtr card(new BlueFishVideoConsumer(format_desc));
	if(card != 0 && card->SetupDevice(deviceIndex) == false)
		card.reset();

	return card;
}

////////////////////////////////////////
// BlueFishVideoConsumer constructor
BlueFishVideoConsumer::BlueFishVideoConsumer(const frame_format_desc& format_desc) : format_desc_(format_desc), pSDK_(BlueVelvetFactory4()), currentFormat_(frame_format::pal), _deviceIndex(0), hasEmbeddedAudio_(false)
{
	frameBuffer_.set_capacity(1);
	thread_ = boost::thread([=]{Run();});
}

////////////////////////////////////////
// BlueFishVideoConsumer destructor
BlueFishVideoConsumer::~BlueFishVideoConsumer()
{
	frameBuffer_.push(nullptr),
	thread_.join();
	ReleaseDevice();
}

/*******************/
/**    METHODS    **/
/*******************/

unsigned long BlueFishVideoConsumer::VidFmtFromFrameFormat(frame_format fmt) 
{
	switch(fmt)
	{
	case frame_format::pal:			return VID_FMT_PAL;
	case frame_format::ntsc:		return VID_FMT_NTSC;
	case frame_format::x576p2500:	return ULONG_MAX;	//not supported
	case frame_format::x720p5000:	return VID_FMT_720P_5000;
	case frame_format::x720p5994:	return VID_FMT_720P_5994;
	case frame_format::x720p6000:	return VID_FMT_720P_6000;
	case frame_format::x1080p2397:	return VID_FMT_1080P_2397;
	case frame_format::x1080p2400:	return VID_FMT_1080P_2400;
	case frame_format::x1080i5000:	return VID_FMT_1080I_5000;
	case frame_format::x1080i5994:	return VID_FMT_1080I_5994;
	case frame_format::x1080i6000:	return VID_FMT_1080I_6000;
	case frame_format::x1080p2500:	return VID_FMT_1080P_2500;
	case frame_format::x1080p2997:	return VID_FMT_1080P_2997;
	case frame_format::x1080p3000:	return VID_FMT_1080P_3000;
	default:						return ULONG_MAX;
	}
}

TCHAR* GetBluefishCardDesc(int cardType) 
{
	switch(cardType) 
	{
	case CRD_BLUEDEEP_LT:				return TEXT("Deepblue LT"); // D64 Lite
	case CRD_BLUEDEEP_SD:				return TEXT("Iridium SD"); // Iridium SD
	case CRD_BLUEDEEP_AV:				return TEXT("Iridium AV"); // Iridium AV
	case CRD_BLUEDEEP_IO:				return TEXT("Deepblue IO"); // D64 Full
	case CRD_BLUEWILD_AV:				return TEXT("Wildblue AV"); // D64 AV
	case CRD_IRIDIUM_HD:				return TEXT("Iridium HD"); // * Iridium HD
	case CRD_BLUEWILD_RT:				return TEXT("Wildblue RT"); // D64 RT
	case CRD_BLUEWILD_HD:				return TEXT("Wildblue HD"); // * BadAss G2
	case CRD_REDDEVIL:					return TEXT("Iridium Full"); // Iridium Full
	case CRD_BLUEDEEP_HD:														 // * BadAss G2 variant, proposed, reserved
	case CRD_BLUEDEEP_HDS:				return TEXT("Reserved for \"BasAss G2"); // * BadAss G2 variant, proposed, reserved
	case CRD_BLUE_ENVY:					return TEXT("Blue envy"); // Mini Din 
	case CRD_BLUE_PRIDE:				return TEXT("Blue pride"); //Mini Din Output 
	case CRD_BLUE_GREED:				return TEXT("Blue greed");
	case CRD_BLUE_INGEST:				return TEXT("Blue ingest");
	case CRD_BLUE_SD_DUALLINK:			return TEXT("Blue SD duallink");
	case CRD_BLUE_CATALYST:				return TEXT("Blue catalyst");
	case CRD_BLUE_SD_DUALLINK_PRO:		return TEXT("Blue SD duallink pro");
	case CRD_BLUE_SD_INGEST_PRO:		return TEXT("Blue SD ingest pro");
	case CRD_BLUE_SD_DEEPBLUE_LITE_PRO:	return TEXT("Blue SD deepblue lite pro");
	case CRD_BLUE_SD_SINGLELINK_PRO:	return TEXT("Blue SD singlelink pro");
	case CRD_BLUE_SD_IRIDIUM_AV_PRO:	return TEXT("Blue SD iridium AV pro");
	case CRD_BLUE_SD_FIDELITY:			return TEXT("Blue SD fidelity");
	case CRD_BLUE_SD_FOCUS:				return TEXT("Blue SD focus");
	case CRD_BLUE_SD_PRIME:				return TEXT("Blue SD prime");
	case CRD_BLUE_EPOCH_2K_CORE:		return TEXT("Blue epoch 2k core");
	case CRD_BLUE_EPOCH_2K_ULTRA:		return TEXT("Blue epoch 2k ultra");
	case CRD_BLUE_EPOCH_HORIZON:		return TEXT("Blue epoch horizon");
	case CRD_BLUE_EPOCH_CORE:			return TEXT("Blue epoch core");
	case CRD_BLUE_EPOCH_ULTRA:			return TEXT("Blue epoch ultra");
	case CRD_BLUE_CREATE_HD:			return TEXT("Blue create HD");
	case CRD_BLUE_CREATE_2K:			return TEXT("Blue create 2k");
	case CRD_BLUE_CREATE_2K_ULTRA:		return TEXT("Blue create 2k ultra");
	default:							return TEXT("Unknown");
	}
}

bool BlueFishVideoConsumer::SetupDevice(unsigned int deviceIndex)
{
	return this->DoSetupDevice(deviceIndex);
}

/*
// Original initialization code
bool BlueFishVideoConsumer::DoSetupDevice(unsigned int deviceIndex, std::wstring strDesiredFrameFormat)
{
	_deviceIndex = deviceIndex;

	unsigned long memFmt = MEM_FMT_ARGB_PC, updFmt = UPD_FMT_FRAME, vidFmt = VID_FMT_PAL, resFmt = RES_FMT_NORMAL;
	unsigned long desiredVideoFormat = VID_FMT_PAL;
	int iDummy;

	int bufferIndex=0;	//Bufferindex used when initializing the buffers

	if(strDesiredFrameFormat.size() == 0)
		strDesiredFrameFormat = TEXT("PAL");

	frame_format casparVideoFormat = caspar::get_video_format(strDesiredFrameFormat);
	desiredVideoFormat = BlueFishVideoConsumer::VidFmtFromFrameFormat(casparVideoFormat);
	currentFormat_ = casparVideoFormat != FFormatInvalid ? casparVideoFormat : FFormatPAL;
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
		LOG << TEXT("BLUECARD INFO: Buffers: ") << m_bufferCount << TEXT(", \"Length\": ") << m_length << TEXT(", Buffer size: ") << m_actual << TEXT(" (device ") << _deviceIndex << TEXT(")") << common::LogStream::Flush;
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
*/

//New, improved(?) initialization code. 
//Based on code sent from the bluefish-sdk support 2009-08-25. Email "RE: [sdk] Ang. RE: Issue with SD Lite Pro PCI-E"
bool BlueFishVideoConsumer::DoSetupDevice(unsigned int deviceIndex)
{
	_deviceIndex = deviceIndex;

	unsigned long memFmt = MEM_FMT_ARGB_PC, updFmt = UPD_FMT_FRAME, vidFmt = VID_FMT_PAL, resFmt = RES_FMT_NORMAL, engineMode = VIDEO_ENGINE_FRAMESTORE;
	unsigned long desiredVideoFormat = VID_FMT_PAL;
	int iDummy;

	int bufferIndex=0;	//Bufferindex used when initializing the buffers

	if(BLUE_FAIL(pSDK_->device_attach(_deviceIndex, FALSE))) {
		CASPAR_LOG(error) << "BLUECARD ERROR: Failed to attach device. (device " << _deviceIndex << TEXT(")");
		return false;
	}

	int videoCardType = pSDK_->has_video_cardtype();
	CASPAR_LOG(info) << "BLUECARD INFO: Card type: " << GetBluefishCardDesc(videoCardType) << TEXT(". (device ") << _deviceIndex << TEXT(")");

	desiredVideoFormat = BlueFishVideoConsumer::VidFmtFromFrameFormat(format_desc_.format);
	currentFormat_ = format_desc_.format != frame_format::invalid ? format_desc_.format : frame_format::pal;
	if(desiredVideoFormat == ULONG_MAX) {
		CASPAR_LOG(error) << "BLUECARD ERROR: Unsupported videomode: " << format_desc_.name << TEXT(". (device ") << _deviceIndex << TEXT(")");
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

	if(vidFmt != desiredVideoFormat) {
		CASPAR_LOG(error) << "BLUECARD ERROR: Failed to set desired videomode: " << format_desc_.name << TEXT(". (device ") << _deviceIndex << TEXT(")");
	}

	if(vidFmt == VID_FMT_PAL) {
		currentFormat_ = frame_format::pal;
		format_desc_ = frame_format_desc::format_descs[frame_format::pal];
	}

	DisableVideoOutput();

	VARIANT value;
	value.vt = VT_UI4;

	//Enable dual link output
	value.ulVal = 1;
	if(!BLUE_PASS(pSDK_->SetCardProperty(VIDEO_DUAL_LINK_OUTPUT, value))) {
		CASPAR_LOG(error) << "BLUECARD ERROR: Failed to enable dual link. (device " << _deviceIndex << TEXT(")");
		return false;
	}

	value.ulVal = Signal_FormatType_4224;
	if(!BLUE_PASS(pSDK_->SetCardProperty(VIDEO_DUAL_LINK_OUTPUT_SIGNAL_FORMAT_TYPE, value))) {
		CASPAR_LOG(error) << "BLUECARD ERROR: Failed to set dual link format type to 4:2:2:4. (device " << _deviceIndex << TEXT(")");
		return false;
	}

	//Setting output Video mode
	value.ulVal = vidFmt;
	if(!BLUE_PASS(pSDK_->SetCardProperty(VIDEO_MODE, value))) {
		CASPAR_LOG(error) << "BLUECARD ERROR: Failed to set videomode. (device " << _deviceIndex << TEXT(")");
		return false;
	}

	//Select Update Mode for output
	value.ulVal = updFmt;
	if(!BLUE_PASS(pSDK_->SetCardProperty(VIDEO_UPDATE_TYPE, value))) {
		CASPAR_LOG(error) << "BLUECARD ERROR: Failed to set update type. (device " << _deviceIndex << TEXT(")");
		return false;
	}
	
	//Select output memory format
	value.ulVal = memFmt;
	if(!BLUE_PASS(pSDK_->SetCardProperty(VIDEO_MEMORY_FORMAT, value))) {
		CASPAR_LOG(error) << "BLUECARD ERROR: Failed to set memory format. (device " << _deviceIndex << TEXT(")");
		return false;
	}

	//SELECT IMAGE ORIENTATION
	value.ulVal = ImageOrientation_Normal;
	if(!BLUE_PASS(pSDK_->SetCardProperty(VIDEO_IMAGE_ORIENTATION, value))) {
		CASPAR_LOG(error) << "BLUECARD ERROR: Failed to set image orientation to normal. (device " << _deviceIndex << TEXT(")");
	}

	value.ulVal = CGR_RANGE;
	if(!BLUE_PASS(pSDK_->SetCardProperty(VIDEO_RGB_DATA_RANGE, value))) {
		CASPAR_LOG(error) << "BLUECARD ERROR: Failed to set RGB data range to CGR. (device " << _deviceIndex << TEXT(")");
	}

	value.ulVal = MATRIX_709_CGR;
	if(vidFmt == VID_FMT_PAL) {
		value.ulVal = MATRIX_601_CGR;
	}
	if(!BLUE_PASS(pSDK_->SetCardProperty(VIDEO_PREDEFINED_COLOR_MATRIX, value))) {
		CASPAR_LOG(error) << "BLUECARD ERROR: Failed to set colormatrix to " << (vidFmt == VID_FMT_PAL ? TEXT("601 CGR") : TEXT("709 CGR")) << TEXT(". (device ") << _deviceIndex << TEXT(")");
	}
	

	//Disable embedded audio
	value.ulVal = 1;
	if(!BLUE_PASS(pSDK_->SetCardProperty(EMBEDDED_AUDIO_OUTPUT, value))) {
		CASPAR_LOG(error) << "BLUECARD ERROR: Failed to enable embedded audio. (device " << _deviceIndex << TEXT(")");
	}
	else {
		CASPAR_LOG(info) << "BLUECARD INFO: Enabled embedded audio. (device " << _deviceIndex << TEXT(")");
		hasEmbeddedAudio_ = true;
	}

	CASPAR_LOG(info) << "BLUECARD INFO: Successfully configured bluecard for " << format_desc_.name << TEXT(". (device ") << _deviceIndex << TEXT(")");

	if (pSDK_->has_output_key()) {
		iDummy = TRUE;
		int v4444 = FALSE, invert = FALSE, white = FALSE;
		pSDK_->set_output_key(iDummy, v4444, invert, white);
	}

	if(pSDK_->GetHDCardType(_deviceIndex) != CRD_HD_INVALID) {
		pSDK_->Set_DownConverterSignalType((vidFmt == VID_FMT_PAL) ? SD_SDI : HD_SDI);
	}

	ULONG videoGolden = BlueVelvetGolden(vidFmt, memFmt, updFmt);

	pFrameManager_ = BluefishFrameManagerPtr(new BluefishFrameManager(pSDK_, currentFormat_, videoGolden));

	pPlayback_ = std::make_shared<BluefishPlaybackStrategy>(this);

	if(BLUE_FAIL(pSDK_->set_video_engine(engineMode))) {
		CASPAR_LOG(error) << "BLUECARD ERROR: Failed to set vido engine. (device " << _deviceIndex << TEXT(")");
		return false;
	}

	EnableVideoOutput();

	CASPAR_LOG(info) << "BLUECARD INFO: Successfully initialized device " << _deviceIndex;
	return true;
}

bool BlueFishVideoConsumer::ReleaseDevice()
{
	pPlayback_.reset();

	pFrameManager_.reset();
	DisableVideoOutput();

	if(pSDK_) {
		pSDK_->device_detach();
	}

	CASPAR_LOG(info) << "BLUECARD INFO: Successfully released device " << _deviceIndex;
	return true;
}

void BlueFishVideoConsumer::EnableVideoOutput()
{
	//Need sync. protection?
	if(pSDK_)
	{
		VARIANT value;
		value.vt = VT_UI4;

		//Deactivate channel
		value.ulVal = 0;
		if(!BLUE_PASS(pSDK_->SetCardProperty(VIDEO_BLACKGENERATOR, value))) {
			CASPAR_LOG(error) << "BLUECARD ERROR: Failed to disable video output. (device " << _deviceIndex << TEXT(")");
		}
	}
}

void BlueFishVideoConsumer::DisableVideoOutput()
{
	//Need sync. protection?
	if(pSDK_)
	{
		VARIANT value;
		value.vt = VT_UI4;

		//Deactivate channel
		value.ulVal = 1;
		if(!BLUE_PASS(pSDK_->SetCardProperty(VIDEO_BLACKGENERATOR, value))) {
			CASPAR_LOG(error) << "BLUECARD ERROR: Failed to disable video output. (device " << _deviceIndex << TEXT(")");
		}
	}
}

void BlueFishVideoConsumer::display(const frame_ptr& frame)
{
	if(frame == nullptr)
		return;

	if(pException_ != nullptr)
		std::rethrow_exception(pException_);

	frameBuffer_.push(frame);
}

void BlueFishVideoConsumer::Run()
{
	while(true)
	{
		try
		{
			frame_ptr frame;
			frameBuffer_.pop(frame);
			if(frame == nullptr)
				return;

			pPlayback_->display(frame);
		}
		catch(...)
		{
			pException_ = std::current_exception();
		}
	}	
}

}}

#endif