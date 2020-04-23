/*
  Copyright (c) Bluefish444. All rights reserved.

  NOTE: Please add
        #define IMPLEMENTATION_BLUEVELVETC_FUNC_PTR
        before including this file so that all the SDK functions are defined
        in your project. If including this header files from multiple c/c++
        files make sure that only one c/c++ file which includs this header
        file defines IMPLEMENTATION_BLUEVELVETC_FUNC_PTR

        #define IMPLEMENTATION_BLUEVELVETC_CONVERSION_FUNC_PTR
        before including this file so that all conversion functions are defined
        in your project. If including this header files from multiple c/c++
        files make sure that only one c/c++ file which includs this header
        file defines IMPLEMENTATION_BLUEVELVETC_CONVERSION_FUNC_PTR
*/

#ifndef HG_BLUEVELVETC_FUNC_PTR
#define HG_BLUEVELVETC_FUNC_PTR


#if defined (_WIN32)
    #define GET_PROCADDR_FOR_FUNC(module, available, name) { name = (pFunc_##name)GetProcAddress(module, #name); if(!name) { *available = false; OutputDebugStringA(#name); break; } }
    
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
	#include <Windows.h>
    #include "BlueDriver_p.h"
#elif defined (__linux__)
    #define GET_PROCADDR_FOR_FUNC(module, available, name) { name = (pFunc_##name)dlsym(module, #name); if(!name) { *available = false; break; } }

    #include <dlfcn.h>
    #define __cdecl

    #include "../../hal/BlueTypes.h"
    #include "../../hal/BlueDriver_p.h"

#endif


typedef void* BLUEVELVETC_HANDLE;
typedef void* BFC_SYNC_INFO;
#define	BLUE_OK(a)      (!a)    /* Test for succcess of a method returning BErr */
#define	BLUE_FAIL(a)    (a)     /* Test for failure of a method returning BErr */


/* FUNCTION PROTOTYPES FOR BLUEVELVETC */
#if defined(__cplusplus)
extern "C" {
#endif
    typedef const char* (__cdecl *pFunc_bfcGetVersion)();
    typedef const wchar_t* (__cdecl *pFunc_bfcGetVersionW)();
    typedef BLUEVELVETC_HANDLE(__cdecl *pFunc_bfcFactory)();
    typedef BLUE_VOID(__cdecl *pFunc_bfcDestroy)(BLUEVELVETC_HANDLE pHandle);
    typedef BLUE_S32(__cdecl *pFunc_bfcEnumerate)(BLUEVELVETC_HANDLE pHandle, BLUE_S32* pDeviceCount);
    typedef BLUE_S32(__cdecl *pFunc_bfcQueryCardType)(BLUEVELVETC_HANDLE pHandle, BLUE_S32* pCardType, BLUE_S32 DeviceID);
    typedef BLUE_S32(__cdecl *pFunc_bfcAttach)(BLUEVELVETC_HANDLE pHandle, BLUE_S32 DeviceId);
    typedef BLUE_S32(__cdecl *pFunc_bfcSetMultiLinkMode)(BLUEVELVETC_HANDLE pHandle, blue_multi_link_info_struct* pMultiLinkInfo);
    typedef BLUE_S32(__cdecl *pFunc_bfcQueryMultiLinkMode)(BLUEVELVETC_HANDLE pHandle, blue_multi_link_info_struct* pMultiLinkInfo);
    typedef BLUE_S32(__cdecl *pFunc_bfcDetach)(BLUEVELVETC_HANDLE pHandle);
    typedef BLUE_S32(__cdecl *pFunc_bfcQueryCardProperty32)(BLUEVELVETC_HANDLE pHandle, const BLUE_S32 Property, BLUE_U32* pValue32);
    typedef BLUE_S32(__cdecl *pFunc_bfcSetCardProperty32)(BLUEVELVETC_HANDLE pHandle, const BLUE_S32 Property, const BLUE_U32 Value32);
    typedef BLUE_S32(__cdecl *pFunc_bfcQueryCardProperty64)(BLUEVELVETC_HANDLE pHandle, const BLUE_S32 Property, BLUE_U64* pValue64);
    typedef BLUE_S32(__cdecl *pFunc_bfcSetCardProperty64)(BLUEVELVETC_HANDLE pHandle, const BLUE_S32 Property, const BLUE_U64 Value64);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetCardSerialNumber)(BLUEVELVETC_HANDLE pHandle, BLUE_S8* pSerialNumber, BLUE_U32 StringSize);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetCardFwVersion)(BLUEVELVETC_HANDLE pHandle, BLUE_U32* pValue);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetCardHwFwVersions)(BLUEVELVETC_HANDLE pHandle, hardware_firmware_versions* pVersions);
#if defined (_WIN32)
    typedef BLUE_S32(__cdecl *pFunc_bfcWaitVideoSyncAsync)(BLUEVELVETC_HANDLE pHandle, OVERLAPPED* pOverlap, blue_video_sync_struct* pSyncData);
#endif
    typedef BLUE_S32(__cdecl *pFunc_bfcWaitVideoSync)(BLUEVELVETC_HANDLE pHandle, sync_options* pSyncData, BFC_SYNC_INFO SyncInfo);
    typedef BLUE_S32(__cdecl *pFunc_bfcWaitVideoInputSync)(BLUEVELVETC_HANDLE pHandle, unsigned long UpdateType, unsigned long* pFieldCount);
    typedef BLUE_S32(__cdecl *pFunc_bfcWaitVideoOutputSync)(BLUEVELVETC_HANDLE pHandle, unsigned long UpdateType, unsigned long* pFieldCount);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetVideoOutputCurrentFieldCount)(BLUEVELVETC_HANDLE pHandle, unsigned long* pFieldCount);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetVideoInputCurrentFieldCount)(BLUEVELVETC_HANDLE pHandle, unsigned long* pFieldCount);
    typedef BLUE_S32(__cdecl *pFunc_bfcVideoCaptureStart)(BLUEVELVETC_HANDLE pHandle);
    typedef BLUE_S32(__cdecl *pFunc_bfcVideoCaptureStop)(BLUEVELVETC_HANDLE pHandle);
    typedef BLUE_S32(__cdecl *pFunc_bfcVideoPlaybackStart)(BLUEVELVETC_HANDLE pHandle, BLUE_S32 Step, BLUE_S32 Loop);
    typedef BLUE_S32(__cdecl *pFunc_bfcVideoPlaybackStop)(BLUEVELVETC_HANDLE pHandle, BLUE_S32 Wait, BLUE_S32 Flush);
    typedef BLUE_S32(__cdecl *pFunc_bfcVideoPlaybackAllocate)(BLUEVELVETC_HANDLE pHandle, BLUE_VOID** pAddress, unsigned long* pBufferID, unsigned long* pUnderrun);
    typedef BLUE_S32(__cdecl *pFunc_bfcVideoPlaybackPresent)(BLUEVELVETC_HANDLE pHandle, unsigned long* UniqueID, unsigned long BufferID, unsigned long Count, BLUE_S32 Keep, BLUE_S32 Odd);
    typedef BLUE_S32(__cdecl *pFunc_bfcVideoPlaybackRelease)(BLUEVELVETC_HANDLE pHandle, unsigned long BufferID);
#if defined (_WIN32)
    typedef BLUE_S32(__cdecl *pFunc_bfcGetCaptureVideoFrameInfoEx)(BLUEVELVETC_HANDLE pHandle, OVERLAPPED* pOverlap, struct blue_videoframe_info_ex* pVideoFrameInfo, BLUE_S32 iCompostLater, BLUE_U32* CaptureFifoSize);
#elif defined(__linux__)
    typedef BLUE_S32(__cdecl *pFunc_bfcGetCaptureVideoFrameInfoEx)(BLUEVELVETC_HANDLE pHandle, struct blue_videoframe_info_ex* pVideoFrameInfo);
#endif
    typedef BLUE_S32(__cdecl *pFunc_bfcRenderBufferCapture)(BLUEVELVETC_HANDLE pHandle, unsigned long BufferID);
    typedef BLUE_S32(__cdecl *pFunc_bfcRenderBufferUpdate)(BLUEVELVETC_HANDLE pHandle, unsigned long BufferID);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetRenderBufferCount)(BLUEVELVETC_HANDLE pHandle, unsigned long* pCount);
    typedef BLUE_S32(__cdecl *pFunc_bfcEncodeHancFrameEx)(BLUEVELVETC_HANDLE pHandle, BLUE_U32 CardType, struct hanc_stream_info_struct* pHancEncodeInfo, BLUE_VOID* pAudioBuffer, BLUE_U32 AudioChannels, BLUE_U32 AudioSamples, BLUE_U32 SampleType, BLUE_U32 AudioFlags);
    typedef BLUE_S32(__cdecl *pFunc_bfcEncodeHancFrameWithUCZ)(BLUEVELVETC_HANDLE pHandle, BLUE_U32 CardType, struct hanc_stream_info_struct* pHancEncodeInfo, BLUE_VOID* pAudioBuffer, BLUE_U32 AudioChannels, BLUE_U32 AudioSamples, BLUE_U32 SampleType, BLUE_U8* pUCZBuffer);
    typedef BLUE_S32(__cdecl *pFunc_bfcDecodeHancFrameEx)(BLUEVELVETC_HANDLE pHandle, BLUE_U32 CardType, BLUE_U32* pHancBuffer, struct hanc_decode_struct* pHancDecodeInfo);
#if defined(_WIN32)
    typedef BLUE_S32(__cdecl *pFunc_bfcSystemBufferReadAsync)(BLUEVELVETC_HANDLE pHandle, unsigned char* pPixels, unsigned long ulSize, OVERLAPPED* pOverlap, unsigned long ulBufferID, unsigned long ulOffset);
    typedef BLUE_S32(__cdecl *pFunc_bfcSystemBufferWriteAsync)(BLUEVELVETC_HANDLE pHandle, unsigned char* pPixels, unsigned long ulSize, OVERLAPPED* pOverlap, unsigned long ulBufferID, unsigned long ulOffset);
#elif defined(__linux__)
    typedef BLUE_S32(__cdecl *pFunc_bfcSystemBufferRead)(BLUEVELVETC_HANDLE pHandle, unsigned char* pPixels, unsigned long ulSize, unsigned long ulBufferID, unsigned long ulOffset);
    typedef BLUE_S32(__cdecl *pFunc_bfcSystemBufferWrite)(BLUEVELVETC_HANDLE pHandle, unsigned char* pPixels, unsigned long ulSize, unsigned long ulBufferID, unsigned long ulOffset);
#endif
    typedef BFC_SYNC_INFO(__cdecl *pFunc_bfcSyncInfoCreate)(BLUEVELVETC_HANDLE pHandle);
    typedef BLUE_S32(__cdecl *pFunc_bfcSyncInfoDelete)(BLUEVELVETC_HANDLE pHandle, BFC_SYNC_INFO SyncInfo);
    typedef BLUE_S32(__cdecl *pFunc_bfcSyncInfoWait)(BLUEVELVETC_HANDLE pHandle, BFC_SYNC_INFO SyncInfo, const BLUE_U32 nTimeOutInMilliSec);
    typedef BLUE_S32(__cdecl *pFunc_bfcSyncInfoWaitWithSyncOptions)(BLUEVELVETC_HANDLE pHandle, BFC_SYNC_INFO SyncInfo, sync_options* pSyncOptions, const BLUE_U32 TimeOutInMilliSec);
    typedef BLUE_S32(__cdecl *pFunc_bfcDmaReadFromCardAsync)(BLUEVELVETC_HANDLE pHandle, BLUE_U8* pData, unsigned long Size, BFC_SYNC_INFO SyncInfo, unsigned long BufferID, unsigned long Offset);
    typedef BLUE_S32(__cdecl *pFunc_bfcDmaWriteToCardAsync)(BLUEVELVETC_HANDLE pHandle, BLUE_U8* pData, unsigned long Size, BFC_SYNC_INFO SyncInfo, unsigned long BufferID, unsigned long Offset);
    typedef BLUE_S32(__cdecl *pFunc_bfcSerialPortWaitForInputData)(BLUEVELVETC_HANDLE pHandle, BLUE_U32 PortFlags, BLUE_U32* pBufferLength);
    typedef BLUE_S32(__cdecl *pFunc_bfcSerialPortRead)(BLUEVELVETC_HANDLE pHandle, BLUE_U32 nPortFlags, BLUE_U8* pBuffer, BLUE_U32 ReadLength);
    typedef BLUE_S32(__cdecl *pFunc_bfcSerialPortWrite)(BLUEVELVETC_HANDLE pHandle, BLUE_U32 nPortFlags, BLUE_U8* pBuffer, BLUE_U32 WriteLength);
#if defined (_WIN32)
    typedef BLUE_S32(__cdecl *pFunc_bfcGetReferenceClockPhaseSettings)(BLUEVELVETC_HANDLE pHandle, BLUE_U32* pHPhase, BLUE_U32* pVPhase, BLUE_U32* pHPhaseMax, BLUE_U32* pVPhaseMax);
    typedef BLUE_S32(__cdecl *pFunc_bfcLoadOutputLUT1D)(BLUEVELVETC_HANDLE pHandle, struct blue_1d_lookup_table_struct* pLutData);
#endif
    typedef BLUE_S32(__cdecl *pFunc_bfcGetVideoWidth)(BLUE_S32 VideoMode, BLUE_U32* pWidth);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetVideoHeight)(BLUE_S32 VideoMode, BLUE_S32 UpdateMethod, BLUE_U32* pHeight);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetVideoBytesPerLineV2)(BLUE_S32 VideoMode, BLUE_S32 MemoryFormat, BLUE_U32* pBytesPerLine);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetVideoBytesPerFrame)(BLUE_S32 VideoMode, BLUE_S32 UpdateMethod, BLUE_S32 MemoryFormat, BLUE_U32* pBytesPerFrame);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetVideoBytesPerFrameGolden)(BLUE_S32 VideoMode, BLUE_S32 UpdateMethod, BLUE_S32 MemoryFormat, BLUE_U32* pGoldenBytes);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetVideoInfo)(BLUE_S32 VideoMode, BLUE_S32 UpdateMethod, BLUE_S32 MemoryFormat, BLUE_U32* pWidth, BLUE_U32* pHeight, BLUE_U32* pBytesPerLine, BLUE_U32* pBytesPerFrame, BLUE_U32* pGoldenBytes);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetVancWidth)(BLUE_S32 VideoMode, BLUE_U32* pWidth);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetVancHeight)(BLUE_S32 VideoMode, BLUE_S32 UpdateMethod, BLUE_U32* pHeight);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetVancBytesPerLineV2)(BLUE_S32 VideoMode, BLUE_S32 MemoryFormat, BLUE_U32* pBytesPerLine);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetVancBytesPerFrame)(BLUE_S32 VideoMode, BLUE_S32 UpdateMethod, BLUE_S32 MemoryFormat, BLUE_U32* pBytesPerFrame);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetVancInfo)(BLUE_S32 VideoMode, BLUE_S32 UpdateMethod, BLUE_S32 MemoryFormat, BLUE_U32* pWidth, BLUE_U32* pHeight, BLUE_U32* pBytesPerLine, BLUE_U32* pBytesPerFrame);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetBytesForGroupPixels)(EMemoryFormat MemoryFormat, BLUE_U32 VideoWidth, BLUE_U32* pVideoPitchBytes);
#if defined (_WIN32)
    typedef BLUE_S32(__cdecl *pFunc_bfcGetWindowsDriverHandle)(BLUEVELVETC_HANDLE pHandle, HANDLE* pDriverHandle);
    typedef BLUE_S32(__cdecl *pFunc_bfcSetDynamicMemoryFormatChange)(BLUEVELVETC_HANDLE pHandle, OVERLAPPED* pOverlap, BLUE_U32 nUniqueId, EMemoryFormat MemoryFormat);
#elif defined(__linux__)
    typedef BLUE_S32(__cdecl *pFunc_bfcGetFileHandle)(BLUEVELVETC_HANDLE pHandle, BLUE_U32* pFileHandle);
#endif
    /* BlueVelvetC utils functions */
    typedef void*(__cdecl *pFunc_bfAlloc)(BLUE_U32 nMemorySize);
    typedef void(__cdecl *pFunc_bfFree)(BLUE_U32 nMemSize, BLUE_VOID* pMemory); 
    typedef char*(__cdecl *pFunc_bfcUtilsGetStringForCardType)(const BLUE_S32 CardType);
    typedef wchar_t*(__cdecl *pFunc_bfcUtilsGetWStringForCardType)(const BLUE_S32 CardType);
    typedef char*(__cdecl *pFunc_bfcUtilsGetStringForBlueProductId)(const BLUE_U32 ProductId);
    typedef wchar_t*(__cdecl *pFunc_bfcUtilsGetWStringForBlueProductId)(const BLUE_U32 ProductId);
    typedef char*(__cdecl *pFunc_bfcUtilsGetStringForVideoMode)(const BLUE_U32 VideoMode);
    typedef wchar_t*(__cdecl *pFunc_bfcUtilsGetWStringForVideoMode)(const BLUE_U32 VideoMode);
    typedef char*(__cdecl *pFunc_bfcUtilsGetStringForMemoryFormat)(const BLUE_U32 MemoryFormat);
    typedef wchar_t*(__cdecl *pFunc_bfcUtilsGetWStringForMemoryFormat)(const BLUE_U32 MemoryFormat);
    typedef char*(__cdecl *pFunc_bfcUtilsGetStringForUpdateFormat)(const BLUE_U32 UpdateFormat);
    typedef wchar_t*(__cdecl *pFunc_bfcUtilsGetWStringForUpdateFormat)(const BLUE_U32 UpdateFormat);
    typedef char*(__cdecl *pFunc_bfcUtilsGetStringForVideoEngine)(const BLUE_U32 VideoEngine);
    typedef wchar_t*(__cdecl *pFunc_bfcUtilsGetWStringForVideoEngine)(const BLUE_U32 VideoEngine);
    typedef char*(__cdecl *pFunc_bfcUtilsGetStringForMr2Node)(const BLUE_U32 Mr2Node);
    typedef char*(__cdecl *pFunc_bfcUtilsGetWStringForMr2Node)(const BLUE_U32 Mr2Node);
    typedef BLUE_S32(__cdecl *pFunc_bfcUtilsGetMR2Routing)(const BLUEVELVETC_HANDLE pHandle, BLUE_U32* pSrcNode, const BLUE_U32 DestNode, BLUE_U32* LinkType);
    typedef BLUE_S32(__cdecl *pFunc_bfcUtilsSetMR2Routing)(const BLUEVELVETC_HANDLE pHandle, const BLUE_U32 SrcNode, const BLUE_U32 DestNode, const BLUE_U32 LinkType);
    typedef BLUE_S32(__cdecl *pFunc_bfcUtilsGetAudioOutputRouting)(const BLUEVELVETC_HANDLE pHandle, const BLUE_U32 AudioConnectorType, BLUE_U32* pAudioSourceChannelId, BLUE_U32 AudioConnectorId);
    typedef BLUE_S32(__cdecl *pFunc_bfcUtilsSetAudioOutputRouting)(const BLUEVELVETC_HANDLE pHandle, const BLUE_U32 AudioConnectorType, BLUE_U32 AudioSourceChannelId, BLUE_U32 AudioConnectorId);
    typedef bool(__cdecl *pFunc_bfcUtilsIsVideoModeProgressive)(const BLUE_U32 VideoMode);
    typedef bool(__cdecl *pFunc_bfcUtilsIsVideoMode1001Framerate)(const BLUE_U32 VideoMode);
    typedef BLUE_S32(__cdecl *pFunc_bfcUtilsGetFpsForVideoMode)(const BLUE_U32 VideoMode);
    typedef BLUE_S32(__cdecl *pFunc_bfcUtilsGetVideoModeExtForFrameInfo)(const BLUE_U32 Width, const BLUE_U32 Height, const BLUE_U32 Rate, const BLUE_U32 bIs1001, const BLUE_U32 bIsProgressive, BLUE_U32* pVideoModeExt);
    typedef BLUE_S32(__cdecl *pFunc_bfcUtilsGetFrameInfoForVideoModeExt)(const BLUE_U32 VideoModeExt, BLUE_U32* pWidth, BLUE_U32* pHeight, BLUE_U32* pRate, BLUE_U32* pIs1001, BLUE_U32* pIsProgressive);
    typedef BLUE_S32(__cdecl *pFunc_bfcUtilsGetAudioSamplesPerFrame)(const BLUE_U32 VideoMode, const BLUE_U32 FrameNo);
    typedef BLUE_S32(__cdecl *pFunc_bfcUtilsInitVancBuffer)(BLUE_U32 nCardType, BLUE_U32 VideoMode, BLUE_U32 PixelsPerLine, BLUE_U32 nLinesPerFrame, BLUE_U32* pVancBuffer);
    typedef BLUE_S32(__cdecl *pFunc_bfcUtilsVancPktExtract)(BLUE_U32 nCardType, BLUE_U32 nVancPktType, BLUE_U32* pSrcVancBuffer, BLUE_U32 nSrcVancBufferSize, BLUE_U32 PixelsPerLine, BLUE_U32 nVancPktDid, BLUE_U16* pVancPktSdid, BLUE_U16* pVancPktDataLength, BLUE_U16* pVancPktData, BLUE_U16* pVancPktLineNo);
    typedef BLUE_S32(__cdecl *pFunc_bfcUtilsVancPktInsert)(BLUE_U32 nCardType, BLUE_U32 nVancPktType, BLUE_U32 nVancPktLineNumber, BLUE_U32* pVancPktBuffer, BLUE_U32 nVancPktBufferSize, BLUE_U32* pDestVancBuffer, BLUE_U32 PixelsPerLine);
    typedef BLUE_S32(__cdecl *pFunc_bfcUtilsDecodeEia708bPkt)(BLUE_U32 CardType, BLUE_U16* pVancPacketData, BLUE_U16 PacketUdwCount, BLUE_U16 EiaPacketSubtype, BLUE_U8* pDecodedString);

    /* deprecated */
    typedef BLUE_S32(__cdecl *pFunc_bfcUtilsGetVideoModeForFrameInfo)(const BLUE_U32 Width, const BLUE_U32 Height, const BLUE_U32 Rate, const BLUE_U32 bIs1001, const BLUE_U32 bIsProgressive, BLUE_U32* pVideoMode);
    typedef BLUE_S32(__cdecl *pFunc_bfcUtilsGetFrameInfoForVideoMode)(const BLUE_U32 VideoMode, BLUE_U32* pWidth, BLUE_U32* pHeight, BLUE_U32* pRate, BLUE_U32* pIs1001, BLUE_U32* pIsProgressive);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetVideoBytesPerLine)(BLUE_S32 VideoMode, BLUE_S32 UpdateMethod, BLUE_S32 MemoryFormat, BLUE_U32* pBytesPerLine);
    typedef BLUE_S32(__cdecl *pFunc_bfcGetVancBytesPerLine)(BLUE_S32 VideoMode, BLUE_S32 UpdateMethod, BLUE_S32 MemoryFormat, BLUE_U32* pBytesPerLine);

#if defined(__cplusplus)
}	//	extern "C"
#endif

#ifdef IMPLEMENTATION_BLUEVELVETC_FUNC_PTR
pFunc_bfcGetVersion bfcGetVersion;
pFunc_bfcGetVersionW bfcGetVersionW;
pFunc_bfcFactory bfcFactory;
pFunc_bfcDestroy bfcDestroy;
pFunc_bfcEnumerate bfcEnumerate;
pFunc_bfcQueryCardType bfcQueryCardType;
pFunc_bfcAttach bfcAttach;
pFunc_bfcSetMultiLinkMode bfcSetMultiLinkMode;
pFunc_bfcQueryMultiLinkMode bfcQueryMultiLinkMode;
pFunc_bfcDetach bfcDetach;
pFunc_bfcQueryCardProperty32 bfcQueryCardProperty32;
pFunc_bfcQueryCardProperty64 bfcQueryCardProperty64;
pFunc_bfcSetCardProperty32 bfcSetCardProperty32;
pFunc_bfcSetCardProperty64 bfcSetCardProperty64;
pFunc_bfcGetCardSerialNumber bfcGetCardSerialNumber;
pFunc_bfcGetCardFwVersion bfcGetCardFwVersion;
pFunc_bfcGetCardHwFwVersions bfcGetCardHwFwVersions;
#if defined(_WIN32)
pFunc_bfcWaitVideoSyncAsync bfcWaitVideoSyncAsync;
#endif
pFunc_bfcWaitVideoSync  bfcWaitVideoSync;
pFunc_bfcWaitVideoInputSync bfcWaitVideoInputSync;
pFunc_bfcWaitVideoOutputSync bfcWaitVideoOutputSync;
pFunc_bfcGetVideoOutputCurrentFieldCount bfcGetVideoOutputCurrentFieldCount;
pFunc_bfcGetVideoInputCurrentFieldCount bfcGetVideoInputCurrentFieldCount;
pFunc_bfcVideoCaptureStart bfcVideoCaptureStart;
pFunc_bfcVideoCaptureStop bfcVideoCaptureStop;
pFunc_bfcVideoPlaybackStart bfcVideoPlaybackStart;
pFunc_bfcVideoPlaybackStop bfcVideoPlaybackStop;
pFunc_bfcVideoPlaybackAllocate bfcVideoPlaybackAllocate;
pFunc_bfcVideoPlaybackPresent bfcVideoPlaybackPresent;
pFunc_bfcVideoPlaybackRelease bfcVideoPlaybackRelease;
pFunc_bfcGetCaptureVideoFrameInfoEx bfcGetCaptureVideoFrameInfoEx;
pFunc_bfcRenderBufferCapture bfcRenderBufferCapture;
pFunc_bfcRenderBufferUpdate bfcRenderBufferUpdate;
pFunc_bfcGetRenderBufferCount bfcGetRenderBufferCount;
pFunc_bfcEncodeHancFrameEx bfcEncodeHancFrameEx;
pFunc_bfcEncodeHancFrameWithUCZ bfcEncodeHancFrameWithUCZ;
pFunc_bfcDecodeHancFrameEx bfcDecodeHancFrameEx;
#if defined(_WIN32)
pFunc_bfcSystemBufferReadAsync bfcSystemBufferReadAsync;
pFunc_bfcSystemBufferWriteAsync bfcSystemBufferWriteAsync;
#elif defined(__linux__)
pFunc_bfcSystemBufferRead bfcSystemBufferRead;
pFunc_bfcSystemBufferWrite bfcSystemBufferWrite;
#endif
pFunc_bfcSyncInfoCreate bfcSyncInfoCreate;
pFunc_bfcSyncInfoDelete bfcSyncInfoDelete;
pFunc_bfcSyncInfoWait bfcSyncInfoWait;
pFunc_bfcSyncInfoWaitWithSyncOptions bfcSyncInfoWaitWithSyncOptions;
pFunc_bfcDmaReadFromCardAsync bfcDmaReadFromCardAsync;
pFunc_bfcDmaWriteToCardAsync bfcDmaWriteToCardAsync;
pFunc_bfcSerialPortWaitForInputData bfcSerialPortWaitForInputData;
pFunc_bfcSerialPortRead bfcSerialPortRead;
pFunc_bfcSerialPortWrite bfcSerialPortWrite;
#if defined(_WIN32)
pFunc_bfcGetReferenceClockPhaseSettings bfcGetReferenceClockPhaseSettings;
pFunc_bfcLoadOutputLUT1D bfcLoadOutputLUT1D;
#endif
pFunc_bfcGetVideoWidth bfcGetVideoWidth;
pFunc_bfcGetVideoHeight bfcGetVideoHeight;
pFunc_bfcGetVideoBytesPerLineV2 bfcGetVideoBytesPerLineV2;
pFunc_bfcGetVideoBytesPerFrame bfcGetVideoBytesPerFrame;
pFunc_bfcGetVideoBytesPerFrameGolden bfcGetVideoBytesPerFrameGolden;
pFunc_bfcGetVideoInfo bfcGetVideoInfo;
pFunc_bfcGetVancWidth bfcGetVancWidth;
pFunc_bfcGetVancHeight bfcGetVancHeight;
pFunc_bfcGetVancBytesPerLineV2 bfcGetVancBytesPerLineV2;
pFunc_bfcGetVancBytesPerFrame bfcGetVancBytesPerFrame;
pFunc_bfcGetVancInfo bfcGetVancInfo;
pFunc_bfcGetBytesForGroupPixels bfcGetBytesForGroupPixels;
#if defined(_WIN32)
pFunc_bfcGetWindowsDriverHandle bfcGetWindowsDriverHandle;
pFunc_bfcSetDynamicMemoryFormatChange bfcSetDynamicMemoryFormatChange;
#elif defined(__linux__)
pFunc_bfcGetFileHandle bfcGetFileHandle;
#endif

/* BlueVelvetC utils functions */
pFunc_bfAlloc bfAlloc;
pFunc_bfFree bfFree;
pFunc_bfcUtilsGetStringForCardType bfcUtilsGetStringForCardType;
pFunc_bfcUtilsGetWStringForCardType bfcUtilsGetWStringForCardType;
pFunc_bfcUtilsGetStringForBlueProductId bfcUtilsGetStringForBlueProductId;
pFunc_bfcUtilsGetWStringForBlueProductId bfcUtilsGetWStringForBlueProductId;
pFunc_bfcUtilsGetStringForVideoMode bfcUtilsGetStringForVideoMode;
pFunc_bfcUtilsGetWStringForVideoMode bfcUtilsGetWStringForVideoMode;
pFunc_bfcUtilsGetStringForMemoryFormat bfcUtilsGetStringForMemoryFormat;
pFunc_bfcUtilsGetWStringForMemoryFormat bfcUtilsGetWStringForMemoryFormat;
pFunc_bfcUtilsGetStringForUpdateFormat bfcUtilsGetStringForUpdateFormat;
pFunc_bfcUtilsGetWStringForUpdateFormat bfcUtilsGetWStringForUpdateFormat;
pFunc_bfcUtilsGetStringForVideoEngine bfcUtilsGetStringForVideoEngine;
pFunc_bfcUtilsGetWStringForVideoEngine bfcUtilsGetWStringForVideoEngine;
pFunc_bfcUtilsGetStringForMr2Node bfcUtilsGetStringForMr2Node;
pFunc_bfcUtilsGetWStringForMr2Node bfcUtilsGetWStringForMr2Node;
pFunc_bfcUtilsGetMR2Routing bfcUtilsGetMR2Routing;
pFunc_bfcUtilsSetMR2Routing bfcUtilsSetMR2Routing;
pFunc_bfcUtilsGetAudioOutputRouting bfcUtilsGetAudioOutputRouting;
pFunc_bfcUtilsSetAudioOutputRouting bfcUtilsSetAudioOutputRouting;
pFunc_bfcUtilsIsVideoModeProgressive bfcUtilsIsVideoModeProgressive;
pFunc_bfcUtilsIsVideoMode1001Framerate bfcUtilsIsVideoMode1001Framerate;
pFunc_bfcUtilsGetFpsForVideoMode bfcUtilsGetFpsForVideoMode;
pFunc_bfcUtilsGetVideoModeExtForFrameInfo bfcUtilsGetVideoModeExtForFrameInfo;
pFunc_bfcUtilsGetFrameInfoForVideoModeExt bfcUtilsGetFrameInfoForVideoModeExt;
pFunc_bfcUtilsGetAudioSamplesPerFrame bfcUtilsGetAudioSamplesPerFrame;
pFunc_bfcUtilsInitVancBuffer bfcUtilsInitVancBuffer;
pFunc_bfcUtilsVancPktExtract bfcUtilsVancPktExtract;
pFunc_bfcUtilsVancPktInsert bfcUtilsVancPktInsert;
pFunc_bfcUtilsDecodeEia708bPkt bfcUtilsDecodeEia708bPkt;

/* deprecated start */
pFunc_bfcUtilsGetVideoModeForFrameInfo bfcUtilsGetVideoModeForFrameInfo;
pFunc_bfcUtilsGetFrameInfoForVideoMode bfcUtilsGetFrameInfoForVideoMode; 
pFunc_bfcGetVideoBytesPerLine bfcGetVideoBytesPerLine;
pFunc_bfcGetVancBytesPerLine bfcGetVancBytesPerLine;
/* deprecated end */

bool LoadFunctionPointers_BlueVelvetC()
{
    bool bAllFunctionsAvailable = false;
#if defined(_WIN32)
    #if defined(_WIN64)
        #if defined(_DEBUG)
            HMODULE hLib = LoadLibraryExA("BlueVelvetC64_d.dll", NULL, 0);
        #else
            HMODULE hLib = LoadLibraryExA("BlueVelvetC64.dll", NULL, 0);
        #endif
    #else
        #if defined(_DEBUG)
            HMODULE hLib = LoadLibraryExA("BlueVelvetC_d.dll", NULL, 0);
        #else
            HMODULE hLib = LoadLibraryExA("BlueVelvetC.dll", NULL, 0);
        #endif
    #endif
#endif


#if defined(__linux__)
    void* hLib = dlopen("libBlueVelvetC64.so", RTLD_LAZY | RTLD_GLOBAL);
#endif

    if(hLib)
    {
        bAllFunctionsAvailable = true;
        do
        {
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetVersion);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetVersionW);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcFactory);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcDestroy);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcEnumerate);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcQueryCardType);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcAttach);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcSetMultiLinkMode);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcQueryMultiLinkMode);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcDetach);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcQueryCardProperty32);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcQueryCardProperty64);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcSetCardProperty32);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcSetCardProperty64);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetCardSerialNumber);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetCardFwVersion);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetCardHwFwVersions);
#if defined(_WIN32)
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcWaitVideoSyncAsync);
#endif
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcWaitVideoSync);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcWaitVideoInputSync);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcWaitVideoOutputSync);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetVideoOutputCurrentFieldCount);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetVideoInputCurrentFieldCount);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcVideoCaptureStart);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcVideoCaptureStop);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcVideoPlaybackStart);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcVideoPlaybackStop);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcVideoPlaybackAllocate);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcVideoPlaybackPresent);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcVideoPlaybackRelease);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetCaptureVideoFrameInfoEx);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcRenderBufferCapture);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcRenderBufferUpdate);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetRenderBufferCount);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcEncodeHancFrameEx);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcEncodeHancFrameWithUCZ);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcDecodeHancFrameEx);
#if defined(_WIN32)
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcSystemBufferReadAsync);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcSystemBufferWriteAsync);
#elif defined(__linux__)
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcSystemBufferRead);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcSystemBufferWrite);
#endif
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcSyncInfoCreate);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcSyncInfoDelete);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcSyncInfoWait);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcSyncInfoWaitWithSyncOptions);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcDmaReadFromCardAsync);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcDmaWriteToCardAsync);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcSerialPortWaitForInputData);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcSerialPortRead);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcSerialPortWrite);
#if defined(_WIN32)
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetReferenceClockPhaseSettings);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcLoadOutputLUT1D);
#endif
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetVideoWidth);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetVideoHeight);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetVideoBytesPerLineV2);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetVideoBytesPerFrame);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetVideoBytesPerFrameGolden);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetVideoInfo);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetVancWidth);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetVancHeight);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetVancBytesPerLineV2);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetVancBytesPerFrame);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetVancInfo);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetBytesForGroupPixels);
#if defined(_WIN32)
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetWindowsDriverHandle);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcSetDynamicMemoryFormatChange);
#elif defined(__linux__)
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetFileHandle);
#endif
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfAlloc);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfFree);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetStringForCardType);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetWStringForCardType);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetStringForBlueProductId);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetWStringForBlueProductId);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetStringForVideoMode);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetWStringForVideoMode);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetStringForMemoryFormat);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetWStringForMemoryFormat);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetStringForUpdateFormat);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetWStringForUpdateFormat);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetStringForVideoEngine);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetWStringForVideoEngine);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetStringForMr2Node);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetWStringForMr2Node);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetMR2Routing);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsSetMR2Routing);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetAudioOutputRouting);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsSetAudioOutputRouting);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsIsVideoModeProgressive);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsIsVideoMode1001Framerate);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetFpsForVideoMode);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetVideoModeExtForFrameInfo);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetFrameInfoForVideoModeExt);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetAudioSamplesPerFrame);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsInitVancBuffer);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsVancPktExtract);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsVancPktInsert);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsDecodeEia708bPkt);

/* deprecated start */
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetVideoModeForFrameInfo);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcUtilsGetFrameInfoForVideoMode);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetVideoBytesPerLine);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcGetVancBytesPerLine);
/* deprecated end */


        } while(0);
    }

    return bAllFunctionsAvailable;
}
#endif /* IMPLEMENTATION_BLUEVELVETC_FUNC_PTR */


typedef void* BFC_CONVERSION_HANDLE;

/* FUNCTION PROTOTYPES FOR BLUEVELVETC CONVERSION FUNCTIONS */
#if defined(__cplusplus)
extern "C" {
#endif

    typedef BFC_CONVERSION_HANDLE(__cdecl* pFunc_bfcConversionFactory)();
    typedef BLUE_VOID(__cdecl* pFunc_bfcConversionDestroy)(BFC_CONVERSION_HANDLE pHandle);
    typedef BLUE_S32(__cdecl* pFunc_bfcConversionGetAvailableThreadCount)(BFC_CONVERSION_HANDLE pHandle, BLUE_S32* pMaxThreadCount, BLUE_U32* pCurrentThreadCount);
    typedef BLUE_S32(__cdecl* pFunc_bfcConversionSetThreadCountLimit)(BFC_CONVERSION_HANDLE pHandle, BLUE_U32 MaxThreadCount);
    typedef BLUE_S32(__cdecl* pFunc_bfcConvert_SquareDivisionToTsi_2VUY)(BFC_CONVERSION_HANDLE pHandle, BLUE_U32 Width, BLUE_U32 Height, BLUE_VOID* pSrcBuffer, BLUE_VOID* pDstBuffer);
    typedef BLUE_S32(__cdecl* pFunc_bfcConvert_SquareDivisionToTsi_ARGB32)(BFC_CONVERSION_HANDLE pHandle, BLUE_U32 Width, BLUE_U32 Height, BLUE_VOID* pSrcBuffer, BLUE_VOID* pDstBuffer);
    typedef BLUE_S32(__cdecl* pFunc_bfcConvert_SquareDivisionToTsi_BGR48)(BFC_CONVERSION_HANDLE pHandle, BLUE_U32 Width, BLUE_U32 Height, BLUE_VOID* pSrcBuffer, BLUE_VOID* pDstBuffer);
    typedef BLUE_S32(__cdecl* pFunc_bfcConvert_SquareDivisionToTsi_V210)(BFC_CONVERSION_HANDLE pHandle, BLUE_U32 Width, BLUE_U32 Height, BLUE_VOID* pSrcBuffer, BLUE_VOID* pDstBuffer);
    typedef BLUE_S32(__cdecl* pFunc_bfcConvert_TsiToSquareDivision_2VUY)(BFC_CONVERSION_HANDLE pHandle, BLUE_U32 Width, BLUE_U32 Height, BLUE_VOID* pSrcBuffer, BLUE_VOID* pDstBuffer);
    typedef BLUE_S32(__cdecl* pFunc_bfcConvert_TsiToSquareDivision_V210)(BFC_CONVERSION_HANDLE pHandle, BLUE_U32 Width, BLUE_U32 Height, BLUE_VOID* pSrcBuffer, BLUE_VOID* pDstBuffer);
    typedef BLUE_S32(__cdecl* pFunc_bfcConvert_TsiToSquareDivision_RGB)(BFC_CONVERSION_HANDLE pHandle, BLUE_U32 Width, BLUE_U32 Height, BLUE_VOID* pSrcBuffer, BLUE_VOID* pDstBuffer);
    typedef BLUE_S32(__cdecl* pFunc_bfcConvert_Single4KV210ToAligned4KV210Quadrants_SSE2)(BFC_CONVERSION_HANDLE pHandle, BLUE_VOID* pSrcBuffer, BLUE_VOID* pDstBuffer);
    typedef BLUE_S32(__cdecl* pFunc_bfcConvert_HalfFloatRGBATo16bitRGBA)(BFC_CONVERSION_HANDLE pHandle, BLUE_U32 Width, BLUE_U32 Height, BLUE_VOID* pSrcBuffer, BLUE_VOID* pDstBuffer);
    typedef BLUE_S32(__cdecl* pFunc_bfcConvert_HalfFloatRGBATo16bitRGB)(BFC_CONVERSION_HANDLE pHandle, BLUE_U32 Width, BLUE_U32 Height, BLUE_VOID* pSrcBuffer, BLUE_VOID* pDstBuffer);
    typedef BLUE_S32(__cdecl* pFunc_bfcConvert_FloatRGBATo16bitRGBA)(BFC_CONVERSION_HANDLE pHandle, BLUE_U32 Width, BLUE_U32 Height, BLUE_VOID* pSrcBuffer, BLUE_VOID* pDstBuffer);
    typedef BLUE_S32(__cdecl* pFunc_bfcConvert_FloatRGBATo16bitRGB)(BFC_CONVERSION_HANDLE pHandle, BLUE_U32 Width, BLUE_U32 Height, BLUE_VOID* pSrcBuffer, BLUE_VOID* pDstBuffer);

#if defined(__cplusplus)
} //	extern "C"
#endif

#ifdef IMPLEMENTATION_BLUEVELVETC_CONVERSION_FUNC_PTR
pFunc_bfcConversionFactory                                  bfcConversionFactory;
pFunc_bfcConversionDestroy                                  bfcConversionDestroy;
pFunc_bfcConversionGetAvailableThreadCount                  bfcConversionGetAvailableThreadCount;
pFunc_bfcConversionSetThreadCountLimit                      bfcConversionSetThreadCountLimit;
pFunc_bfcConvert_SquareDivisionToTsi_2VUY                   bfcConvert_SquareDivisionToTsi_2VUY;
pFunc_bfcConvert_SquareDivisionToTsi_ARGB32                 bfcConvert_SquareDivisionToTsi_ARGB32;
pFunc_bfcConvert_SquareDivisionToTsi_BGR48                  bfcConvert_SquareDivisionToTsi_BGR48;
pFunc_bfcConvert_SquareDivisionToTsi_V210                   bfcConvert_SquareDivisionToTsi_V210;
pFunc_bfcConvert_TsiToSquareDivision_2VUY                   bfcConvert_TsiToSquareDivision_2VUY;
pFunc_bfcConvert_TsiToSquareDivision_V210                   bfcConvert_TsiToSquareDivision_V210;
pFunc_bfcConvert_TsiToSquareDivision_RGB                    bfcConvert_TsiToSquareDivision_RGB;
pFunc_bfcConvert_Single4KV210ToAligned4KV210Quadrants_SSE2  bfcConvert_Single4KV210ToAligned4KV210Quadrants_SSE2;
pFunc_bfcConvert_HalfFloatRGBATo16bitRGBA                   bfcConvert_HalfFloatRGBATo16bitRGBA;
pFunc_bfcConvert_HalfFloatRGBATo16bitRGB                    bfcConvert_HalfFloatRGBATo16bitRGB;
pFunc_bfcConvert_FloatRGBATo16bitRGBA                       bfcConvert_FloatRGBATo16bitRGBA;
pFunc_bfcConvert_FloatRGBATo16bitRGB                        bfcConvert_FloatRGBATo16bitRGB;

bool LoadFunctionPointers_BlueConversion()
{
    bool bAllFunctionsAvailable = false;
#if defined(_WIN32)
    #if defined(_WIN64)
        #if defined(_DEBUG)
            HMODULE hLib = LoadLibraryExA("BlueVelvetC64_d.dll", NULL, 0);
        #else
            HMODULE hLib = LoadLibraryExA("BlueVelvetC64.dll", NULL, 0);
        #endif
    #else
        #if defined(_DEBUG)
            HMODULE hLib = LoadLibraryExA("BlueVelvetC_d.dll", NULL, 0);
        #else
            HMODULE hLib = LoadLibraryExA("BlueVelvetC.dll", NULL, 0);
        #endif
    #endif
#endif

    if(hLib)
    {
        bAllFunctionsAvailable = true;
        do
        {
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcConversionFactory);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcConversionDestroy);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcConversionGetAvailableThreadCount);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcConversionSetThreadCountLimit);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcConvert_SquareDivisionToTsi_2VUY);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcConvert_SquareDivisionToTsi_ARGB32);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcConvert_SquareDivisionToTsi_BGR48);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcConvert_SquareDivisionToTsi_V210);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcConvert_TsiToSquareDivision_2VUY);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcConvert_TsiToSquareDivision_V210);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcConvert_TsiToSquareDivision_RGB);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcConvert_Single4KV210ToAligned4KV210Quadrants_SSE2);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcConvert_HalfFloatRGBATo16bitRGBA);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcConvert_HalfFloatRGBATo16bitRGB);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcConvert_FloatRGBATo16bitRGBA);
            GET_PROCADDR_FOR_FUNC(hLib, &bAllFunctionsAvailable, bfcConvert_FloatRGBATo16bitRGB);

        } while(0);
    }

    return bAllFunctionsAvailable;
}
#endif /* IMPLEMENTATION_BLUEVELVETC_CONVERSION_FUNC_PTR */

#endif /* HG_BLUEVELVETC_FUNC_PTR */

