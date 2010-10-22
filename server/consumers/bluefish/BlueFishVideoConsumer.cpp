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

#include <BlueVelvet4.h>
#include "..\..\application.h"
#include "BlueFishVideoConsumer.h"
#include "..\..\frame\FramePlaybackControl.h"
#include "BluefishPlaybackStrategy.h"

#include <stdlib.h>
#include <stdio.h>

namespace caspar { namespace bluefish {

int BlueFishVideoConsumer::EnumerateDevices()
{
	LOG << TEXT("Bluefish SDK version: ") << BlueVelvetVersion();
	BlueVelvetPtr pSDK(BlueVelvetFactory4());

	if(pSDK != 0) 
	{
		int deviceCount = 0;
		pSDK->device_enumerate(deviceCount);
		return deviceCount;
	}
	else
		return 0;
}

VideoConsumerPtr BlueFishVideoConsumer::Create(unsigned int deviceIndex)
{
	BlueFishVideoConsumerPtr card(new BlueFishVideoConsumer());
	if(card != 0 && card->SetupDevice(deviceIndex) == false)
		card.reset();

	return card;
}

BlueFishVideoConsumer::BlueFishVideoConsumer() : pSDK_(BlueVelvetFactory4()), currentFormat_(FFormatPAL), _deviceIndex(0)
{}

BlueFishVideoConsumer::~BlueFishVideoConsumer()
{
	ReleaseDevice();
}

IPlaybackControl* BlueFishVideoConsumer::GetPlaybackControl() const
{
	return pPlaybackControl_.get();
}

bool BlueFishVideoConsumer::SetupDevice(unsigned int deviceIndex)
{
	tstring strDesiredFrameFormat = caspar::GetApplication()->GetSetting(TEXT("videomode"));
	return this->DoSetupDevice(deviceIndex, strDesiredFrameFormat);
}

unsigned long BlueFishVideoConsumer::BlueSetVideoFormat(tstring strDesiredFrameFormat)
{
	unsigned long vidFmt = VID_FMT_PAL;
	unsigned long desiredVideoFormat = VID_FMT_PAL;

	if(strDesiredFrameFormat.size() == 0)
		strDesiredFrameFormat = TEXT("PAL");

	FrameFormat casparVideoFormat = caspar::GetVideoFormat(strDesiredFrameFormat);
	desiredVideoFormat = bluefish::VidFmtFromFrameFormat(casparVideoFormat);
	currentFormat_ = casparVideoFormat != FFormatInvalid ? casparVideoFormat : FFormatPAL;
	if(desiredVideoFormat == ULONG_MAX) 
	{
		LOG << TEXT("BLUECARD ERROR: Unsupported videomode: ") << strDesiredFrameFormat << TEXT(". (device ") << _deviceIndex << TEXT(")");
		return ULONG_MAX;
	}

	if(desiredVideoFormat != VID_FMT_PAL)
	{
		int videoModeCount = pSDK_->count_video_mode();
		for(int videoModeIndex=1; videoModeIndex <= videoModeCount; ++videoModeIndex) 
		{
			EVideoMode videoMode = pSDK_->enum_video_mode(videoModeIndex);
			if(videoMode == desiredVideoFormat) 
				vidFmt = videoMode;			
		}
	}

	if(vidFmt != desiredVideoFormat)
		LOG << TEXT("BLUECARD ERROR: Failed to set desired videomode: ") << strDesiredFrameFormat << TEXT(". (device ") << _deviceIndex << TEXT(")");
	
	if(vidFmt == VID_FMT_PAL)
	{
		strDesiredFrameFormat = TEXT("PAL");
		currentFormat_ = FFormatPAL;
	}
	return vidFmt;
}

bool BlueFishVideoConsumer::DoSetupDevice(unsigned int deviceIndex, tstring strDesiredFrameFormat)
{				
	memFmt_		= MEM_FMT_ARGB_PC;
	updFmt_		= UPD_FMT_FRAME;
	vidFmt_		= VID_FMT_PAL; 
	resFmt_		= RES_FMT_NORMAL; 
	engineMode_	= VIDEO_ENGINE_FRAMESTORE;

	_deviceIndex = deviceIndex;
		
	if(BLUE_FAIL(pSDK_->device_attach(_deviceIndex, FALSE))) 
	{
		LOG << TEXT("BLUECARD ERROR: Failed to attach device. (device ") << _deviceIndex << TEXT(")");;
		return false;
	}
	
	int videoCardType = pSDK_->has_video_cardtype();
	LOG << TEXT("BLUECARD INFO: Card type: ") << GetBluefishCardDesc(videoCardType) << TEXT(". (device ") << _deviceIndex << TEXT(")");;
	
	//void* pBlueDevice = blue_attach_to_device(1);
	//EBlueConnectorPropertySetting video_routing[1];
	//auto channel = BLUE_VIDEO_OUTPUT_CHANNEL_A;
	//video_routing[0].channel = channel;	
	//video_routing[0].propType = BLUE_CONNECTOR_PROP_SINGLE_LINK;
	//video_routing[0].connector = channel == BLUE_VIDEO_OUTPUT_CHANNEL_A ? BLUE_CONNECTOR_SDI_OUTPUT_A : BLUE_CONNECTOR_SDI_OUTPUT_B;
	//blue_set_connector_property(pBlueDevice, 1, video_routing);
	//blue_detach_from_device(&pBlueDevice);
		
	vidFmt_ = BlueSetVideoFormat(strDesiredFrameFormat);
	if(vidFmt_ == ULONG_MAX)
		return false;
		
	// Set default video output channel
	//if(BLUE_FAIL(SetCardProperty(pSDK_, DEFAULT_VIDEO_OUTPUT_CHANNEL, channel)))
	//	LOG << TEXT("BLUECARD ERROR: Failed to set default channel. (device ") << _deviceIndex << TEXT(")");

	//Setting output Video mode
	if(BLUE_FAIL(SetCardProperty(pSDK_, VIDEO_MODE, vidFmt_))) 
	{
		LOG << TEXT("BLUECARD ERROR: Failed to set videomode. (device ") << _deviceIndex << TEXT(")");
		return false;
	}

	//Select Update Mode for output
	if(BLUE_FAIL(SetCardProperty(pSDK_, VIDEO_UPDATE_TYPE, updFmt_))) 
	{
		LOG << TEXT("BLUECARD ERROR: Failed to set update type. (device ") << _deviceIndex << TEXT(")");
		return false;
	}

	DisableVideoOutput();

	//Enable dual link output
	if(BLUE_FAIL(SetCardProperty(pSDK_, VIDEO_DUAL_LINK_OUTPUT, 1)))
	{
		LOG << TEXT("BLUECARD ERROR: Failed to enable dual link. (device ") << _deviceIndex << TEXT(")");
		return false;
	}

	if(BLUE_FAIL(SetCardProperty(pSDK_, VIDEO_DUAL_LINK_OUTPUT_SIGNAL_FORMAT_TYPE, Signal_FormatType_4224)))
	{
		LOG << TEXT("BLUECARD ERROR: Failed to set dual link format type to 4:2:2:4. (device ") << _deviceIndex << TEXT(")");
		return false;
	}

	
	//Select output memory format
	if(BLUE_FAIL(SetCardProperty(pSDK_, VIDEO_MEMORY_FORMAT, memFmt_))) 
	{
		LOG << TEXT("BLUECARD ERROR: Failed to set memory format. (device ") << _deviceIndex << TEXT(")");
		return false;
	}

	//Select image orientation
	if(BLUE_FAIL(SetCardProperty(pSDK_, VIDEO_IMAGE_ORIENTATION, ImageOrientation_Normal)))
		LOG << TEXT("BLUECARD ERROR: Failed to set image orientation to normal. (device ") << _deviceIndex << TEXT(")");	

	// Select data range
	if(BLUE_FAIL(SetCardProperty(pSDK_, VIDEO_RGB_DATA_RANGE, CGR_RANGE))) 
		LOG << TEXT("BLUECARD ERROR: Failed to set RGB data range to CGR. (device ") << _deviceIndex << TEXT(")");	
		
	if(BLUE_FAIL(SetCardProperty(pSDK_, VIDEO_PREDEFINED_COLOR_MATRIX, vidFmt_ == VID_FMT_PAL ? MATRIX_601_CGR : MATRIX_709_CGR)))
		LOG << TEXT("BLUECARD ERROR: Failed to set colormatrix to ") << (vidFmt_ == VID_FMT_PAL ? TEXT("601 CGR") : TEXT("709 CGR")) << TEXT(". (device ") << _deviceIndex << TEXT(")");
		
	//if(BLUE_FAIL(SetCardProperty(pSDK_, EMBEDDED_AUDIO_OUTPUT, 0))) 	
	//	LOG << TEXT("BLUECARD ERROR: Failed to enable embedded audio. (device ") << _deviceIndex << TEXT(")");	
	//else 
	//{
	//	LOG << TEXT("BLUECARD INFO: Enabled embedded audio. (device ") << _deviceIndex << TEXT(")");
	//	hasEmbeddedAudio_ = true;
	//}

	LOG << TEXT("BLUECARD INFO: Successfully configured bluecard for ") << strDesiredFrameFormat << TEXT(". (device ") << _deviceIndex << TEXT(")");

	if (pSDK_->has_output_key()) 
	{
		int dummy = TRUE;
		int v4444 = FALSE;
		int invert = FALSE;
		int white = FALSE;
		pSDK_->set_output_key(dummy, v4444, invert, white);
	}

	if(pSDK_->GetHDCardType(_deviceIndex) != CRD_HD_INVALID) 
		pSDK_->Set_DownConverterSignalType(vidFmt_ == VID_FMT_PAL ? SD_SDI : HD_SDI);	
	
	if(BLUE_FAIL(pSDK_->set_video_engine(engineMode_)))
	{
		LOG << TEXT("BLUECARD ERROR: Failed to set vido engine. (device ") << _deviceIndex << TEXT(")");
		return false;
	}

	EnableVideoOutput();

	LOG << TEXT("BLUECARD INFO: Successfully initialized device ") << _deviceIndex;

	pFrameManager_ = std::make_shared<SystemFrameManager>(FrameFormatDescription::FormatDescriptions[FFormatPAL]);

	pPlaybackControl_ = std::make_shared<FramePlaybackControl>(FramePlaybackStrategyPtr(new BluefishPlaybackStrategy(this)));
	pPlaybackControl_->Start();

	return true;
}

bool BlueFishVideoConsumer::ReleaseDevice()
{
	pPlaybackControl_.reset();
	pFrameManager_.reset();

	DisableVideoOutput();

	if(pSDK_) 
		pSDK_->device_detach();	

	LOG << TEXT("BLUECARD INFO: Successfully released device ") << _deviceIndex;
	return true;
}

void BlueFishVideoConsumer::EnableVideoOutput()
{	
	if(pSDK_)
	{
		if(BLUE_FAIL(SetCardProperty(pSDK_, VIDEO_BLACKGENERATOR, 0))) 
			LOG << TEXT("BLUECARD ERROR: Failed to disable video output. (device ") << _deviceIndex << TEXT(")");		
	}
}

void BlueFishVideoConsumer::DisableVideoOutput()
{	
	if(pSDK_)
	{
		if(BLUE_FAIL(SetCardProperty(pSDK_, VIDEO_BLACKGENERATOR, 1))) 
			LOG << TEXT("BLUECARD ERROR: Failed to disable video output. (device ") << _deviceIndex << TEXT(")");		
	}
}

bool BlueFishVideoConsumer::SetVideoFormat(const tstring& strDesiredFrameFormat)
{
	tstring prevFrameFormat = this->GetFormatDescription();

	unsigned long desiredVideoFormat = bluefish::VidFmtFromFrameFormat(caspar::GetVideoFormat(strDesiredFrameFormat));
	if(desiredVideoFormat == ULONG_MAX)
	{
		LOG << TEXT("BLUECARD INFO: Unsupported video format. Ignored ") << strDesiredFrameFormat;
		return false;
	}

	this->ReleaseDevice();
	
	if(!this->DoSetupDevice(this->_deviceIndex, strDesiredFrameFormat))
	{	
		LOG << TEXT("BLUECARD ERROR: Failed to set video format. Trying to revert to previous format ") << prevFrameFormat;
		DoSetupDevice(this->_deviceIndex, prevFrameFormat);
		return false;
	}

	LOG << TEXT("BLUECARD INFO: Successfully set video format ") << strDesiredFrameFormat;

	return true;
}

}}
