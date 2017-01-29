/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "../StdAfx.h"
#include "blue_velvet.h"
#include <common/utf.h>	
#include <core/video_format.h>
#include <bluefish/interop/BlueVelvetCUtils.h>

#if defined(__APPLE__)
	#include <dlfcn.h>
#endif

#if defined(_WIN32)
	#define GET_PROCADDR_FOR_FUNC(name, module) { name = (pFunc_##name)GetProcAddress(module, #name); if(!name) { return false; } }
#elif defined(__APPLE__)
	#define GET_PROCADDR_FOR_FUNC(name, module) { name = (pFunc_##name)dlsym(module, #name); if(!name) { return false; } }
#endif

namespace caspar { namespace bluefish {

	BvcWrapper::BvcWrapper()
	{
		bvc = nullptr;
		hModule = nullptr;
		InitFunctionsPointers();
		if (bfcFactory)
			bvc = bfcFactory();
	}

	BvcWrapper::~BvcWrapper()
	{
		if (bvc)
			bfcDestroy(bvc);
		if (hModule)
		{
#if defined(_WIN32)
			FreeLibrary(hModule);
#elif defined(__APPLE__)
			dlclose(hModule);
#endif
		}
	}

	bool BvcWrapper::InitFunctionsPointers()
	{
		bool bRes = false;

#if defined(_WIN32)
#ifdef _DEBUG
		hModule = LoadLibraryExA("BlueVelvetC64_d.dll", NULL, 0);
#else
		hModule = LoadLibraryExA("BlueVelvetC64.dll", NULL, 0);
#endif
		
#elif defined(__APPLE__)
		// Look for the framework and load it accordingly.
		char* libraryPath("/Library/Frameworks/BlueVelvetC.framework");		// full path may not be required, OSX might check in /l/f by default - MUST TEST!
		hModule = dlopen(libraryPath, RTLD_NOW);
#endif
		
		if (hModule)
		{
			GET_PROCADDR_FOR_FUNC(bfcGetVersion, hModule);
			GET_PROCADDR_FOR_FUNC(bfcFactory, hModule);
			GET_PROCADDR_FOR_FUNC(bfcDestroy, hModule);
			GET_PROCADDR_FOR_FUNC(bfcEnumerate, hModule);
			GET_PROCADDR_FOR_FUNC(bfcQueryCardType, hModule);
			GET_PROCADDR_FOR_FUNC(bfcAttach, hModule);
			GET_PROCADDR_FOR_FUNC(bfcDetach, hModule);
			GET_PROCADDR_FOR_FUNC(bfcQueryCardProperty32, hModule);
			GET_PROCADDR_FOR_FUNC(bfcQueryCardProperty64, hModule);
			GET_PROCADDR_FOR_FUNC(bfcSetCardProperty32, hModule);
			GET_PROCADDR_FOR_FUNC(bfcSetCardProperty64, hModule);
			GET_PROCADDR_FOR_FUNC(bfcGetCardSerialNumber, hModule);
			GET_PROCADDR_FOR_FUNC(bfcGetCardFwVersion, hModule);
			GET_PROCADDR_FOR_FUNC(bfcWaitVideoSyncAsync, hModule);
			GET_PROCADDR_FOR_FUNC(bfcWaitVideoInputSync, hModule);
			GET_PROCADDR_FOR_FUNC(bfcWaitVideoOutputSync, hModule);
			GET_PROCADDR_FOR_FUNC(bfcGetVideoOutputCurrentFieldCount, hModule);
			GET_PROCADDR_FOR_FUNC(bfcGetVideoInputCurrentFieldCount, hModule);
			GET_PROCADDR_FOR_FUNC(bfcVideoCaptureStart, hModule);
			GET_PROCADDR_FOR_FUNC(bfcVideoCaptureStop, hModule);
			GET_PROCADDR_FOR_FUNC(bfcVideoPlaybackStart, hModule);
			GET_PROCADDR_FOR_FUNC(bfcVideoPlaybackStop, hModule);
			GET_PROCADDR_FOR_FUNC(bfcVideoPlaybackAllocate, hModule);
			GET_PROCADDR_FOR_FUNC(bfcVideoPlaybackPresent, hModule);
			GET_PROCADDR_FOR_FUNC(bfcVideoPlaybackRelease, hModule);
			GET_PROCADDR_FOR_FUNC(bfcGetCaptureVideoFrameInfoEx, hModule);
			GET_PROCADDR_FOR_FUNC(bfcRenderBufferCapture, hModule);
			GET_PROCADDR_FOR_FUNC(bfcRenderBufferUpdate, hModule);
			GET_PROCADDR_FOR_FUNC(bfcGetRenderBufferCount, hModule);
			GET_PROCADDR_FOR_FUNC(bfcEncodeHancFrameEx, hModule);
			GET_PROCADDR_FOR_FUNC(bfcDecodeHancFrameEx, hModule);
			GET_PROCADDR_FOR_FUNC(bfcSystemBufferReadAsync, hModule);
			GET_PROCADDR_FOR_FUNC(bfcSystemBufferWriteAsync, hModule);
			GET_PROCADDR_FOR_FUNC(bfcGetBytesForGroupPixels, hModule);
			GET_PROCADDR_FOR_FUNC(bfcGetPixelsPerLine, hModule);
			GET_PROCADDR_FOR_FUNC(bfcGetLinesPerFrame, hModule);
			GET_PROCADDR_FOR_FUNC(bfcGetBytesPerLine, hModule);
			GET_PROCADDR_FOR_FUNC(bfcGetBytesPerFrame, hModule);
			GET_PROCADDR_FOR_FUNC(bfcGetGoldenValue, hModule);
			GET_PROCADDR_FOR_FUNC(bfcGetVBILines, hModule);
			GET_PROCADDR_FOR_FUNC(bfcGetVANCGoldenValue, hModule);
			GET_PROCADDR_FOR_FUNC(bfcGetVANCLineBytes, hModule);
			GET_PROCADDR_FOR_FUNC(bfcGetVANCLineCount, hModule);
			GET_PROCADDR_FOR_FUNC(bfcGetWindowsDriverHandle, hModule);
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetStringForCardType, hModule);
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetStringForBlueProductId, hModule);
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetStringForVideoMode, hModule);
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetStringForMemoryFormat, hModule);
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetMR2Routing, hModule);
			GET_PROCADDR_FOR_FUNC(bfcUtilsSetMR2Routing, hModule);
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetAudioOutputRouting, hModule);
			GET_PROCADDR_FOR_FUNC(bfcUtilsSetAudioOutputRouting, hModule);
			GET_PROCADDR_FOR_FUNC(bfcUtilsIsVideoModeProgressive, hModule);
			GET_PROCADDR_FOR_FUNC(bfcUtilsIsVideoMode1001Framerate, hModule);
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetFpsForVideoMode, hModule);
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetVideoModeForFrameInfo, hModule);
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetFrameInfoForVideoMode, hModule);
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetAudioSamplesPerFrame, hModule);

			bRes = true;
		}

		return bRes;
	}

	bool BvcWrapper::IsBvcValid()
	{
		if (bvc)
			return true;
		else
			return false;
	}

	const char * BvcWrapper::GetVersion()
	{
		return bfcGetVersion();
	}

	BLUE_UINT32 BvcWrapper::Attach(int iDeviceId)
	{
		return bfcAttach(bvc, iDeviceId);
	}

	BLUE_UINT32 BvcWrapper::Detach()
	{
		return bfcDetach(bvc);
	}

	BLUE_UINT32 BvcWrapper::QueryCardProperty32(const int iProperty, unsigned int & nValue)
	{
		if (bvc)
			return (BLUE_UINT32)bfcQueryCardProperty32(bvc, iProperty, nValue);
		else
			return 0;
	}

	BLUE_UINT32 BvcWrapper::SetCardProperty32(const int iProperty, const unsigned int nValue)
	{
		return bfcSetCardProperty32(bvc, iProperty, nValue);
	}

	BLUE_UINT32 BvcWrapper::Enumerate(int & iDevices)
	{
		return bfcEnumerate(bvc, iDevices);
	}

	BLUE_UINT32 BvcWrapper::QueryCardType(int & iCardType, int iDeviceID)
	{
		return bfcQueryCardType(bvc, iCardType, iDeviceID);
	}

	BLUE_UINT32 BvcWrapper::SystemBufferWrite(unsigned char * pPixels, unsigned long ulSize, unsigned long ulBufferID, unsigned long ulOffset)
	{
		return bfcSystemBufferWriteAsync(bvc, pPixels, ulSize, nullptr, ulBufferID, ulOffset);
	}

	BLUE_UINT32 BvcWrapper::SystemBufferRead(unsigned char* pPixels, unsigned long ulSize, unsigned long ulBufferID, unsigned long ulOffset)
	{
		return bfcSystemBufferReadAsync(bvc, pPixels, ulSize, nullptr, ulBufferID, ulOffset);
	}

	BLUE_UINT32 BvcWrapper::VideoPlaybackStop(int iWait, int iFlush)
	{
		return bfcVideoPlaybackStop(bvc, iWait, iFlush);
	}

	BLUE_UINT32 BvcWrapper::WaitVideoOutputSync(unsigned long ulUpdateType, unsigned long & ulFieldCount)
	{
		return bfcWaitVideoOutputSync(bvc, ulUpdateType, ulFieldCount);
	}

	BLUE_UINT32 BvcWrapper::WaitVideoInputSync(unsigned long ulUpdateType, unsigned long & ulFieldCount)
	{
		return bfcWaitVideoInputSync(bvc, ulUpdateType, ulFieldCount);
	}

	BLUE_UINT32 BvcWrapper::RenderBufferUpdate( unsigned long ulBufferID)
	{
		return bfcRenderBufferUpdate(bvc, ulBufferID);
	}

	BLUE_UINT32 BvcWrapper::RenderBufferCapture(unsigned long ulBufferID)
	{
		return bfcRenderBufferCapture(bvc, ulBufferID);
	}

	BLUE_UINT32 BvcWrapper::EncodeHancFrameEx(unsigned int nCardType, hanc_stream_info_struct * pHancEncodeInfo, void * pAudioBuffer, unsigned int nAudioChannels, unsigned int nAudioSamples, unsigned int nSampleType, unsigned int nAudioFlags)
	{
		return bfcEncodeHancFrameEx(bvc, CRD_BLUE_NEUTRON, pHancEncodeInfo, pAudioBuffer, nAudioChannels, nAudioSamples, nSampleType, nAudioFlags);
	}

	BLUE_UINT32 BvcWrapper::DecodeHancFrameEx(unsigned int nCardType, unsigned int * pHancBuffer, hanc_decode_struct * pHancDecodeInfo)
	{
		return bfcDecodeHancFrameEx(bvc, CRD_BLUE_NEUTRON, pHancBuffer, pHancDecodeInfo);
	}

	BLUE_UINT32 BvcWrapper::GetFrameInfoForVideoVode(const unsigned int nVideoMode, unsigned int&  nWidth, unsigned int& nHeight, unsigned int& nRate, unsigned int& bIs1001, unsigned int& bIsProgressive)
	{
		return bfcUtilsGetFrameInfoForVideoMode(nVideoMode, nWidth, nHeight, nRate, bIs1001, bIsProgressive);
	}

	BLUE_UINT32 BvcWrapper::GetBytesPerFrame(EVideoMode nVideoMode, EMemoryFormat nMemoryFormat, EUpdateMethod nUpdateMethod, unsigned int& nBytesPerFrame)
	{
		return bfcGetBytesPerFrame(nVideoMode, nMemoryFormat, nUpdateMethod, nBytesPerFrame);
	}


EVideoMode vid_fmt_from_video_format(const core::video_format& fmt) 
{
	switch(fmt)
	{
	case core::video_format::pal:			return VID_FMT_PAL;
	case core::video_format::ntsc:			return VID_FMT_NTSC;
	case core::video_format::x576p2500:		return VID_FMT_INVALID;	//not supported
	case core::video_format::x720p2398:		return VID_FMT_720P_2398;
	case core::video_format::x720p2400:		return VID_FMT_720P_2400;
	case core::video_format::x720p2500:		return VID_FMT_720P_2500;
	case core::video_format::x720p5000:		return VID_FMT_720P_5000;
	case core::video_format::x720p2997:		return VID_FMT_720P_2997;
	case core::video_format::x720p5994:		return VID_FMT_720P_5994;
	case core::video_format::x720p3000:		return VID_FMT_720P_3000;
	case core::video_format::x720p6000:		return VID_FMT_720P_6000;
	case core::video_format::x1080p2398:	return VID_FMT_1080P_2397;
	case core::video_format::x1080p2400:	return VID_FMT_1080P_2400;
	case core::video_format::x1080i5000:	return VID_FMT_1080I_5000;
	case core::video_format::x1080i5994:	return VID_FMT_1080I_5994;
	case core::video_format::x1080i6000:	return VID_FMT_1080I_6000;
	case core::video_format::x1080p2500:	return VID_FMT_1080P_2500;
	case core::video_format::x1080p2997:	return VID_FMT_1080P_2997;
	case core::video_format::x1080p3000:	return VID_FMT_1080P_3000;
	case core::video_format::x1080p5000:	return VID_FMT_1080P_5000;
	case core::video_format::x1080p5994:	return VID_FMT_1080P_5994;
	case core::video_format::x1080p6000:	return VID_FMT_1080P_6000;
	default:								return VID_FMT_INVALID;
	}
}

bool is_epoch_card(BvcWrapper& blue)
{
	int iDeviceID = 1;
	int iCardType = 0;
	blue.QueryCardType(iCardType, iDeviceID);

	switch(iCardType)
	{
		case CRD_BLUE_EPOCH_HORIZON:
		case CRD_BLUE_EPOCH_CORE:
		case CRD_BLUE_EPOCH_ULTRA:
		case CRD_BLUE_EPOCH_2K_HORIZON:
		case CRD_BLUE_EPOCH_2K_CORE:
		case CRD_BLUE_EPOCH_2K_ULTRA:
		case CRD_BLUE_CREATE_HD:
		case CRD_BLUE_CREATE_2K:
		case CRD_BLUE_CREATE_2K_ULTRA:
		case CRD_BLUE_SUPER_NOVA:
		case CRD_BLUE_SUPER_NOVA_S_PLUS:
		case CRD_BLUE_NEUTRON:
		case CRD_BLUE_EPOCH_CG:
		return true;
	default:
		return false;
	}
}

bool is_epoch_neutron_1i2o_card(BvcWrapper& blue)
{
	BLUE_UINT32 val = 0;
	blue.QueryCardProperty32(EPOCH_GET_PRODUCT_ID, val);

	if (val == ORAC_NEUTRON_1_IN_2_OUT_FIRMWARE_PRODUCTID)
		return true;
	else
		return false;
}

bool is_epoch_neutron_3o_card(BvcWrapper& blue)
{
	BLUE_UINT32 val = 0;
	blue.QueryCardProperty32(EPOCH_GET_PRODUCT_ID, val);

	if (val == ORAC_NEUTRON_0_IN_3_OUT_FIRMWARE_PRODUCTID)
		return true;
	else
		return false;
}


std::wstring get_card_desc(BvcWrapper& blue, int iDeviceID)
{
	int iCardType = 0;
	blue.QueryCardType(iCardType, iDeviceID);

	switch(iCardType) 
	{
		case CRD_BLUEDEEP_LT:				return L"Deepblue LT";// D64 Lite
		case CRD_BLUEDEEP_SD:				return L"Iridium SD";// Iridium SD
		case CRD_BLUEDEEP_AV:				return L"Iridium AV";// Iridium AV
		case CRD_BLUEDEEP_IO:				return L"Deepblue IO";// D64 Full
		case CRD_BLUEWILD_AV:				return L"Wildblue AV";// D64 AV
		case CRD_IRIDIUM_HD:				return L"Iridium HD";// * Iridium HD
		case CRD_BLUEWILD_RT:				return L"Wildblue RT";// D64 RT
		case CRD_BLUEWILD_HD:				return L"Wildblue HD";// * BadAss G2
		case CRD_REDDEVIL:					return L"Iridium Full";// Iridium Full
		case CRD_BLUEDEEP_HD:	
		case CRD_BLUEDEEP_HDS:				return L"Reserved for \"BasAss G2";// * BadAss G2 variant, proposed, reserved
		case CRD_BLUE_ENVY:					return L"Blue Envy"; // Mini Din 
		case CRD_BLUE_PRIDE:				return L"Blue Pride";//Mini Din Output 
		case CRD_BLUE_GREED:				return L"Blue Greed";
		case CRD_BLUE_INGEST:				return L"Blue Ingest";
		case CRD_BLUE_SD_DUALLINK:			return L"Blue SD Duallink";
		case CRD_BLUE_CATALYST:				return L"Blue Catalyst";
		case CRD_BLUE_SD_DUALLINK_PRO:		return L"Blue SD Duallink Pro";
		case CRD_BLUE_SD_INGEST_PRO:		return L"Blue SD Ingest pro";
		case CRD_BLUE_SD_DEEPBLUE_LITE_PRO:	return L"Blue SD Deepblue lite Pro";
		case CRD_BLUE_SD_SINGLELINK_PRO:	return L"Blue SD Singlelink Pro";
		case CRD_BLUE_SD_IRIDIUM_AV_PRO:	return L"Blue SD Iridium AV Pro";
		case CRD_BLUE_SD_FIDELITY:			return L"Blue SD Fidelity";
		case CRD_BLUE_SD_FOCUS:				return L"Blue SD Focus";
		case CRD_BLUE_SD_PRIME:				return L"Blue SD Prime";
		case CRD_BLUE_EPOCH_2K_CORE:		return L"Blue Epoch 2K Core";
		case CRD_BLUE_EPOCH_2K_ULTRA:		return L"Blue Epoch 2K Ultra";
		case CRD_BLUE_EPOCH_HORIZON:		return L"Blue Epoch Horizon";
		case CRD_BLUE_EPOCH_CORE:			return L"Blue Epoch Core";
		case CRD_BLUE_EPOCH_ULTRA:			return L"Blue Epoch Ultra";
		case CRD_BLUE_CREATE_HD:			return L"Blue Create HD";
		case CRD_BLUE_CREATE_2K:			return L"Blue Create 2K";
		case CRD_BLUE_CREATE_2K_ULTRA:		return L"Blue Create 2K Ultra";
		case CRD_BLUE_SUPER_NOVA:			return L"Blue SuperNova";
		case CRD_BLUE_SUPER_NOVA_S_PLUS:	return L"Blue SuperNova s+";
		case CRD_BLUE_NEUTRON:				return L"Blue Neutron 4k";
		case CRD_BLUE_EPOCH_CG:				return L"Blue Epopch CG";
		default:							return L"Unknown";
	}
}

EVideoMode get_video_mode(BvcWrapper& blue, const core::video_format_desc& format_desc)
{
	EVideoMode vid_fmt = VID_FMT_INVALID;
	BLUE_UINT32 inValidFmt = 0;
	BErr err = 0;
	err = blue.QueryCardProperty32(INVALID_VIDEO_MODE_FLAG, inValidFmt);
	vid_fmt = vid_fmt_from_video_format(format_desc.format);

	if (vid_fmt == VID_FMT_INVALID)
		CASPAR_THROW_EXCEPTION(not_supported() << msg_info(L"video-mode not supported: " + format_desc.name));


	if ((unsigned int)vid_fmt >= inValidFmt)
		CASPAR_THROW_EXCEPTION(not_supported() << msg_info(L"video-mode not supported - Outside of valid range: " + format_desc.name));

	return vid_fmt;
}

spl::shared_ptr<BvcWrapper> create_blue()
{
	auto pWrap = new BvcWrapper();
	if (!pWrap->IsBvcValid())
		CASPAR_THROW_EXCEPTION(not_supported() << msg_info("Bluefish drivers not found."));

	return spl::shared_ptr<BvcWrapper>(pWrap);
}

spl::shared_ptr<BvcWrapper> create_blue(int device_index)
{
	auto blue = create_blue();
	
	if(BLUE_FAIL(blue->Attach(device_index)))
		CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to attach device."));

	return blue;
}

core::video_format video_format_from_vid_fmt(EVideoMode fmt)
{
	switch (fmt)
	{
	case VID_FMT_PAL: return core::video_format::pal;
	case VID_FMT_NTSC: return core::video_format::ntsc;
	case VID_FMT_720P_2398: return core::video_format::x720p2398;
	case VID_FMT_720P_2400: return core::video_format::x720p2400;
	case VID_FMT_720P_2500: return core::video_format::x720p2500;
	case VID_FMT_720P_5000: return core::video_format::x720p5000;
	case VID_FMT_720P_2997: return core::video_format::x720p2997;
	case VID_FMT_720P_5994: return core::video_format::x720p5994;
	case VID_FMT_720P_3000: return core::video_format::x720p3000;
	case VID_FMT_720P_6000: return core::video_format::x720p6000;
	case VID_FMT_1080P_2397: return core::video_format::x1080p2398;
	case VID_FMT_1080P_2400: return core::video_format::x1080p2400;
	case VID_FMT_1080I_5000: return core::video_format::x1080i5000;
	case VID_FMT_1080I_5994: return core::video_format::x1080i5994;
	case VID_FMT_1080I_6000: return core::video_format::x1080i6000;
	case VID_FMT_1080P_2500: return core::video_format::x1080p2500;
	case VID_FMT_1080P_2997: return core::video_format::x1080p2997;
	case VID_FMT_1080P_3000: return core::video_format::x1080p3000;
	case VID_FMT_1080P_5000: return core::video_format::x1080p5000;
	case VID_FMT_1080P_5994: return core::video_format::x1080p5994;
	case VID_FMT_1080P_6000: return core::video_format::x1080p6000;
	default: return core::video_format::invalid;
	}
}

core::video_format_desc get_format_desc(BvcWrapper& blue, EVideoMode vid_fmt, EMemoryFormat mem_fmt)
{
	core::video_format_desc fmt;
	unsigned int width, height, duration = 0, time_scale = 0, rate = 0, bIs1001 = 0, bIsProgressive = 0, size = 0;
	std::vector<int>	audio_cadence;
	core::field_mode video_field_mode = core::field_mode::progressive;

	blue.GetFrameInfoForVideoVode(vid_fmt, width, height, rate, bIs1001, bIsProgressive);
	blue.GetBytesPerFrame(vid_fmt, mem_fmt, UPD_FMT_FRAME, size);

	switch (vid_fmt)
	{
	case VID_FMT_NTSC:
	case VID_FMT_1080I_5994:
		duration = 30000;
		time_scale = 1001;
		audio_cadence = { 1601,1602,1601,1602,1602 };
		video_field_mode = core::field_mode::upper;
		break;
	case VID_FMT_2048_1080P_2500:
	case VID_FMT_2048_1080PSF_2500:
	case VID_FMT_576I_5000:
	case VID_FMT_1080P_2500:
	case VID_FMT_1080I_5000:
	case VID_FMT_1080PSF_2500:
	case VID_FMT_720P_2500:
		duration = 25000;
		time_scale = 1000;
		audio_cadence = { 1920,1920,1920,1920,1920 };
		break;

	case VID_FMT_720P_5994:
	case VID_FMT_2048_1080P_5994:
	case VID_FMT_1080P_5994:
		duration = 60000;
		time_scale = 1001;
		audio_cadence = { 801,800,801,800,800 };
		break;

	case VID_FMT_1080P_6000:
	case VID_FMT_2048_1080P_6000:
	case VID_FMT_720P_6000:
		duration = 60000;
		time_scale = 1000;
		audio_cadence = { 801,800,801,800,800 };
		break;

	case VID_FMT_1080PSF_2397:
	case VID_FMT_1080P_2397:
	case VID_FMT_720P_2398:
	case VID_FMT_2048_1080PSF_2397:
	case VID_FMT_2048_1080P_2397:
		duration = 24000;
		time_scale = 1000;
		break;

	case VID_FMT_1080PSF_2400:
	case VID_FMT_1080P_2400:
	case VID_FMT_720P_2400:
	case VID_FMT_2048_1080PSF_2400:
	case VID_FMT_2048_1080P_2400:
		duration = 24000;
		time_scale = 1000;
		break;

	case VID_FMT_1080I_6000:
	case VID_FMT_1080PSF_3000:
		duration = 30000;
		time_scale = 1000;
		break;

	case VID_FMT_720P_2997:
	case VID_FMT_1080P_2997:
	case VID_FMT_2048_1080PSF_2997:
	case VID_FMT_2048_1080P_2997:
	case VID_FMT_1080PSF_2997:
		duration = 30000;
		time_scale = 1001;
		break;

	case VID_FMT_720P_3000:
	case VID_FMT_1080P_3000:
	case VID_FMT_2048_1080PSF_3000:
	case VID_FMT_2048_1080P_3000:
		duration = 30000;
		time_scale = 1001;
		break;

	case VID_FMT_720P_5000:
	case VID_FMT_1080P_5000:
	case VID_FMT_2048_1080P_5000:
		audio_cadence = { 960,960,960,960,960 };
		duration = 50000;
		time_scale = 1000;
		break;
		/*case VID_FMT_1080P_4800:
		case VID_FMT_2048_1080P_4800:
		fmt.duration = 48000;
		fmt.time_scale = 1000;
		break;*/
	}
	fmt = core::video_format_desc(video_format_from_vid_fmt(vid_fmt), width, height, width, height, video_field_mode, time_scale, duration, std::wstring(L""), audio_cadence);
	fmt.size = size;
	return fmt;
}

}}