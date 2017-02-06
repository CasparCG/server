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
#include <BlueVelvetCUtils.h>

#if defined(__APPLE__)
#include <dlfcn.h>
#endif

#if defined(_WIN32)
	#define GET_PROCADDR_FOR_FUNC(name, module) { name = (pFunc_##name)GetProcAddress(reinterpret_cast<HMODULE>(module), #name); if(!name) { return false; } }
//	#define GET_PROCADDR_FOR_FUNC(name, module) { name = (pFunc_##name)GetProcAddress(module, #name); if(!name) { return false; } }
#elif defined(__APPLE__)
	#define GET_PROCADDR_FOR_FUNC(name, module) { name = (pFunc_##name)dlsym(module, #name); if(!name) { return false; } }
#endif

namespace caspar { namespace bluefish {

	bvc_wrapper::bvc_wrapper()
	{
		bvc_ = nullptr;
		h_module_ = nullptr;
		init_function_pointers();
		bvc_ = std::shared_ptr<void>(bfcFactory(), bfcDestroy);
		
		if (!bvc_)
			CASPAR_THROW_EXCEPTION(not_supported() << msg_info("Bluefish drivers not found."));
	}

	bool bvc_wrapper::init_function_pointers()
	{
		bool res = false;
#if defined(_WIN32)
#ifdef _DEBUG
		h_module_ = std::shared_ptr<void>(LoadLibraryExA("BlueVelvetC64_d.dll", NULL, 0), FreeLibrary);
#else
		h_module_ = std::shared_ptr<void>(LoadLibraryExA("BlueVelvetC64.dll", NULL, 0), FreeLibrary);
#endif
		
#elif defined(__APPLE__)
		// Look for the framework and load it accordingly.
		char* libraryPath("/Library/Frameworks/BlueVelvetC.framework");		// full path may not be required, OSX might check in /l/f by default - MUST TEST!
		h_module_ = std::shared_ptr<void>(dlopen(libraryPath, RTLD_NOW), dlclose);
#endif
		
		if (h_module_)
		{
			GET_PROCADDR_FOR_FUNC(bfcGetVersion, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcFactory, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcDestroy, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcEnumerate, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcQueryCardType, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcAttach, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcDetach, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcQueryCardProperty32, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcQueryCardProperty64, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcSetCardProperty32, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcSetCardProperty64, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcGetCardSerialNumber, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcGetCardFwVersion, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcWaitVideoSyncAsync, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcWaitVideoInputSync, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcWaitVideoOutputSync, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcGetVideoOutputCurrentFieldCount, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcGetVideoInputCurrentFieldCount, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcVideoCaptureStart, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcVideoCaptureStop, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcVideoPlaybackStart, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcVideoPlaybackStop, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcVideoPlaybackAllocate, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcVideoPlaybackPresent, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcVideoPlaybackRelease, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcGetCaptureVideoFrameInfoEx, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcRenderBufferCapture, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcRenderBufferUpdate, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcGetRenderBufferCount, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcEncodeHancFrameEx, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcDecodeHancFrameEx, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcSystemBufferReadAsync, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcSystemBufferWriteAsync, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcGetBytesForGroupPixels, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcGetPixelsPerLine, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcGetLinesPerFrame, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcGetBytesPerLine, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcGetBytesPerFrame, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcGetGoldenValue, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcGetVBILines, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcGetVANCGoldenValue, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcGetVANCLineBytes, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcGetVANCLineCount, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcGetWindowsDriverHandle, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetStringForCardType, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetStringForBlueProductId, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetStringForVideoMode, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetStringForMemoryFormat, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetMR2Routing, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcUtilsSetMR2Routing, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetAudioOutputRouting, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcUtilsSetAudioOutputRouting, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcUtilsIsVideoModeProgressive, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcUtilsIsVideoMode1001Framerate, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetFpsForVideoMode, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetVideoModeForFrameInfo, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetFrameInfoForVideoMode, h_module_.get());
			GET_PROCADDR_FOR_FUNC(bfcUtilsGetAudioSamplesPerFrame, h_module_.get());
			res = true;
		}
		return res;
	}

	const char * bvc_wrapper::get_version()
	{
		return bfcGetVersion();
	}

	BLUE_UINT32 bvc_wrapper::attach(int iDeviceId)
	{
		return bfcAttach((BLUEVELVETC_HANDLE)bvc_.get(), iDeviceId);
	}

	BLUE_UINT32 bvc_wrapper::detach()
	{
		// disable the audio on detach
		unsigned int audio_value = 0;
		bfcQueryCardProperty32((BLUEVELVETC_HANDLE)bvc_.get(), EMBEDEDDED_AUDIO_OUTPUT, audio_value);
		return bfcDetach((BLUEVELVETC_HANDLE)bvc_.get());
	}

	BLUE_UINT32 bvc_wrapper::get_card_property32(const int iProperty, unsigned int & nValue)
	{
		if (bvc_)
			return (BLUE_UINT32)bfcQueryCardProperty32((BLUEVELVETC_HANDLE)bvc_.get(), iProperty, nValue);
		else
			return 0;
	}

	BLUE_UINT32 bvc_wrapper::set_card_property32(const int iProperty, const unsigned int nValue)
	{
		return bfcSetCardProperty32((BLUEVELVETC_HANDLE)bvc_.get(), iProperty, nValue);
	}

	BLUE_UINT32 bvc_wrapper::enumerate(int & iDevices)
	{
		return bfcEnumerate((BLUEVELVETC_HANDLE)bvc_.get(), iDevices);
	}

	BLUE_UINT32 bvc_wrapper::query_card_type(int & iCardType, int iDeviceID)
	{
		return bfcQueryCardType((BLUEVELVETC_HANDLE)bvc_.get(), iCardType, iDeviceID);
	}

	BLUE_UINT32 bvc_wrapper::system_buffer_write(unsigned char * pPixels, unsigned long ulSize, unsigned long ulBufferID, unsigned long ulOffset)
	{
		return bfcSystemBufferWriteAsync((BLUEVELVETC_HANDLE)bvc_.get(), pPixels, ulSize, nullptr, ulBufferID, ulOffset);
	}

	BLUE_UINT32 bvc_wrapper::system_buffer_read(unsigned char* pPixels, unsigned long ulSize, unsigned long ulBufferID, unsigned long ulOffset)
	{
		return bfcSystemBufferReadAsync((BLUEVELVETC_HANDLE)bvc_.get(), pPixels, ulSize, nullptr, ulBufferID, ulOffset);
	}

	BLUE_UINT32 bvc_wrapper::video_playback_stop(int iWait, int iFlush)
	{
		return bfcVideoPlaybackStop((BLUEVELVETC_HANDLE)bvc_.get(), iWait, iFlush);
	}

	BLUE_UINT32 bvc_wrapper::wait_video_output_sync(unsigned long ulUpdateType, unsigned long & ulFieldCount)
	{
		return bfcWaitVideoOutputSync((BLUEVELVETC_HANDLE)bvc_.get(), ulUpdateType, ulFieldCount);
	}

	BLUE_UINT32 bvc_wrapper::wait_video_input_sync(unsigned long ulUpdateType, unsigned long & ulFieldCount)
	{
		return bfcWaitVideoInputSync((BLUEVELVETC_HANDLE)bvc_.get(), ulUpdateType, ulFieldCount);
	}

	BLUE_UINT32 bvc_wrapper::render_buffer_update( unsigned long ulBufferID)
	{
		return bfcRenderBufferUpdate((BLUEVELVETC_HANDLE)bvc_.get(), ulBufferID);
	}

	BLUE_UINT32 bvc_wrapper::render_buffer_capture(unsigned long ulBufferID)
	{
		return bfcRenderBufferCapture((BLUEVELVETC_HANDLE)bvc_.get(), ulBufferID);
	}

	BLUE_UINT32 bvc_wrapper::encode_hanc_frame(unsigned int nCardType, hanc_stream_info_struct * pHancEncodeInfo, void * pAudioBuffer, unsigned int nAudioChannels, unsigned int nAudioSamples, unsigned int nSampleType, unsigned int nAudioFlags)
	{
		return bfcEncodeHancFrameEx((BLUEVELVETC_HANDLE)bvc_.get(), CRD_BLUE_NEUTRON, pHancEncodeInfo, pAudioBuffer, nAudioChannels, nAudioSamples, nSampleType, nAudioFlags);
	}

	BLUE_UINT32 bvc_wrapper::decode_hanc_frame(unsigned int nCardType, unsigned int * pHancBuffer, hanc_decode_struct * pHancDecodeInfo)
	{
		return bfcDecodeHancFrameEx((BLUEVELVETC_HANDLE)bvc_.get(), CRD_BLUE_NEUTRON, pHancBuffer, pHancDecodeInfo);
	}

	BLUE_UINT32 bvc_wrapper::get_frame_info_for_video_mode(const unsigned int nVideoMode, unsigned int&  nWidth, unsigned int& nHeight, unsigned int& nRate, unsigned int& bIs1001, unsigned int& bIsProgressive)
	{
		return bfcUtilsGetFrameInfoForVideoMode(nVideoMode, nWidth, nHeight, nRate, bIs1001, bIsProgressive);
	}

	BLUE_UINT32 bvc_wrapper::get_bytes_per_frame(EVideoMode nVideoMode, EMemoryFormat nMemoryFormat, EUpdateMethod nUpdateMethod, unsigned int& nBytesPerFrame)
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

bool is_epoch_card(bvc_wrapper& blue)
{
	int device_id = 1;
	int card_type = 0;
	blue.query_card_type(card_type, device_id);

	switch(card_type)
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

bool is_epoch_neutron_1i2o_card(bvc_wrapper& blue)
{
	BLUE_UINT32 val = 0;
	blue.get_card_property32(EPOCH_GET_PRODUCT_ID, val);
	if (val == ORAC_NEUTRON_1_IN_2_OUT_FIRMWARE_PRODUCTID)
		return true;
	else
		return false;
}

bool is_epoch_neutron_3o_card(bvc_wrapper& blue)
{
	BLUE_UINT32 val = 0;
	blue.get_card_property32(EPOCH_GET_PRODUCT_ID, val);

	if (val == ORAC_NEUTRON_0_IN_3_OUT_FIRMWARE_PRODUCTID)
		return true;
	else
		return false;
}

std::wstring get_card_desc(bvc_wrapper& blue, int device_id)
{
	int card_type = 0;
	blue.query_card_type(card_type, device_id);

	switch(card_type)
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

EVideoMode get_video_mode(bvc_wrapper& blue, const core::video_format_desc& format_desc)
{
	EVideoMode vid_fmt = VID_FMT_INVALID;
	BLUE_UINT32 invalid_fmt = 0;
	BErr err = 0;
	err = blue.get_card_property32(INVALID_VIDEO_MODE_FLAG, invalid_fmt);
	vid_fmt = vid_fmt_from_video_format(format_desc.format);

	if (vid_fmt == VID_FMT_INVALID)
		CASPAR_THROW_EXCEPTION(not_supported() << msg_info(L"video-mode not supported: " + format_desc.name));


	if ((unsigned int)vid_fmt >= invalid_fmt)
		CASPAR_THROW_EXCEPTION(not_supported() << msg_info(L"video-mode not supported - Outside of valid range: " + format_desc.name));

	return vid_fmt;
}

spl::shared_ptr<bvc_wrapper> create_blue()
{
	auto pWrap = new bvc_wrapper();
	return spl::shared_ptr<bvc_wrapper>(pWrap);
}

spl::shared_ptr<bvc_wrapper> create_blue(int device_index)
{
	auto blue = create_blue();
	
	if(BLUE_FAIL(blue->attach(device_index)))
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

core::video_format_desc get_format_desc(bvc_wrapper& blue, EVideoMode vid_fmt, EMemoryFormat mem_fmt)
{
	core::video_format_desc fmt;
	unsigned int width, height, duration = 0, time_scale = 0, rate = 0, bIs1001 = 0, bIsProgressive = 0, size = 0;
	std::vector<int>	audio_cadence;
	core::field_mode video_field_mode = core::field_mode::progressive;

	blue.get_frame_info_for_video_mode(vid_fmt, width, height, rate, bIs1001, bIsProgressive);
	blue.get_bytes_per_frame(vid_fmt, mem_fmt, UPD_FMT_FRAME, size);

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