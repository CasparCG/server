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

#pragma once

#include <Windows.h>

#include <BlueVelvetC.h>

#include <common/memory.h>
#include <common/except.h>

#include <core/fwd.h>

namespace caspar { namespace bluefish {

class bvc_wrapper
{
	// Define a different type for each of the function ptrs.
	typedef const char* (__cdecl *pFunc_bfcGetVersion)();
	typedef BLUEVELVETC_HANDLE(__cdecl *pFunc_bfcFactory)();
	typedef void(__cdecl *pFunc_bfcDestroy)(BLUEVELVETC_HANDLE pHandle);
	typedef int(__cdecl *pFunc_bfcEnumerate)(BLUEVELVETC_HANDLE pHandle, int& iDevices);
	typedef int(__cdecl *pFunc_bfcQueryCardType)(BLUEVELVETC_HANDLE pHandle, int& iCardType, int iDeviceID);
	typedef int(__cdecl *pFunc_bfcAttach)(BLUEVELVETC_HANDLE pHandle, int iDeviceId);
	typedef int(__cdecl *pFunc_bfcDetach)(BLUEVELVETC_HANDLE pHandle);
	typedef int(__cdecl *pFunc_bfcQueryCardProperty32)(BLUEVELVETC_HANDLE pHandle, const int iProperty, unsigned int& nValue);
	typedef int(__cdecl *pFunc_bfcQueryCardProperty64)(BLUEVELVETC_HANDLE pHandle, const int iProperty, unsigned long long& ullValue);
	typedef int(__cdecl *pFunc_bfcSetCardProperty32)(BLUEVELVETC_HANDLE pHandle, const int iProperty, const unsigned int nValue);
	typedef int(__cdecl *pFunc_bfcSetCardProperty64)(BLUEVELVETC_HANDLE pHandle, const int iProperty, const unsigned long long ullValue);
	typedef int(__cdecl *pFunc_bfcGetCardSerialNumber)(BLUEVELVETC_HANDLE pHandle, char* pSerialNumber, unsigned int nStringSize);
	typedef int(__cdecl *pFunc_bfcGetCardFwVersion)(BLUEVELVETC_HANDLE pHandle, unsigned int& nValue);
	typedef int(__cdecl *pFunc_bfcWaitVideoSyncAsync)(BLUEVELVETC_HANDLE pHandle, OVERLAPPED* pOverlap, blue_video_sync_struct* pSyncData);
	typedef int(__cdecl *pFunc_bfcWaitVideoInputSync)(BLUEVELVETC_HANDLE pHandle, unsigned long ulUpdateType, unsigned long& ulFieldCount);
	typedef int(__cdecl *pFunc_bfcWaitVideoOutputSync)(BLUEVELVETC_HANDLE pHandle, unsigned long ulUpdateType, unsigned long& ulFieldCount);
	typedef int(__cdecl *pFunc_bfcGetVideoOutputCurrentFieldCount)(BLUEVELVETC_HANDLE pHandle, unsigned long& ulFieldCount);
	typedef int(__cdecl *pFunc_bfcGetVideoInputCurrentFieldCount)(BLUEVELVETC_HANDLE pHandle, unsigned long& ulFieldCount);
	typedef int(__cdecl *pFunc_bfcVideoCaptureStart)(BLUEVELVETC_HANDLE pHandle);
	typedef int(__cdecl *pFunc_bfcVideoCaptureStop)(BLUEVELVETC_HANDLE pHandle);
	typedef int(__cdecl *pFunc_bfcVideoPlaybackStart)(BLUEVELVETC_HANDLE pHandle, int iStep, int iLoop);
	typedef int(__cdecl *pFunc_bfcVideoPlaybackStop)(BLUEVELVETC_HANDLE pHandle, int iWait, int iFlush);
	typedef int(__cdecl *pFunc_bfcVideoPlaybackAllocate)(BLUEVELVETC_HANDLE pHandle, void** pAddress, unsigned long& ulBufferID, unsigned long& ulUnderrun);
	typedef int(__cdecl *pFunc_bfcVideoPlaybackPresent)(BLUEVELVETC_HANDLE pHandle, unsigned long& ulUniqueID, unsigned long ulBufferID, unsigned long ulCount, int iKeep, int iOdd);
	typedef int(__cdecl *pFunc_bfcVideoPlaybackRelease)(BLUEVELVETC_HANDLE pHandle, unsigned long ulBufferID);
	typedef int(__cdecl *pFunc_bfcGetCaptureVideoFrameInfoEx)(BLUEVELVETC_HANDLE pHandle, OVERLAPPED* pOverlap, struct blue_videoframe_info_ex& VideoFrameInfo, int iCompostLater, unsigned int* nCaptureFifoSize);
	typedef int(__cdecl *pFunc_bfcRenderBufferCapture)(BLUEVELVETC_HANDLE pHandle, unsigned long ulBufferID);
	typedef int(__cdecl *pFunc_bfcRenderBufferUpdate)(BLUEVELVETC_HANDLE pHandle, unsigned long ulBufferID);
	typedef int(__cdecl *pFunc_bfcGetRenderBufferCount)(BLUEVELVETC_HANDLE pHandle, unsigned long& ulCount);
	typedef int(__cdecl *pFunc_bfcEncodeHancFrameEx)(BLUEVELVETC_HANDLE pHandle, unsigned int nCardType, struct hanc_stream_info_struct* pHancEncodeInfo, void* pAudioBuffer, unsigned int nAudioChannels, unsigned int nAudioSamples, unsigned int nSampleType, unsigned int nAudioFlags);
	typedef int(__cdecl *pFunc_bfcDecodeHancFrameEx)(BLUEVELVETC_HANDLE pHandle, unsigned int nCardType, unsigned int* pHancBuffer, struct hanc_decode_struct* pHancDecodeInfo);
#if defined(_WIN32)
	typedef int(__cdecl *pFunc_bfcSystemBufferReadAsync)(BLUEVELVETC_HANDLE pHandle, unsigned char* pPixels, unsigned long ulSize, OVERLAPPED* pOverlap, unsigned long ulBufferID, unsigned long ulOffset);
	typedef int(__cdecl *pFunc_bfcSystemBufferWriteAsync)(BLUEVELVETC_HANDLE pHandle, unsigned char* pPixels, unsigned long ulSize, OVERLAPPED* pOverlap, unsigned long ulBufferID, unsigned long ulOffset);
#else
	typedef int(__cdecl *pFunc_bfcSystemBufferRead)(BLUEVELVETC_HANDLE pHandle, unsigned char* pPixels, unsigned long ulSize, unsigned long ulBufferID, unsigned long ulOffset;
	typedef int(__cdecl *pFunc_bfcSystemBufferWrite)(BLUEVELVETC_HANDLE pHandle, unsigned char* pPixels, unsigned long ulSize, unsigned long ulBufferID, unsigned long ulOffset;
#endif
	typedef int(__cdecl *pFunc_bfcGetBytesForGroupPixels)(EMemoryFormat nMemoryFormat, unsigned int nVideoWidth, unsigned int& nVideoPitchBytes);
	typedef int(__cdecl *pFunc_bfcGetPixelsPerLine)(EVideoMode nVideoMode, unsigned int& nPixelsPerLine);
	typedef int(__cdecl *pFunc_bfcGetLinesPerFrame)(EVideoMode nVideoMode, EUpdateMethod nUpdateMethod, unsigned int& nLinesPerFrame);
	typedef int(__cdecl *pFunc_bfcGetBytesPerLine)(EVideoMode nVideoMode, EMemoryFormat nMemoryFormat, unsigned int& nBytesPerLine);
	typedef int(__cdecl *pFunc_bfcGetBytesPerFrame)(EVideoMode nVideoMode, EMemoryFormat nMemoryFormat, EUpdateMethod nUpdateMethod, unsigned int& nBytesPerFrame);
	typedef int(__cdecl *pFunc_bfcGetGoldenValue)(EVideoMode nVideoMode, EMemoryFormat nMemoryFormat, EUpdateMethod nUpdateMethod, unsigned int& nGoldenFrameSize);
	typedef int(__cdecl *pFunc_bfcGetVBILines)(EVideoMode nVideoMode, EDMADataType nDataType, unsigned int& nVBILinesPerFrame);
	typedef int(__cdecl *pFunc_bfcGetVANCGoldenValue)(unsigned int nCardType, EVideoMode nVideoMode, EMemoryFormat nMemoryFormat, EDMADataType nDataFormat, unsigned int& nVANCGoldenValue);
	typedef int(__cdecl *pFunc_bfcGetVANCLineBytes)(unsigned int nCardType, EVideoMode nVideoMode, EMemoryFormat nMemoryFormat, unsigned int& nVANCLineBytes);
	typedef int(__cdecl *pFunc_bfcGetVANCLineCount)(unsigned int nCardType, EVideoMode nVideoMode, EDMADataType nDataFormat, unsigned int& nVANCLineCount);
	typedef int(__cdecl *pFunc_bfcGetWindowsDriverHandle)(BLUEVELVETC_HANDLE pHandle, HANDLE* pDriverHandle);
	typedef int(__cdecl *pFunc_bfcSetDynamicMemoryFormatChange)(BLUEVELVETC_HANDLE pHandle, OVERLAPPED* pOverlap, unsigned int nUniqueId, EMemoryFormat nMemoryFormat);

	//BlueVelvetC utils functions
	typedef char*(__cdecl *pFunc_bfcUtilsGetStringForCardType)(const int iCardType);
	typedef char*(__cdecl *pFunc_bfcUtilsGetStringForBlueProductId)(const unsigned int nProductId);
	typedef char*(__cdecl *pFunc_bfcUtilsGetStringForVideoMode)(const unsigned int nVideoMode);
	typedef char*(__cdecl *pFunc_bfcUtilsGetStringForMemoryFormat)(const unsigned int nMemoryFormat);
	typedef int(__cdecl *pFunc_bfcUtilsGetMR2Routing)(const BLUEVELVETC_HANDLE pHandle, unsigned int& nSrcNode, const unsigned int nDestNode, unsigned int& nLinkType);
	typedef int(__cdecl *pFunc_bfcUtilsSetMR2Routing)(const BLUEVELVETC_HANDLE pHandle, const unsigned int nSrcNode, const unsigned int nDestNode, const unsigned int nLinkType);
	typedef int(__cdecl *pFunc_bfcUtilsGetAudioOutputRouting)(const BLUEVELVETC_HANDLE pHandle, const unsigned int nAudioConnectorType, unsigned int& nAudioSourceChannelId, unsigned int nAudioConnectorId);
	typedef int(__cdecl *pFunc_bfcUtilsSetAudioOutputRouting)(const BLUEVELVETC_HANDLE pHandle, const unsigned int nAudioConnectorType, unsigned int nAudioSourceChannelId, unsigned int nAudioConnectorId);
	typedef bool(__cdecl *pFunc_bfcUtilsIsVideoModeProgressive)(const unsigned int nVideoMode);
	typedef bool(__cdecl *pFunc_bfcUtilsIsVideoMode1001Framerate)(const unsigned int nVideoMode);
	typedef int(__cdecl *pFunc_bfcUtilsGetFpsForVideoMode)(const unsigned int nVideoMode);
	typedef int(__cdecl *pFunc_bfcUtilsGetVideoModeForFrameInfo)(const BLUE_UINT32 nWidth, const BLUE_UINT32 nHeight, const BLUE_UINT32 nRate, const BLUE_UINT32 bIs1001, const BLUE_UINT32 bIsProgressive, BLUE_UINT32& nVideoMode);
	typedef int(__cdecl *pFunc_bfcUtilsGetFrameInfoForVideoMode)(const BLUE_UINT32 nVideoMode, BLUE_UINT32&  nWidth, BLUE_UINT32& nHeight, BLUE_UINT32& nRate, BLUE_UINT32& bIs1001, BLUE_UINT32& bIsProgressive);
	typedef int(__cdecl *pFunc_bfcUtilsGetAudioSamplesPerFrame)(const BLUE_UINT32 nVideoMode, const BLUE_UINT32 nFrameNo);


public:

	bvc_wrapper();						// bfcFactory + function pointer lookups

	const char*	get_version();

	BLUE_UINT32 enumerate(int& iDevices);
	BLUE_UINT32 query_card_type(int& iCardType, int iDeviceID);

	BLUE_UINT32 attach(int iDeviceId);
	BLUE_UINT32 detach();

	BLUE_UINT32 get_card_property32(const int iProperty, unsigned int& nValue);
	BLUE_UINT32 set_card_property32(const int iProperty, const unsigned int nValue);
	
	BLUE_UINT32 system_buffer_write(unsigned char* pPixels, unsigned long ulSize, unsigned long ulBufferID, unsigned long ulOffset);
	BLUE_UINT32 system_buffer_read(unsigned char* pPixels, unsigned long ulSize, unsigned long ulBufferID, unsigned long ulOffset);

	BLUE_UINT32 video_playback_stop(int iWait, int iFlush);
	BLUE_UINT32 video_playback_start(int wait, int loop);
	BLUE_UINT32 video_playback_allocate(unsigned long& buffer_id, unsigned long& underrun);
	BLUE_UINT32 video_playback_present(unsigned long buffer_id, unsigned long count, unsigned long keep, unsigned long odd);

	BLUE_UINT32 wait_video_output_sync(unsigned long ulUpdateType, unsigned long& ulFieldCount);
	BLUE_UINT32 wait_video_input_sync(unsigned long ulUpdateType, unsigned long & ulFieldCount);

	BLUE_UINT32 render_buffer_update(unsigned long ulBufferID);
	BLUE_UINT32 render_buffer_capture(unsigned long ulBufferID);

	BLUE_UINT32 encode_hanc_frame(unsigned int nCardType, struct hanc_stream_info_struct* pHancEncodeInfo, void* pAudioBuffer, unsigned int nAudioChannels, unsigned int nAudioSamples, unsigned int nSampleType, unsigned int nAudioFlags);
	BLUE_UINT32 decode_hanc_frame(unsigned int nCardType, unsigned int* pHancBuffer, struct hanc_decode_struct* pHancDecodeInfo);

	BLUE_UINT32 get_frame_info_for_video_mode(const unsigned int nVideoMode, unsigned int&  nWidth, unsigned int& nHeight, unsigned int& nRate, unsigned int& bIs1001, unsigned int& bIsProgressive);
	BLUE_UINT32 get_bytes_per_frame(EVideoMode nVideoMode, EMemoryFormat nMemoryFormat, EUpdateMethod nUpdateMethod, unsigned int& nBytesPerFrame);

private:
	bool					init_function_pointers();
	
	std::shared_ptr<void>	h_module_;
	std::shared_ptr<void>	bvc_;

//BlueVelvetC function pointers members
	pFunc_bfcGetVersion bfcGetVersion = nullptr;
	pFunc_bfcFactory bfcFactory = nullptr;
	pFunc_bfcDestroy bfcDestroy = nullptr;
	pFunc_bfcEnumerate bfcEnumerate = nullptr;
	pFunc_bfcQueryCardType bfcQueryCardType = nullptr;
	pFunc_bfcAttach bfcAttach = nullptr;
	pFunc_bfcDetach bfcDetach = nullptr;
	pFunc_bfcQueryCardProperty32 bfcQueryCardProperty32 = nullptr;
	pFunc_bfcQueryCardProperty64 bfcQueryCardProperty64 = nullptr;
	pFunc_bfcSetCardProperty32 bfcSetCardProperty32 = nullptr;
	pFunc_bfcSetCardProperty64 bfcSetCardProperty64 = nullptr;
	pFunc_bfcGetCardSerialNumber bfcGetCardSerialNumber = nullptr;
	pFunc_bfcGetCardFwVersion bfcGetCardFwVersion = nullptr;
	pFunc_bfcWaitVideoSyncAsync bfcWaitVideoSyncAsync = nullptr;
	pFunc_bfcWaitVideoInputSync bfcWaitVideoInputSync = nullptr;
	pFunc_bfcWaitVideoOutputSync bfcWaitVideoOutputSync = nullptr;
	pFunc_bfcGetVideoOutputCurrentFieldCount bfcGetVideoOutputCurrentFieldCount = nullptr;
	pFunc_bfcGetVideoInputCurrentFieldCount bfcGetVideoInputCurrentFieldCount = nullptr;
	pFunc_bfcVideoCaptureStart bfcVideoCaptureStart = nullptr;
	pFunc_bfcVideoCaptureStop bfcVideoCaptureStop = nullptr;
	pFunc_bfcVideoPlaybackStart bfcVideoPlaybackStart = nullptr;
	pFunc_bfcVideoPlaybackStop bfcVideoPlaybackStop = nullptr;
	pFunc_bfcVideoPlaybackAllocate bfcVideoPlaybackAllocate = nullptr;
	pFunc_bfcVideoPlaybackPresent bfcVideoPlaybackPresent = nullptr;
	pFunc_bfcVideoPlaybackRelease bfcVideoPlaybackRelease = nullptr;
	pFunc_bfcGetCaptureVideoFrameInfoEx bfcGetCaptureVideoFrameInfoEx = nullptr;
	pFunc_bfcRenderBufferCapture bfcRenderBufferCapture = nullptr;
	pFunc_bfcRenderBufferUpdate bfcRenderBufferUpdate = nullptr;
	pFunc_bfcGetRenderBufferCount bfcGetRenderBufferCount = nullptr;
	pFunc_bfcEncodeHancFrameEx bfcEncodeHancFrameEx = nullptr;
	pFunc_bfcDecodeHancFrameEx bfcDecodeHancFrameEx = nullptr;
#if defined(_WIN32)
	pFunc_bfcSystemBufferReadAsync bfcSystemBufferReadAsync = nullptr;
	pFunc_bfcSystemBufferWriteAsync bfcSystemBufferWriteAsync = nullptr;
#else
	pFunc_bfcSystemBufferRead bfcSystemBufferRead = nullptr;
    pFunc_bfcSystemBufferWrite bfcSystemBufferWrite = nullptr;
#endif
	pFunc_bfcGetBytesForGroupPixels bfcGetBytesForGroupPixels = nullptr;
	pFunc_bfcGetPixelsPerLine bfcGetPixelsPerLine = nullptr;
	pFunc_bfcGetLinesPerFrame bfcGetLinesPerFrame = nullptr;
	pFunc_bfcGetBytesPerLine bfcGetBytesPerLine = nullptr;
	pFunc_bfcGetBytesPerFrame bfcGetBytesPerFrame = nullptr;
	pFunc_bfcGetGoldenValue bfcGetGoldenValue = nullptr;
	pFunc_bfcGetVBILines bfcGetVBILines = nullptr;
	pFunc_bfcGetVANCGoldenValue bfcGetVANCGoldenValue = nullptr;
	pFunc_bfcGetVANCLineBytes bfcGetVANCLineBytes = nullptr;
	pFunc_bfcGetVANCLineCount bfcGetVANCLineCount = nullptr;
	pFunc_bfcGetWindowsDriverHandle bfcGetWindowsDriverHandle = nullptr;
	pFunc_bfcUtilsGetStringForCardType bfcUtilsGetStringForCardType = nullptr;
	pFunc_bfcUtilsGetStringForBlueProductId bfcUtilsGetStringForBlueProductId = nullptr;
	pFunc_bfcUtilsGetStringForVideoMode bfcUtilsGetStringForVideoMode = nullptr;
	pFunc_bfcUtilsGetStringForMemoryFormat bfcUtilsGetStringForMemoryFormat = nullptr;
	pFunc_bfcUtilsGetMR2Routing bfcUtilsGetMR2Routing = nullptr;
	pFunc_bfcUtilsSetMR2Routing bfcUtilsSetMR2Routing = nullptr;
	pFunc_bfcUtilsGetAudioOutputRouting bfcUtilsGetAudioOutputRouting = nullptr;
	pFunc_bfcUtilsSetAudioOutputRouting bfcUtilsSetAudioOutputRouting = nullptr;
	pFunc_bfcUtilsIsVideoModeProgressive bfcUtilsIsVideoModeProgressive = nullptr;
	pFunc_bfcUtilsIsVideoMode1001Framerate bfcUtilsIsVideoMode1001Framerate = nullptr;
	pFunc_bfcUtilsGetFpsForVideoMode bfcUtilsGetFpsForVideoMode = nullptr;
	pFunc_bfcUtilsGetVideoModeForFrameInfo bfcUtilsGetVideoModeForFrameInfo = nullptr;
	pFunc_bfcUtilsGetFrameInfoForVideoMode bfcUtilsGetFrameInfoForVideoMode = nullptr;
	pFunc_bfcUtilsGetAudioSamplesPerFrame bfcUtilsGetAudioSamplesPerFrame = nullptr;

};

spl::shared_ptr<bvc_wrapper> create_blue();
spl::shared_ptr<bvc_wrapper> create_blue(int device_index);

bool is_epoch_card(bvc_wrapper& blue);
bool is_epoch_neutron_1i2o_card(bvc_wrapper& blue);
bool is_epoch_neutron_3o_card(bvc_wrapper& blue);
std::wstring get_card_desc(bvc_wrapper& blue, int device_id);
EVideoMode get_video_mode(bvc_wrapper& blue, const core::video_format_desc& format_desc);
core::video_format_desc get_format_desc(bvc_wrapper& blue, EVideoMode vid_fmt, EMemoryFormat mem_fmt);

}}