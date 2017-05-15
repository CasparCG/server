//
//  Copyright (c) 2014 Bluefish444. All rights reserved.
//

#ifndef HG_BLUEVELVET_C
#define HG_BLUEVELVET_C


#if defined (_WIN32)
	#include "BlueDriver_p.h"
	#include "BlueHancUtils.h"
    #ifdef BLUEVELVETC_EXPORTS
        #define BLUEVELVETC_API __declspec(dllexport)
    #else
        #define BLUEVELVETC_API __declspec(dllimport)
    #endif
#elif defined (__APPLE__)
    #define DLLEXPORT __attribute__ ((visibility("default")))
    #define DLLLOCAL __attribute__ ((visibility("hidden")))
    #define BLUEVELVETC_API DLLEXPORT

	#include <dispatch/dispatch.h>
    #include "BlueDriver_p.h"
	#include "BlueVelvetCDefines.h"
#elif defined (__linux__)
	#define BLUEVELVETC_API

    #include <BlueDriver_p.h>
	#include "BlueVelvetCDefines.h"
#endif


//----------------------------------------------------------------------------
typedef void* BLUEVELVETC_HANDLE;

typedef int BErr;

#define	BLUE_OK(a)				(!a)		// Test for succcess of a method returning BErr
#define	BLUE_FAIL(a)			(a)			// Test for failure of a method returning BErr
//----------------------------------------------------------------------------


#if defined (_WIN32) || defined (__linux__)
extern "C" {
#endif 

    BLUEVELVETC_API const char* bfcGetVersion();
	BLUEVELVETC_API const wchar_t* bfcGetVersionW();
	BLUEVELVETC_API BLUEVELVETC_HANDLE bfcFactory();
	BLUEVELVETC_API void bfcDestroy(BLUEVELVETC_HANDLE pHandle);
	BLUEVELVETC_API BErr bfcEnumerate(BLUEVELVETC_HANDLE pHandle, int& iDevices);

// Get the card type of the Card indicated by iDeviceID, if iDeviceID is 0, then get the cardtype for that card that this pHandle is attached to.
	BLUEVELVETC_API BErr bfcQueryCardType(BLUEVELVETC_HANDLE pHandle, int& iCardType, int iDeviceID = 0);
	BLUEVELVETC_API BErr bfcAttach(BLUEVELVETC_HANDLE pHandle, int iDeviceId);
	BLUEVELVETC_API BErr bfcDetach(BLUEVELVETC_HANDLE pHandle);
  
	BLUEVELVETC_API BErr bfcQueryCardProperty32(BLUEVELVETC_HANDLE pHandle, const int iProperty, unsigned int& nValue);
    BLUEVELVETC_API BErr bfcSetCardProperty32(BLUEVELVETC_HANDLE pHandle, const int iProperty, const unsigned int nValue);
	BLUEVELVETC_API BErr bfcQueryCardProperty64(BLUEVELVETC_HANDLE pHandle, const int iProperty, unsigned long long& ullValue);
    BLUEVELVETC_API BErr bfcSetCardProperty64(BLUEVELVETC_HANDLE pHandle, const int iProperty, const unsigned long long ullValue);

	BLUEVELVETC_API BErr bfcGetCardSerialNumber(BLUEVELVETC_HANDLE pHandle, char* pSerialNumber, unsigned int nStringSize); //nStringSize must be at least 20
	BLUEVELVETC_API BErr bfcGetCardFwVersion(BLUEVELVETC_HANDLE pHandle, unsigned int& nValue);

	//Interrupt related functions
#if defined(_WIN32)
	BLUEVELVETC_API BErr bfcWaitVideoSyncAsync(BLUEVELVETC_HANDLE pHandle, OVERLAPPED* pOverlap, blue_video_sync_struct* pSyncData);
#endif
	BLUEVELVETC_API BErr bfcWaitVideoInputSync(BLUEVELVETC_HANDLE pHandle, unsigned long ulUpdateType, unsigned long& ulFieldCount);
	BLUEVELVETC_API BErr bfcWaitVideoOutputSync(BLUEVELVETC_HANDLE pHandle, unsigned long ulUpdateType, unsigned long& ulFieldCount);
	BLUEVELVETC_API BErr bfcGetVideoOutputCurrentFieldCount(BLUEVELVETC_HANDLE pHandle, unsigned long& ulFieldCount);
    BLUEVELVETC_API BErr bfcGetVideoInputCurrentFieldCount(BLUEVELVETC_HANDLE pHandle, unsigned long& ulFieldCount);

	//FIFO functions
	BLUEVELVETC_API BErr bfcVideoCaptureStart(BLUEVELVETC_HANDLE pHandle);
	BLUEVELVETC_API BErr bfcVideoCaptureStop(BLUEVELVETC_HANDLE pHandle);
	BLUEVELVETC_API BErr bfcVideoPlaybackStart(BLUEVELVETC_HANDLE pHandle, int iStep, int iLoop);
	BLUEVELVETC_API BErr bfcVideoPlaybackStop(BLUEVELVETC_HANDLE pHandle, int iWait, int iFlush);
	BLUEVELVETC_API BErr bfcVideoPlaybackAllocate(BLUEVELVETC_HANDLE pHandle, void** pAddress, unsigned long& ulBufferID, unsigned long& ulUnderrun);
	BLUEVELVETC_API BErr bfcVideoPlaybackPresent(BLUEVELVETC_HANDLE pHandle, unsigned long& ulUniqueID, unsigned long ulBufferID, unsigned long ulCount, int iKeep, int iOdd=0);
	BLUEVELVETC_API BErr bfcVideoPlaybackRelease(BLUEVELVETC_HANDLE pHandle, unsigned long ulBufferID);
	
#if defined (_WIN32)
	BLUEVELVETC_API BErr bfcGetCaptureVideoFrameInfoEx(BLUEVELVETC_HANDLE pHandle, OVERLAPPED* pOverlap, struct blue_videoframe_info_ex& VideoFrameInfo, int iCompostLater, unsigned int* nCaptureFifoSize);
#elif defined (__APPLE__)
	BLUEVELVETC_API BErr bfcGetCaptureVideoFrameInfoEx(BLUEVELVETC_HANDLE pHandle, struct blue_videoframe_info_ex& VideoFrameInfo, int iCompostLater, unsigned int* nCaptureFifoSize);
#elif defined(__linux__)
	BLUEVELVETC_API BErr bfcGetCaptureVideoFrameInfoEx(BLUEVELVETC_HANDLE pHandle, struct blue_videoframe_info_ex& VideoFrameInfo);
#endif

	//FRAMESTORE functions
	BLUEVELVETC_API BErr bfcRenderBufferCapture(BLUEVELVETC_HANDLE pHandle, unsigned long ulBufferID);
	BLUEVELVETC_API BErr bfcRenderBufferUpdate(BLUEVELVETC_HANDLE pHandle, unsigned long ulBufferID);
	BLUEVELVETC_API BErr bfcGetRenderBufferCount(BLUEVELVETC_HANDLE pHandle, unsigned long& ulCount);

	//AUDIO Helper functions (BlueHancUtils)
	BLUEVELVETC_API BErr bfcEncodeHancFrameEx(BLUEVELVETC_HANDLE pHandle, unsigned int nCardType, struct hanc_stream_info_struct* pHancEncodeInfo, void* pAudioBuffer, unsigned int nAudioChannels, unsigned int nAudioSamples, unsigned int nSampleType, unsigned int nAudioFlags);
	BLUEVELVETC_API BErr bfcDecodeHancFrameEx(BLUEVELVETC_HANDLE pHandle, unsigned int nCardType, unsigned int* pHancBuffer, struct hanc_decode_struct* pHancDecodeInfo);

	//DMA functions
#if defined(_WIN32)
    BLUEVELVETC_API BErr bfcSystemBufferReadAsync(BLUEVELVETC_HANDLE pHandle, unsigned char* pPixels, unsigned long ulSize, OVERLAPPED* pOverlap, unsigned long ulBufferID, unsigned long ulOffset=0);
    BLUEVELVETC_API BErr bfcSystemBufferWriteAsync(BLUEVELVETC_HANDLE pHandle, unsigned char* pPixels, unsigned long ulSize, OVERLAPPED* pOverlap, unsigned long ulBufferID, unsigned long ulOffset=0);
#elif defined(__APPLE__) || defined (__linux__)
    BLUEVELVETC_API BErr bfcSystemBufferRead(BLUEVELVETC_HANDLE pHandle, unsigned char* pPixels, unsigned long ulSize, unsigned long ulBufferID, unsigned long ulOffset=0);
    BLUEVELVETC_API BErr bfcSystemBufferWrite(BLUEVELVETC_HANDLE pHandle, unsigned char* pPixels, unsigned long ulSize, unsigned long ulBufferID, unsigned long ulOffset=0);
#endif

#if defined(__APPLE__)
	BLUEVELVETC_API BErr bfcSystemBufferReadAsync(BLUEVELVETC_HANDLE pHandle, unsigned char* pPixels, unsigned long ulSize, dispatch_semaphore_t* sem, unsigned long ulBufferID, unsigned long ulOffset=0);
    BLUEVELVETC_API BErr bfcSystemBufferWriteAsync(BLUEVELVETC_HANDLE pHandle, unsigned char* pPixels, unsigned long ulSize, dispatch_semaphore_t* sem, unsigned long ulBufferID, unsigned long ulOffset=0);
#endif
	
	// RS422 Serial Port Functions
	BLUEVELVETC_API BErr bfcSerialPortWaitForInputData(BLUEVELVETC_HANDLE pHandle, unsigned int nPortFlags, unsigned int& nBufferLength);
	BLUEVELVETC_API BErr bfcSerialPortRead(BLUEVELVETC_HANDLE pHandle, unsigned int nPortFlags, unsigned char* pBuffer, unsigned int nReadLength);
	BLUEVELVETC_API BErr bfcSerialPortWrite(BLUEVELVETC_HANDLE pHandle, unsigned int nPortFlags, unsigned char* pBuffer, unsigned int nWriteLength);

#if defined (_WIN32)
	//Miscellaneous functions
	BLUEVELVETC_API BErr bfcGetReferenceClockPhaseSettings(BLUEVELVETC_HANDLE pHandle, unsigned int& nHPhase, unsigned int& nVPhase, unsigned int& nHPhaseMax, unsigned int& nVPhaseMax);
	BLUEVELVETC_API BErr bfcLoadOutputLUT1D(BLUEVELVETC_HANDLE pHandle, struct blue_1d_lookup_table_struct* pLutData);
	BLUEVELVETC_API BErr bfcControlVideoScaler(BLUEVELVETC_HANDLE pHandle,	unsigned int nScalerId,
																			bool bOnlyReadValue,
																			float* pSrcVideoHeight,
																			float* pSrcVideoWidth,
																			float* pSrcVideoYPos,
																			float* pSrcVideoXPos,
																			float* pDestVideoHeight,
																			float* pDestVideoWidth,
																			float* pDestVideoYPos,
																			float* pDestVideoXPos);
#endif

    // Video mode and Format information functions
	BLUEVELVETC_API BErr bfcGetBytesForGroupPixels(EMemoryFormat nMemoryFormat, unsigned int nVideoWidth, unsigned int& nVideoPitchBytes);
	BLUEVELVETC_API BErr bfcGetPixelsPerLine(EVideoMode nVideoMode, unsigned int& nPixelsPerLine);
    BLUEVELVETC_API BErr bfcGetLinesPerFrame(EVideoMode nVideoMode, EUpdateMethod nUpdateMethod, unsigned int& nLinesPerFrame);
    BLUEVELVETC_API BErr bfcGetBytesPerLine(EVideoMode nVideoMode, EMemoryFormat nMemoryFormat, unsigned int& nBytesPerLine);
    BLUEVELVETC_API BErr bfcGetBytesPerFrame(EVideoMode nVideoMode, EMemoryFormat nMemoryFormat, EUpdateMethod nUpdateMethod, unsigned int& nBytesPerFrame);
    BLUEVELVETC_API BErr bfcGetGoldenValue(EVideoMode nVideoMode, EMemoryFormat nMemoryFormat, EUpdateMethod nUpdateMethod, unsigned int& nGoldenFrameSize);
    BLUEVELVETC_API BErr bfcGetVBILines(EVideoMode nVideoMode, EUpdateMethod nUpdateMethod, unsigned int& nVBILinesPerFrame);
    
    BLUEVELVETC_API BErr bfcGetVANCGoldenValue(unsigned int nCardType, EVideoMode nVideoMode, EMemoryFormat nMemoryFormat, EDMADataType nDataFormat, unsigned int& nVANCGoldenValue);
    BLUEVELVETC_API BErr bfcGetVANCLineBytes(unsigned int nCardType, EVideoMode nVideoMode, EMemoryFormat nMemoryFormat, unsigned int& nVANCLineBytes);
    BLUEVELVETC_API BErr bfcGetVANCLineCount(unsigned int nCardType, EVideoMode nVideoMode, EDMADataType nDataFormat, unsigned int& nVANCLineCount);
    

#if defined (_WIN32)	
	//AMD SDI-Link support
	BLUEVELVETC_API BErr bfcDmaWaitMarker(BLUEVELVETC_HANDLE pHandle,	OVERLAPPED* pOverlap,
																	unsigned int nVideoChannel,
																	unsigned int nBufferId,
																	unsigned int nMarker);
	BLUEVELVETC_API BErr bfcDmaReadToBusAddressWithMarker(	BLUEVELVETC_HANDLE pHandle,
															unsigned int nVideoChannel,
															unsigned long long n64DataAddress,
															unsigned int nSize,
															OVERLAPPED* pOverlap,
															unsigned int BufferID,
															unsigned long Offset,
															unsigned long long n64MarkerAddress,
															unsigned int nMarker);
	BLUEVELVETC_API BErr bfcDmaReadToBusAddress(	BLUEVELVETC_HANDLE pHandle,
												unsigned int nVideoChannel,
												unsigned long long n64DataAddress,
												unsigned int nSize,
												OVERLAPPED* pOverlap,
												unsigned int BufferID,
												unsigned long Offset);
	BLUEVELVETC_API BErr bfcDmaWriteMarker(	BLUEVELVETC_HANDLE pHandle,
											unsigned long long n64MarkerAddress,
											unsigned int nMarker);

	//misc
	BLUEVELVETC_API BErr bfcGetWindowsDriverHandle(BLUEVELVETC_HANDLE pHandle, HANDLE* pDriverHandle);
	BLUEVELVETC_API BErr bfcSetDynamicMemoryFormatChange(BLUEVELVETC_HANDLE pHandle, OVERLAPPED* pOverlap, unsigned int nUniqueId, EMemoryFormat nMemoryFormat);
#endif


#if defined (_WIN32) || defined (__linux__)
} //extern "C"
#endif

#endif //HG_BLUEVELVET_C
