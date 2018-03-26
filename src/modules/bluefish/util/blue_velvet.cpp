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
#include "../interop/BlueVelvetCUtils.h"

#if defined(__APPLE__)
#include <dlfcn.h>
#endif

#if defined(_WIN32)
	#define GET_PROCADDR_FOR_FUNC(name, module) { name = (pFunc_##name)GetProcAddress(reinterpret_cast<HMODULE>(module), #name); if(!name) { return false; } }
#elif defined(__APPLE__)
	#define GET_PROCADDR_FOR_FUNC(name, module) { name = (pFunc_##name)dlsym(module, #name); if(!name) { return false; } }
#endif

namespace caspar { namespace bluefish {

	bvc_wrapper::bvc_wrapper()
	{
		if(!init_function_pointers())
			CASPAR_THROW_EXCEPTION(not_supported() << msg_info("Bluefish drivers not found.  Unable to init Funcion Pointers"));
		
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
		return bfcAttach(bvc_.get(), iDeviceId);
	}

	BLUE_UINT32 bvc_wrapper::detach()
	{
		return bfcDetach(bvc_.get());
	}

	BLUE_UINT32 bvc_wrapper::get_card_property32(const int iProperty, unsigned int & nValue)
	{
		return (BLUE_UINT32)bfcQueryCardProperty32(bvc_.get(), iProperty, nValue);
	}

	BLUE_UINT32 bvc_wrapper::set_card_property32(const int iProperty, const unsigned int nValue)
	{
		return bfcSetCardProperty32(bvc_.get(), iProperty, nValue);
	}

    BLUE_UINT32 bvc_wrapper::get_card_property64(const int iProperty, unsigned long long& nValue)
    {
        return (BLUE_UINT32)bfcQueryCardProperty64(bvc_.get(), iProperty, nValue);
    }

    BLUE_UINT32 bvc_wrapper::set_card_property64(const int iProperty, const unsigned long long nValue)
    {
        return bfcSetCardProperty64(bvc_.get(), iProperty, nValue);
    }

	BLUE_UINT32 bvc_wrapper::enumerate(int & iDevices)
	{
		return bfcEnumerate(bvc_.get(), iDevices);
	}

	BLUE_UINT32 bvc_wrapper::query_card_type(int & iCardType, int iDeviceID)
	{
		return bfcQueryCardType(bvc_.get(), iCardType, iDeviceID);
	}

	BLUE_UINT32 bvc_wrapper::system_buffer_write(unsigned char * pPixels, unsigned long ulSize, unsigned long ulBufferID, unsigned long ulOffset)
	{
		return bfcSystemBufferWriteAsync(bvc_.get(), pPixels, ulSize, nullptr, ulBufferID, ulOffset);
	}

	BLUE_UINT32 bvc_wrapper::system_buffer_read(unsigned char* pPixels, unsigned long ulSize, unsigned long ulBufferID, unsigned long ulOffset)
	{
		return bfcSystemBufferReadAsync(bvc_.get(), pPixels, ulSize, nullptr, ulBufferID, ulOffset);
	}

	BLUE_UINT32 bvc_wrapper::video_playback_stop(int iWait, int iFlush)
	{
		return bfcVideoPlaybackStop(bvc_.get(), iWait, iFlush);
	}

	BLUE_UINT32 bvc_wrapper::video_playback_start(int step, int loop)
	{
		return bfcVideoPlaybackStart(bvc_.get(), step, loop);
	}

	BLUE_UINT32 bvc_wrapper::video_playback_allocate(unsigned long& buffer_id, unsigned long& underrun)
	{
		void* unused = nullptr;
		return bfcVideoPlaybackAllocate(bvc_.get(), &unused, buffer_id, underrun);
	}

	BLUE_UINT32 bvc_wrapper::video_playback_present(unsigned long buffer_id, unsigned long count, unsigned long keep, unsigned long odd)
	{
		unsigned long unique_id;
		return bfcVideoPlaybackPresent(bvc_.get(), unique_id, buffer_id, count, keep, odd);
	}

	BLUE_UINT32 bvc_wrapper::wait_video_output_sync(unsigned long ulUpdateType, unsigned long & ulFieldCount)
	{
		return bfcWaitVideoOutputSync(bvc_.get(), ulUpdateType, ulFieldCount);
	}

	BLUE_UINT32 bvc_wrapper::wait_video_input_sync(unsigned long ulUpdateType, unsigned long & ulFieldCount)
	{
		return bfcWaitVideoInputSync(bvc_.get(), ulUpdateType, ulFieldCount);
	}

	BLUE_UINT32 bvc_wrapper::render_buffer_update( unsigned long ulBufferID)
	{
		return bfcRenderBufferUpdate(bvc_.get(), ulBufferID);
	}

	BLUE_UINT32 bvc_wrapper::render_buffer_capture(unsigned long ulBufferID)
	{
		return bfcRenderBufferCapture(bvc_.get(), ulBufferID);
	}

	BLUE_UINT32 bvc_wrapper::encode_hanc_frame(unsigned int nCardType, hanc_stream_info_struct * pHancEncodeInfo, void * pAudioBuffer, unsigned int nAudioChannels, unsigned int nAudioSamples, unsigned int nSampleType, unsigned int nAudioFlags)
	{
		return bfcEncodeHancFrameEx(bvc_.get(), CRD_BLUE_NEUTRON, pHancEncodeInfo, pAudioBuffer, nAudioChannels, nAudioSamples, nSampleType, nAudioFlags);
	}

	BLUE_UINT32 bvc_wrapper::decode_hanc_frame(unsigned int nCardType, unsigned int * pHancBuffer, hanc_decode_struct * pHancDecodeInfo)
	{
		return bfcDecodeHancFrameEx(bvc_.get(), CRD_BLUE_NEUTRON, pHancBuffer, pHancDecodeInfo);
	}

	BLUE_UINT32 bvc_wrapper::get_frame_info_for_video_mode(const unsigned int nVideoMode, unsigned int&  nWidth, unsigned int& nHeight, unsigned int& nRate, unsigned int& bIs1001, unsigned int& bIsProgressive)
	{
		return bfcUtilsGetFrameInfoForVideoMode(nVideoMode, nWidth, nHeight, nRate, bIs1001, bIsProgressive);
	}

	BLUE_UINT32 bvc_wrapper::get_bytes_per_frame(EVideoMode nVideoMode, EMemoryFormat nMemoryFormat, EUpdateMethod nUpdateMethod, unsigned int& nBytesPerFrame)
	{
        return bfcGetGoldenValue(nVideoMode, nMemoryFormat, nUpdateMethod, nBytesPerFrame);
	}

    std::string bvc_wrapper::get_string_for_card_type(unsigned int nCardType)
    {
        return bfcUtilsGetStringForCardType(nCardType);
    }

    std::wstring bvc_wrapper::get_wstring_for_video_mode(unsigned int nVideoMode)
    {
        std::wstring mode_desc;
        switch (nVideoMode)
        {
        case  VID_FMT_PAL:	        mode_desc = L"pal";
            break;
        case  VID_FMT_NTSC:     	mode_desc = L"ntsc";
            break;
        case  VID_FMT_720P_2398:	mode_desc = L"720p23";
            break;
        case  VID_FMT_720P_2400:	mode_desc = L"720p24";
            break;
        case  VID_FMT_720P_2500:	mode_desc = L"720p25";
            break;
        case  VID_FMT_720P_5000:	mode_desc = L"720p50";
            break;
        case  VID_FMT_720P_2997:	mode_desc = L"720p29";
            break;
        case  VID_FMT_720P_5994:	mode_desc = L"720p59";
            break;
        case  VID_FMT_720P_3000:	mode_desc = L"720p30";
            break;
        case  VID_FMT_720P_6000:	mode_desc = L"720p60";
            break;
        case  VID_FMT_1080P_2397:	mode_desc = L"1080p23";
            break;
        case  VID_FMT_1080P_2400:	mode_desc = L"1080p24";
            break;
        case  VID_FMT_1080I_5000:	mode_desc = L"1080i50";
            break;
        case  VID_FMT_1080I_5994:	mode_desc = L"1080i59";
            break;
        case  VID_FMT_1080I_6000:	mode_desc = L"1080i60";
            break;
        case  VID_FMT_1080P_2500:	mode_desc = L"1080p25";
            break;
        case  VID_FMT_1080P_2997:	mode_desc = L"1080p29";
            break;
        case  VID_FMT_1080P_3000:	mode_desc = L"1080p30";
            break;
        case  VID_FMT_1080P_5000:	mode_desc = L"1080p50";
            break;
        case  VID_FMT_1080P_5994:	mode_desc = L"1080p59";
            break;
        case  VID_FMT_1080P_6000:	mode_desc = L"1080p60";
            break;
        default:					mode_desc = L"invalid";
            break;
        }
        return mode_desc;
    }

  int  bvc_wrapper::get_num_audio_samples_for_frame(const BLUE_UINT32 nVideoMode, const BLUE_UINT32 nFrameNo)
  {
    return bfcUtilsGetAudioSamplesPerFrame(nVideoMode, nFrameNo);
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

std::wstring get_card_desc(bvc_wrapper blue, int device_id)
{
  std::wstring card_desc;
  int card_type = 0;
  blue.query_card_type(card_type, device_id);

  switch (card_type)
  {
  case CRD_BLUE_EPOCH_2K_CORE:		    card_desc = L"Bluefish Epoch 2K Core";
      break;
  case CRD_BLUE_EPOCH_2K_ULTRA:		    card_desc = L"Bluefish Epoch 2K Ultra";
      break;
  case CRD_BLUE_EPOCH_HORIZON:		    card_desc = L"Bluefish Epoch Horizon";
      break;
  case CRD_BLUE_EPOCH_CORE:			    card_desc = L"Blufishe Epoch Core";
      break;
  case CRD_BLUE_EPOCH_ULTRA:			card_desc = L"Bluefish Epoch Ultra";
      break;
  case CRD_BLUE_CREATE_HD:			    card_desc = L"Bluefish Create HD";
      break;
  case CRD_BLUE_CREATE_2K:			    card_desc = L"Bluefish Create 2K";
      break;
  case CRD_BLUE_CREATE_2K_ULTRA:		card_desc = L"Bluefish Create 2K Ultra";
      break;
  case CRD_BLUE_SUPER_NOVA:			    card_desc = L"Bluefish SuperNova";
      break;
  case CRD_BLUE_SUPER_NOVA_S_PLUS:	    card_desc = L"Bluefish SuperNova s+";
      break;
  case CRD_BLUE_NEUTRON:				card_desc = L"Bluefish Neutron 4k";
      break;
  case CRD_BLUE_EPOCH_CG:				card_desc = L"Bluefish Epoch CG";
      break;
  default:							    card_desc = L"Unknown";
  }
  return card_desc;
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

core::video_format_desc get_format_desc(bvc_wrapper& blue, EVideoMode vid_fmt, EMemoryFormat mem_fmt)
{
	core::video_format_desc fmt;
	unsigned int width, height, duration = 0, time_scale = 0, rate = 0, bIs1001 = 0, is_progressive = 0, size = 0;
	std::vector<int>	audio_cadence;
    int field_count = 1;

	blue.get_frame_info_for_video_mode(vid_fmt, width, height, rate, bIs1001, is_progressive);
	blue.get_bytes_per_frame(vid_fmt, mem_fmt, UPD_FMT_FRAME, size);

	switch (vid_fmt)
	{
	case VID_FMT_NTSC:
	case VID_FMT_1080I_5994:
		duration = 30000;
		time_scale = 1001;
		audio_cadence = { 1601,1602,1601,1602,1602 };
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
	}

    if (!is_progressive)
        field_count = 2;

    fmt.field_count = field_count;
    fmt = get_caspar_video_format(vid_fmt);
    fmt.size = size;
    fmt.audio_cadence = audio_cadence;
    fmt.name = blue.get_wstring_for_video_mode(vid_fmt);

	return fmt;
}

std::wstring get_sdi_inputs(bvc_wrapper& blue)
{
    BLUE_UINT32 val = 0;
    blue.get_card_property32(CARD_FEATURE_CONNECTOR_INFO, val);
    int connectors = CARD_FEATURE_GET_SDI_INPUT_CONNECTOR_COUNT(val);
    return  std::to_wstring(connectors);
}

std::wstring get_sdi_outputs(bvc_wrapper& blue)
{
    BLUE_UINT32 val = 0;
    blue.get_card_property32(CARD_FEATURE_CONNECTOR_INFO, val);
    int connectors = CARD_FEATURE_GET_SDI_OUTPUT_CONNECTOR_COUNT(val);
    return  std::to_wstring(connectors);
}

EVideoMode get_bluefish_video_format(core::video_format fmt)
{
  switch (fmt)
  {
    case core::video_format::pal:
      return VID_FMT_PAL;
    case core::video_format::ntsc:
      return VID_FMT_NTSC;
    case core::video_format::x720p2398:
      return VID_FMT_720P_2398;
    case core::video_format::x720p2400:
      return VID_FMT_720P_2400;
    case core::video_format::x720p2500:
      return VID_FMT_720P_2500;
    case core::video_format::x720p2997:
      return VID_FMT_720P_2997;
    case core::video_format::x720p3000:
      return VID_FMT_720P_3000;
    case core::video_format::x720p5000:
      return VID_FMT_720P_5000;
    case core::video_format::x720p5994:
      return VID_FMT_720P_5994;
    case core::video_format::x720p6000:
      return VID_FMT_720P_6000;

    case core::video_format::x1080i5000:
      return VID_FMT_1080I_5000;
    case core::video_format::x1080i5994:
      return VID_FMT_1080I_5994;
    case core::video_format::x1080i6000:
      return VID_FMT_1080I_6000;
    case core::video_format::x1080p2398:
      return VID_FMT_1080P_2397;
    case core::video_format::x1080p2400:
      return VID_FMT_1080P_2400;
    case core::video_format::x1080p2500:
      return VID_FMT_1080P_2500;
    case core::video_format::x1080p2997:
      return VID_FMT_1080P_2997;
    case core::video_format::x1080p3000:
      return VID_FMT_1080P_3000;
    case core::video_format::x1080p5000:
      return VID_FMT_1080P_5000;
    case core::video_format::x1080p5994:
      return VID_FMT_1080P_5994;
    case core::video_format::x1080p6000:
      return VID_FMT_1080P_6000;
    default:
      return VID_FMT_INVALID;
  }
}

static core::video_format get_caspar_video_format(EVideoMode fmt)
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

}}
