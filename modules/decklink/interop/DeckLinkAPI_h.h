

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0500 */
/* at Wed Jan 13 09:58:01 2010
 */
/* Compiler settings for .\consumers\declink\DeckLinkAPI.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __DeckLinkAPI_h_h__
#define __DeckLinkAPI_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDeckLinkVideoOutputCallback_FWD_DEFINED__
#define __IDeckLinkVideoOutputCallback_FWD_DEFINED__
typedef interface IDeckLinkVideoOutputCallback IDeckLinkVideoOutputCallback;
#endif 	/* __IDeckLinkVideoOutputCallback_FWD_DEFINED__ */


#ifndef __IDeckLinkInputCallback_FWD_DEFINED__
#define __IDeckLinkInputCallback_FWD_DEFINED__
typedef interface IDeckLinkInputCallback IDeckLinkInputCallback;
#endif 	/* __IDeckLinkInputCallback_FWD_DEFINED__ */


#ifndef __IDeckLinkMemoryAllocator_FWD_DEFINED__
#define __IDeckLinkMemoryAllocator_FWD_DEFINED__
typedef interface IDeckLinkMemoryAllocator IDeckLinkMemoryAllocator;
#endif 	/* __IDeckLinkMemoryAllocator_FWD_DEFINED__ */


#ifndef __IDeckLinkAudioOutputCallback_FWD_DEFINED__
#define __IDeckLinkAudioOutputCallback_FWD_DEFINED__
typedef interface IDeckLinkAudioOutputCallback IDeckLinkAudioOutputCallback;
#endif 	/* __IDeckLinkAudioOutputCallback_FWD_DEFINED__ */


#ifndef __IDeckLinkIterator_FWD_DEFINED__
#define __IDeckLinkIterator_FWD_DEFINED__
typedef interface IDeckLinkIterator IDeckLinkIterator;
#endif 	/* __IDeckLinkIterator_FWD_DEFINED__ */


#ifndef __IDeckLinkAPIInformation_FWD_DEFINED__
#define __IDeckLinkAPIInformation_FWD_DEFINED__
typedef interface IDeckLinkAPIInformation IDeckLinkAPIInformation;
#endif 	/* __IDeckLinkAPIInformation_FWD_DEFINED__ */


#ifndef __IDeckLinkDisplayModeIterator_FWD_DEFINED__
#define __IDeckLinkDisplayModeIterator_FWD_DEFINED__
typedef interface IDeckLinkDisplayModeIterator IDeckLinkDisplayModeIterator;
#endif 	/* __IDeckLinkDisplayModeIterator_FWD_DEFINED__ */


#ifndef __IDeckLinkDisplayMode_FWD_DEFINED__
#define __IDeckLinkDisplayMode_FWD_DEFINED__
typedef interface IDeckLinkDisplayMode IDeckLinkDisplayMode;
#endif 	/* __IDeckLinkDisplayMode_FWD_DEFINED__ */


#ifndef __IDeckLink_FWD_DEFINED__
#define __IDeckLink_FWD_DEFINED__
typedef interface IDeckLink IDeckLink;
#endif 	/* __IDeckLink_FWD_DEFINED__ */


#ifndef __IDeckLinkOutput_FWD_DEFINED__
#define __IDeckLinkOutput_FWD_DEFINED__
typedef interface IDeckLinkOutput IDeckLinkOutput;
#endif 	/* __IDeckLinkOutput_FWD_DEFINED__ */


#ifndef __IDeckLinkInput_FWD_DEFINED__
#define __IDeckLinkInput_FWD_DEFINED__
typedef interface IDeckLinkInput IDeckLinkInput;
#endif 	/* __IDeckLinkInput_FWD_DEFINED__ */


#ifndef __IDeckLinkTimecode_FWD_DEFINED__
#define __IDeckLinkTimecode_FWD_DEFINED__
typedef interface IDeckLinkTimecode IDeckLinkTimecode;
#endif 	/* __IDeckLinkTimecode_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoFrame_FWD_DEFINED__
#define __IDeckLinkVideoFrame_FWD_DEFINED__
typedef interface IDeckLinkVideoFrame IDeckLinkVideoFrame;
#endif 	/* __IDeckLinkVideoFrame_FWD_DEFINED__ */


#ifndef __IDeckLinkMutableVideoFrame_FWD_DEFINED__
#define __IDeckLinkMutableVideoFrame_FWD_DEFINED__
typedef interface IDeckLinkMutableVideoFrame IDeckLinkMutableVideoFrame;
#endif 	/* __IDeckLinkMutableVideoFrame_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoInputFrame_FWD_DEFINED__
#define __IDeckLinkVideoInputFrame_FWD_DEFINED__
typedef interface IDeckLinkVideoInputFrame IDeckLinkVideoInputFrame;
#endif 	/* __IDeckLinkVideoInputFrame_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoFrameAncillary_FWD_DEFINED__
#define __IDeckLinkVideoFrameAncillary_FWD_DEFINED__
typedef interface IDeckLinkVideoFrameAncillary IDeckLinkVideoFrameAncillary;
#endif 	/* __IDeckLinkVideoFrameAncillary_FWD_DEFINED__ */


#ifndef __IDeckLinkAudioInputPacket_FWD_DEFINED__
#define __IDeckLinkAudioInputPacket_FWD_DEFINED__
typedef interface IDeckLinkAudioInputPacket IDeckLinkAudioInputPacket;
#endif 	/* __IDeckLinkAudioInputPacket_FWD_DEFINED__ */


#ifndef __IDeckLinkScreenPreviewCallback_FWD_DEFINED__
#define __IDeckLinkScreenPreviewCallback_FWD_DEFINED__
typedef interface IDeckLinkScreenPreviewCallback IDeckLinkScreenPreviewCallback;
#endif 	/* __IDeckLinkScreenPreviewCallback_FWD_DEFINED__ */


#ifndef __IDeckLinkGLScreenPreviewHelper_FWD_DEFINED__
#define __IDeckLinkGLScreenPreviewHelper_FWD_DEFINED__
typedef interface IDeckLinkGLScreenPreviewHelper IDeckLinkGLScreenPreviewHelper;
#endif 	/* __IDeckLinkGLScreenPreviewHelper_FWD_DEFINED__ */


#ifndef __IDeckLinkConfiguration_FWD_DEFINED__
#define __IDeckLinkConfiguration_FWD_DEFINED__
typedef interface IDeckLinkConfiguration IDeckLinkConfiguration;
#endif 	/* __IDeckLinkConfiguration_FWD_DEFINED__ */


#ifndef __IDeckLinkAttributes_FWD_DEFINED__
#define __IDeckLinkAttributes_FWD_DEFINED__
typedef interface IDeckLinkAttributes IDeckLinkAttributes;
#endif 	/* __IDeckLinkAttributes_FWD_DEFINED__ */


#ifndef __IDeckLinkKeyer_FWD_DEFINED__
#define __IDeckLinkKeyer_FWD_DEFINED__
typedef interface IDeckLinkKeyer IDeckLinkKeyer;
#endif 	/* __IDeckLinkKeyer_FWD_DEFINED__ */


#ifndef __CDeckLinkIterator_FWD_DEFINED__
#define __CDeckLinkIterator_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkIterator CDeckLinkIterator;
#else
typedef struct CDeckLinkIterator CDeckLinkIterator;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkIterator_FWD_DEFINED__ */


#ifndef __CDeckLinkGLScreenPreviewHelper_FWD_DEFINED__
#define __CDeckLinkGLScreenPreviewHelper_FWD_DEFINED__

#ifdef __cplusplus
typedef class CDeckLinkGLScreenPreviewHelper CDeckLinkGLScreenPreviewHelper;
#else
typedef struct CDeckLinkGLScreenPreviewHelper CDeckLinkGLScreenPreviewHelper;
#endif /* __cplusplus */

#endif 	/* __CDeckLinkGLScreenPreviewHelper_FWD_DEFINED__ */


#ifndef __IDeckLinkDisplayModeIterator_v7_1_FWD_DEFINED__
#define __IDeckLinkDisplayModeIterator_v7_1_FWD_DEFINED__
typedef interface IDeckLinkDisplayModeIterator_v7_1 IDeckLinkDisplayModeIterator_v7_1;
#endif 	/* __IDeckLinkDisplayModeIterator_v7_1_FWD_DEFINED__ */


#ifndef __IDeckLinkDisplayMode_v7_1_FWD_DEFINED__
#define __IDeckLinkDisplayMode_v7_1_FWD_DEFINED__
typedef interface IDeckLinkDisplayMode_v7_1 IDeckLinkDisplayMode_v7_1;
#endif 	/* __IDeckLinkDisplayMode_v7_1_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoFrame_v7_1_FWD_DEFINED__
#define __IDeckLinkVideoFrame_v7_1_FWD_DEFINED__
typedef interface IDeckLinkVideoFrame_v7_1 IDeckLinkVideoFrame_v7_1;
#endif 	/* __IDeckLinkVideoFrame_v7_1_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoInputFrame_v7_1_FWD_DEFINED__
#define __IDeckLinkVideoInputFrame_v7_1_FWD_DEFINED__
typedef interface IDeckLinkVideoInputFrame_v7_1 IDeckLinkVideoInputFrame_v7_1;
#endif 	/* __IDeckLinkVideoInputFrame_v7_1_FWD_DEFINED__ */


#ifndef __IDeckLinkAudioInputPacket_v7_1_FWD_DEFINED__
#define __IDeckLinkAudioInputPacket_v7_1_FWD_DEFINED__
typedef interface IDeckLinkAudioInputPacket_v7_1 IDeckLinkAudioInputPacket_v7_1;
#endif 	/* __IDeckLinkAudioInputPacket_v7_1_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoOutputCallback_v7_1_FWD_DEFINED__
#define __IDeckLinkVideoOutputCallback_v7_1_FWD_DEFINED__
typedef interface IDeckLinkVideoOutputCallback_v7_1 IDeckLinkVideoOutputCallback_v7_1;
#endif 	/* __IDeckLinkVideoOutputCallback_v7_1_FWD_DEFINED__ */


#ifndef __IDeckLinkInputCallback_v7_1_FWD_DEFINED__
#define __IDeckLinkInputCallback_v7_1_FWD_DEFINED__
typedef interface IDeckLinkInputCallback_v7_1 IDeckLinkInputCallback_v7_1;
#endif 	/* __IDeckLinkInputCallback_v7_1_FWD_DEFINED__ */


#ifndef __IDeckLinkOutput_v7_1_FWD_DEFINED__
#define __IDeckLinkOutput_v7_1_FWD_DEFINED__
typedef interface IDeckLinkOutput_v7_1 IDeckLinkOutput_v7_1;
#endif 	/* __IDeckLinkOutput_v7_1_FWD_DEFINED__ */


#ifndef __IDeckLinkInput_v7_1_FWD_DEFINED__
#define __IDeckLinkInput_v7_1_FWD_DEFINED__
typedef interface IDeckLinkInput_v7_1 IDeckLinkInput_v7_1;
#endif 	/* __IDeckLinkInput_v7_1_FWD_DEFINED__ */


#ifndef __IDeckLinkInputCallback_v7_3_FWD_DEFINED__
#define __IDeckLinkInputCallback_v7_3_FWD_DEFINED__
typedef interface IDeckLinkInputCallback_v7_3 IDeckLinkInputCallback_v7_3;
#endif 	/* __IDeckLinkInputCallback_v7_3_FWD_DEFINED__ */


#ifndef __IDeckLinkOutput_v7_3_FWD_DEFINED__
#define __IDeckLinkOutput_v7_3_FWD_DEFINED__
typedef interface IDeckLinkOutput_v7_3 IDeckLinkOutput_v7_3;
#endif 	/* __IDeckLinkOutput_v7_3_FWD_DEFINED__ */


#ifndef __IDeckLinkInput_v7_3_FWD_DEFINED__
#define __IDeckLinkInput_v7_3_FWD_DEFINED__
typedef interface IDeckLinkInput_v7_3 IDeckLinkInput_v7_3;
#endif 	/* __IDeckLinkInput_v7_3_FWD_DEFINED__ */


#ifndef __IDeckLinkVideoInputFrame_v7_3_FWD_DEFINED__
#define __IDeckLinkVideoInputFrame_v7_3_FWD_DEFINED__
typedef interface IDeckLinkVideoInputFrame_v7_3 IDeckLinkVideoInputFrame_v7_3;
#endif 	/* __IDeckLinkVideoInputFrame_v7_3_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __DeckLinkAPI_LIBRARY_DEFINED__
#define __DeckLinkAPI_LIBRARY_DEFINED__

/* library DeckLinkAPI */
/* [helpstring][version][uuid] */ 

typedef LONGLONG BMDTimeValue;

typedef LONGLONG BMDTimeScale;

typedef unsigned long BMDTimecodeBCD;

typedef unsigned long BMDFrameFlags;
typedef unsigned long BMDVideoInputFlags;
typedef unsigned long BMDVideoInputFormatChangedEvents;
typedef unsigned long BMDDetectedVideoInputFormatFlags;
typedef unsigned long BMDTimecodeFlags;
typedef unsigned long BMDAnalogVideoFlags;
#if 0
typedef enum _BMDFrameFlags BMDFrameFlags;

typedef enum _BMDVideoInputFlags BMDVideoInputFlags;

typedef enum _BMDVideoInputFormatChangedEvents BMDVideoInputFormatChangedEvents;

typedef enum _BMDDetectedVideoInputFormatFlags BMDDetectedVideoInputFormatFlags;

typedef enum _BMDTimecodeFlags BMDTimecodeFlags;

typedef enum _BMDAnalogVideoFlags BMDAnalogVideoFlags;

#endif
typedef /* [v1_enum] */ 
enum _BMDDisplayMode
    {	bmdModeNTSC	= 0x6e747363,
	bmdModeNTSC2398	= 0x6e743233,
	bmdModePAL	= 0x70616c20,
	bmdModeHD1080p2398	= 0x32337073,
	bmdModeHD1080p24	= 0x32347073,
	bmdModeHD1080p25	= 0x48703235,
	bmdModeHD1080p2997	= 0x48703239,
	bmdModeHD1080p30	= 0x48703330,
	bmdModeHD1080i50	= 0x48693530,
	bmdModeHD1080i5994	= 0x48693539,
	bmdModeHD1080i6000	= 0x48693630,
	bmdModeHD1080p50	= 0x48703530,
	bmdModeHD1080p5994	= 0x48703539,
	bmdModeHD1080p6000	= 0x48703630,
	bmdModeHD720p50	= 0x68703530,
	bmdModeHD720p5994	= 0x68703539,
	bmdModeHD720p60	= 0x68703630,
	bmdMode2k2398	= 0x326b3233,
	bmdMode2k24	= 0x326b3234,
	bmdMode2k25	= 0x326b3235
    } 	BMDDisplayMode;

typedef /* [v1_enum] */ 
enum _BMDFieldDominance
    {	bmdUnknownFieldDominance	= 0,
	bmdLowerFieldFirst	= 0x6c6f7772,
	bmdUpperFieldFirst	= 0x75707072,
	bmdProgressiveFrame	= 0x70726f67,
	bmdProgressiveSegmentedFrame	= 0x70736620
    } 	BMDFieldDominance;

typedef /* [v1_enum] */ 
enum _BMDPixelFormat
    {	bmdFormat8BitYUV	= 0x32767579,
	bmdFormat10BitYUV	= 0x76323130,
	bmdFormat8BitARGB	= 0x20,
	bmdFormat8BitBGRA	= 0x42475241,
	bmdFormat10BitRGB	= 0x72323130
    } 	BMDPixelFormat;

typedef /* [v1_enum] */ 
enum _BMDVideoOutputFlags
    {	bmdVideoOutputFlagDefault	= 0,
	bmdVideoOutputRP188	= ( 1 << 0 ) ,
	bmdVideoOutputVANC	= ( 1 << 1 ) 
    } 	BMDVideoOutputFlags;

/* [v1_enum] */ 
enum _BMDFrameFlags
    {	bmdFrameFlagDefault	= 0,
	bmdFrameFlagFlipVertical	= ( 1 << 0 ) ,
	bmdFrameHasNoInputSource	= ( 1 << 31 ) 
    } ;
/* [v1_enum] */ 
enum _BMDVideoInputFlags
    {	bmdVideoInputFlagDefault	= 0,
	bmdVideoInputEnableFormatDetection	= ( 1 << 0 ) 
    } ;
/* [v1_enum] */ 
enum _BMDVideoInputFormatChangedEvents
    {	bmdVideoInputDisplayModeChanged	= ( 1 << 0 ) ,
	bmdVideoInputFieldDominanceChanged	= ( 1 << 1 ) ,
	bmdVideoInputColorspaceChanged	= ( 1 << 2 ) 
    } ;
/* [v1_enum] */ 
enum _BMDDetectedVideoInputFormatFlags
    {	bmdDetectedVideoInputYCbCr422	= ( 1 << 0 ) ,
	bmdDetectedVideoInputRGB444	= ( 1 << 1 ) 
    } ;
typedef /* [v1_enum] */ 
enum _BMDOutputFrameCompletionResult
    {	bmdOutputFrameCompleted	= 0,
	bmdOutputFrameDisplayedLate	= ( bmdOutputFrameCompleted + 1 ) ,
	bmdOutputFrameDropped	= ( bmdOutputFrameDisplayedLate + 1 ) ,
	bmdOutputFrameFlushed	= ( bmdOutputFrameDropped + 1 ) 
    } 	BMDOutputFrameCompletionResult;

typedef /* [v1_enum] */ 
enum _BMDAudioSampleRate
    {	bmdAudioSampleRate48kHz	= 48000
    } 	BMDAudioSampleRate;

typedef /* [v1_enum] */ 
enum _BMDAudioSampleType
    {	bmdAudioSampleType16bitInteger	= 16,
	bmdAudioSampleType32bitInteger	= 32
    } 	BMDAudioSampleType;

typedef /* [v1_enum] */ 
enum _BMDAudioOutputStreamType
    {	bmdAudioOutputStreamContinuous	= 0,
	bmdAudioOutputStreamContinuousDontResample	= ( bmdAudioOutputStreamContinuous + 1 ) ,
	bmdAudioOutputStreamTimestamped	= ( bmdAudioOutputStreamContinuousDontResample + 1 ) 
    } 	BMDAudioOutputStreamType;

typedef /* [v1_enum] */ 
enum _BMDDisplayModeSupport
    {	bmdDisplayModeNotSupported	= 0,
	bmdDisplayModeSupported	= ( bmdDisplayModeNotSupported + 1 ) ,
	bmdDisplayModeSupportedWithConversion	= ( bmdDisplayModeSupported + 1 ) 
    } 	BMDDisplayModeSupport;

typedef /* [v1_enum] */ 
enum _BMDTimecodeFormat
    {	bmdTimecodeRP188	= 0x72703138,
	bmdTimecodeVITC	= 0x76697463,
	bmdTimecodeSerial	= 0x73657269
    } 	BMDTimecodeFormat;

/* [v1_enum] */ 
enum _BMDTimecodeFlags
    {	bmdTimecodeFlagDefault	= 0,
	bmdTimecodeIsDropFrame	= ( 1 << 0 ) 
    } ;
typedef /* [v1_enum] */ 
enum _BMDVideoConnection
    {	bmdVideoConnectionSDI	= 0x73646920,
	bmdVideoConnectionHDMI	= 0x68646d69,
	bmdVideoConnectionOpticalSDI	= 0x6f707469,
	bmdVideoConnectionComponent	= 0x63706e74,
	bmdVideoConnectionComposite	= 0x636d7374,
	bmdVideoConnectionSVideo	= 0x73766964
    } 	BMDVideoConnection;

/* [v1_enum] */ 
enum _BMDAnalogVideoFlags
    {	bmdAnalogVideoFlagCompositeSetup75	= ( 1 << 0 ) ,
	bmdAnalogVideoFlagComponentBetacamLevels	= ( 1 << 1 ) 
    } ;
typedef /* [v1_enum] */ 
enum _BMDAudioConnection
    {	bmdAudioConnectionEmbedded	= 0x656d6264,
	bmdAudioConnectionAESEBU	= 0x61657320,
	bmdAudioConnectionAnalog	= 0x616e6c67
    } 	BMDAudioConnection;

typedef /* [v1_enum] */ 
enum _BMDVideoOutputConversionMode
    {	bmdNoVideoOutputConversion	= 0x6e6f6e65,
	bmdVideoOutputLetterboxDownonversion	= 0x6c746278,
	bmdVideoOutputAnamorphicDownonversion	= 0x616d7068,
	bmdVideoOutputHD720toHD1080Conversion	= 0x37323063,
	bmdVideoOutputHardwareLetterboxDownconversion	= 0x48576c62,
	bmdVideoOutputHardwareAnamorphicDownconversion	= 0x4857616d,
	bmdVideoOutputHardwareCenterCutDownconversion	= 0x48576363
    } 	BMDVideoOutputConversionMode;

typedef /* [v1_enum] */ 
enum _BMDVideoInputConversionMode
    {	bmdNoVideoInputConversion	= 0x6e6f6e65,
	bmdVideoInputLetterboxDownconversionFromHD1080	= 0x31306c62,
	bmdVideoInputAnamorphicDownconversionFromHD1080	= 0x3130616d,
	bmdVideoInputLetterboxDownconversionFromHD720	= 0x37326c62,
	bmdVideoInputAnamorphicDownconversionFromHD720	= 0x3732616d,
	bmdVideoInputLetterboxUpconversion	= 0x6c627570,
	bmdVideoInputAnamorphicUpconversion	= 0x616d7570
    } 	BMDVideoInputConversionMode;

typedef /* [v1_enum] */ 
enum _BMDDeckLinkAttributeID
    {	BMDDeckLinkSupportsInternalKeying	= 0x6b657969,
	BMDDeckLinkSupportsExternalKeying	= 0x6b657965,
	BMDDeckLinkSupportsHDKeying	= 0x6b657968,
	BMDDeckLinkSupportsInputFormatDetection	= 0x696e6664,
	BMDDeckLinkHasSerialPort	= 0x68737074,
	BMDDeckLinkMaximumAudioChannels	= 0x6d616368,
	BMDDeckLinkSerialPortDeviceName	= 0x736c706e
    } 	BMDDeckLinkAttributeID;

typedef /* [v1_enum] */ 
enum _BMDDeckLinkAPIInformationID
    {	BMDDeckLinkAPIVersion	= 0x76657273
    } 	BMDDeckLinkAPIInformationID;

































EXTERN_C const IID LIBID_DeckLinkAPI;

#ifndef __IDeckLinkVideoOutputCallback_INTERFACE_DEFINED__
#define __IDeckLinkVideoOutputCallback_INTERFACE_DEFINED__

/* interface IDeckLinkVideoOutputCallback */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoOutputCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E763A626-4A3C-49D1-BF13-E7AD3692AE52")
    IDeckLinkVideoOutputCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ScheduledFrameCompleted( 
            /* [in] */ IDeckLinkVideoFrame *completedFrame,
            /* [in] */ BMDOutputFrameCompletionResult result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduledPlaybackHasStopped( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoOutputCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoOutputCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoOutputCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoOutputCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *ScheduledFrameCompleted )( 
            IDeckLinkVideoOutputCallback * This,
            /* [in] */ IDeckLinkVideoFrame *completedFrame,
            /* [in] */ BMDOutputFrameCompletionResult result);
        
        HRESULT ( STDMETHODCALLTYPE *ScheduledPlaybackHasStopped )( 
            IDeckLinkVideoOutputCallback * This);
        
        END_INTERFACE
    } IDeckLinkVideoOutputCallbackVtbl;

    interface IDeckLinkVideoOutputCallback
    {
        CONST_VTBL struct IDeckLinkVideoOutputCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoOutputCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoOutputCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoOutputCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoOutputCallback_ScheduledFrameCompleted(This,completedFrame,result)	\
    ( (This)->lpVtbl -> ScheduledFrameCompleted(This,completedFrame,result) ) 

#define IDeckLinkVideoOutputCallback_ScheduledPlaybackHasStopped(This)	\
    ( (This)->lpVtbl -> ScheduledPlaybackHasStopped(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoOutputCallback_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkInputCallback_INTERFACE_DEFINED__
#define __IDeckLinkInputCallback_INTERFACE_DEFINED__

/* interface IDeckLinkInputCallback */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkInputCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("31D28EE7-88B6-4CB1-897A-CDBF79A26414")
    IDeckLinkInputCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged( 
            /* [in] */ BMDVideoInputFormatChangedEvents notificationEvents,
            /* [in] */ IDeckLinkDisplayMode *newDisplayMode,
            /* [in] */ BMDDetectedVideoInputFormatFlags detectedSignalFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived( 
            /* [in] */ IDeckLinkVideoInputFrame *videoFrame,
            /* [in] */ IDeckLinkAudioInputPacket *audioPacket) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkInputCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkInputCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkInputCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkInputCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *VideoInputFormatChanged )( 
            IDeckLinkInputCallback * This,
            /* [in] */ BMDVideoInputFormatChangedEvents notificationEvents,
            /* [in] */ IDeckLinkDisplayMode *newDisplayMode,
            /* [in] */ BMDDetectedVideoInputFormatFlags detectedSignalFlags);
        
        HRESULT ( STDMETHODCALLTYPE *VideoInputFrameArrived )( 
            IDeckLinkInputCallback * This,
            /* [in] */ IDeckLinkVideoInputFrame *videoFrame,
            /* [in] */ IDeckLinkAudioInputPacket *audioPacket);
        
        END_INTERFACE
    } IDeckLinkInputCallbackVtbl;

    interface IDeckLinkInputCallback
    {
        CONST_VTBL struct IDeckLinkInputCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkInputCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkInputCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkInputCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkInputCallback_VideoInputFormatChanged(This,notificationEvents,newDisplayMode,detectedSignalFlags)	\
    ( (This)->lpVtbl -> VideoInputFormatChanged(This,notificationEvents,newDisplayMode,detectedSignalFlags) ) 

#define IDeckLinkInputCallback_VideoInputFrameArrived(This,videoFrame,audioPacket)	\
    ( (This)->lpVtbl -> VideoInputFrameArrived(This,videoFrame,audioPacket) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkInputCallback_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkMemoryAllocator_INTERFACE_DEFINED__
#define __IDeckLinkMemoryAllocator_INTERFACE_DEFINED__

/* interface IDeckLinkMemoryAllocator */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkMemoryAllocator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B36EB6E7-9D29-4AA8-92EF-843B87A289E8")
    IDeckLinkMemoryAllocator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE AllocateBuffer( 
            unsigned long bufferSize,
            /* [out] */ void **allocatedBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseBuffer( 
            /* [in] */ void *buffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Commit( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Decommit( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkMemoryAllocatorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkMemoryAllocator * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkMemoryAllocator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkMemoryAllocator * This);
        
        HRESULT ( STDMETHODCALLTYPE *AllocateBuffer )( 
            IDeckLinkMemoryAllocator * This,
            unsigned long bufferSize,
            /* [out] */ void **allocatedBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseBuffer )( 
            IDeckLinkMemoryAllocator * This,
            /* [in] */ void *buffer);
        
        HRESULT ( STDMETHODCALLTYPE *Commit )( 
            IDeckLinkMemoryAllocator * This);
        
        HRESULT ( STDMETHODCALLTYPE *Decommit )( 
            IDeckLinkMemoryAllocator * This);
        
        END_INTERFACE
    } IDeckLinkMemoryAllocatorVtbl;

    interface IDeckLinkMemoryAllocator
    {
        CONST_VTBL struct IDeckLinkMemoryAllocatorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkMemoryAllocator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkMemoryAllocator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkMemoryAllocator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkMemoryAllocator_AllocateBuffer(This,bufferSize,allocatedBuffer)	\
    ( (This)->lpVtbl -> AllocateBuffer(This,bufferSize,allocatedBuffer) ) 

#define IDeckLinkMemoryAllocator_ReleaseBuffer(This,buffer)	\
    ( (This)->lpVtbl -> ReleaseBuffer(This,buffer) ) 

#define IDeckLinkMemoryAllocator_Commit(This)	\
    ( (This)->lpVtbl -> Commit(This) ) 

#define IDeckLinkMemoryAllocator_Decommit(This)	\
    ( (This)->lpVtbl -> Decommit(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkMemoryAllocator_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkAudioOutputCallback_INTERFACE_DEFINED__
#define __IDeckLinkAudioOutputCallback_INTERFACE_DEFINED__

/* interface IDeckLinkAudioOutputCallback */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkAudioOutputCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("403C681B-7F46-4A12-B993-2BB127084EE6")
    IDeckLinkAudioOutputCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE RenderAudioSamples( 
            BOOL preroll) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkAudioOutputCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkAudioOutputCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkAudioOutputCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkAudioOutputCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *RenderAudioSamples )( 
            IDeckLinkAudioOutputCallback * This,
            BOOL preroll);
        
        END_INTERFACE
    } IDeckLinkAudioOutputCallbackVtbl;

    interface IDeckLinkAudioOutputCallback
    {
        CONST_VTBL struct IDeckLinkAudioOutputCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkAudioOutputCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkAudioOutputCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkAudioOutputCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkAudioOutputCallback_RenderAudioSamples(This,preroll)	\
    ( (This)->lpVtbl -> RenderAudioSamples(This,preroll) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkAudioOutputCallback_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkIterator_INTERFACE_DEFINED__
#define __IDeckLinkIterator_INTERFACE_DEFINED__

/* interface IDeckLinkIterator */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkIterator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("74E936FC-CC28-4A67-81A0-1E94E52D4E69")
    IDeckLinkIterator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [out] */ IDeckLink **deckLinkInstance) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkIteratorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkIterator * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkIterator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkIterator * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDeckLinkIterator * This,
            /* [out] */ IDeckLink **deckLinkInstance);
        
        END_INTERFACE
    } IDeckLinkIteratorVtbl;

    interface IDeckLinkIterator
    {
        CONST_VTBL struct IDeckLinkIteratorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkIterator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkIterator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkIterator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkIterator_Next(This,deckLinkInstance)	\
    ( (This)->lpVtbl -> Next(This,deckLinkInstance) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkIterator_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkAPIInformation_INTERFACE_DEFINED__
#define __IDeckLinkAPIInformation_INTERFACE_DEFINED__

/* interface IDeckLinkAPIInformation */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkAPIInformation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7BEA3C68-730D-4322-AF34-8A7152B532A4")
    IDeckLinkAPIInformation : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDDeckLinkAPIInformationID cfgID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkAPIInformationID cfgID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDDeckLinkAPIInformationID cfgID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDDeckLinkAPIInformationID cfgID,
            /* [out] */ BSTR *value) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkAPIInformationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkAPIInformation * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkAPIInformation * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkAPIInformation * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkAPIInformation * This,
            /* [in] */ BMDDeckLinkAPIInformationID cfgID,
            /* [out] */ BOOL *value);
        
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkAPIInformation * This,
            /* [in] */ BMDDeckLinkAPIInformationID cfgID,
            /* [out] */ LONGLONG *value);
        
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkAPIInformation * This,
            /* [in] */ BMDDeckLinkAPIInformationID cfgID,
            /* [out] */ double *value);
        
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkAPIInformation * This,
            /* [in] */ BMDDeckLinkAPIInformationID cfgID,
            /* [out] */ BSTR *value);
        
        END_INTERFACE
    } IDeckLinkAPIInformationVtbl;

    interface IDeckLinkAPIInformation
    {
        CONST_VTBL struct IDeckLinkAPIInformationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkAPIInformation_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkAPIInformation_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkAPIInformation_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkAPIInformation_GetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,cfgID,value) ) 

#define IDeckLinkAPIInformation_GetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetInt(This,cfgID,value) ) 

#define IDeckLinkAPIInformation_GetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,cfgID,value) ) 

#define IDeckLinkAPIInformation_GetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetString(This,cfgID,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkAPIInformation_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkDisplayModeIterator_INTERFACE_DEFINED__
#define __IDeckLinkDisplayModeIterator_INTERFACE_DEFINED__

/* interface IDeckLinkDisplayModeIterator */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkDisplayModeIterator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("455D741F-1779-4800-86F5-0B5D13D79751")
    IDeckLinkDisplayModeIterator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [out] */ IDeckLinkDisplayMode **deckLinkDisplayMode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkDisplayModeIteratorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkDisplayModeIterator * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkDisplayModeIterator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkDisplayModeIterator * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDeckLinkDisplayModeIterator * This,
            /* [out] */ IDeckLinkDisplayMode **deckLinkDisplayMode);
        
        END_INTERFACE
    } IDeckLinkDisplayModeIteratorVtbl;

    interface IDeckLinkDisplayModeIterator
    {
        CONST_VTBL struct IDeckLinkDisplayModeIteratorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkDisplayModeIterator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkDisplayModeIterator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkDisplayModeIterator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkDisplayModeIterator_Next(This,deckLinkDisplayMode)	\
    ( (This)->lpVtbl -> Next(This,deckLinkDisplayMode) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkDisplayModeIterator_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkDisplayMode_INTERFACE_DEFINED__
#define __IDeckLinkDisplayMode_INTERFACE_DEFINED__

/* interface IDeckLinkDisplayMode */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkDisplayMode;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("87451E84-2B7E-439E-A629-4393EA4A8550")
    IDeckLinkDisplayMode : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ BSTR *name) = 0;
        
        virtual BMDDisplayMode STDMETHODCALLTYPE GetDisplayMode( void) = 0;
        
        virtual long STDMETHODCALLTYPE GetWidth( void) = 0;
        
        virtual long STDMETHODCALLTYPE GetHeight( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameRate( 
            /* [out] */ BMDTimeValue *frameDuration,
            /* [out] */ BMDTimeScale *timeScale) = 0;
        
        virtual BMDFieldDominance STDMETHODCALLTYPE GetFieldDominance( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkDisplayModeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkDisplayMode * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkDisplayMode * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkDisplayMode * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IDeckLinkDisplayMode * This,
            /* [out] */ BSTR *name);
        
        BMDDisplayMode ( STDMETHODCALLTYPE *GetDisplayMode )( 
            IDeckLinkDisplayMode * This);
        
        long ( STDMETHODCALLTYPE *GetWidth )( 
            IDeckLinkDisplayMode * This);
        
        long ( STDMETHODCALLTYPE *GetHeight )( 
            IDeckLinkDisplayMode * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameRate )( 
            IDeckLinkDisplayMode * This,
            /* [out] */ BMDTimeValue *frameDuration,
            /* [out] */ BMDTimeScale *timeScale);
        
        BMDFieldDominance ( STDMETHODCALLTYPE *GetFieldDominance )( 
            IDeckLinkDisplayMode * This);
        
        END_INTERFACE
    } IDeckLinkDisplayModeVtbl;

    interface IDeckLinkDisplayMode
    {
        CONST_VTBL struct IDeckLinkDisplayModeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkDisplayMode_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkDisplayMode_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkDisplayMode_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkDisplayMode_GetName(This,name)	\
    ( (This)->lpVtbl -> GetName(This,name) ) 

#define IDeckLinkDisplayMode_GetDisplayMode(This)	\
    ( (This)->lpVtbl -> GetDisplayMode(This) ) 

#define IDeckLinkDisplayMode_GetWidth(This)	\
    ( (This)->lpVtbl -> GetWidth(This) ) 

#define IDeckLinkDisplayMode_GetHeight(This)	\
    ( (This)->lpVtbl -> GetHeight(This) ) 

#define IDeckLinkDisplayMode_GetFrameRate(This,frameDuration,timeScale)	\
    ( (This)->lpVtbl -> GetFrameRate(This,frameDuration,timeScale) ) 

#define IDeckLinkDisplayMode_GetFieldDominance(This)	\
    ( (This)->lpVtbl -> GetFieldDominance(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkDisplayMode_INTERFACE_DEFINED__ */


#ifndef __IDeckLink_INTERFACE_DEFINED__
#define __IDeckLink_INTERFACE_DEFINED__

/* interface IDeckLink */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLink;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("62BFF75D-6569-4E55-8D4D-66AA03829ABC")
    IDeckLink : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetModelName( 
            /* [out] */ BSTR *modelName) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLink * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLink * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLink * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetModelName )( 
            IDeckLink * This,
            /* [out] */ BSTR *modelName);
        
        END_INTERFACE
    } IDeckLinkVtbl;

    interface IDeckLink
    {
        CONST_VTBL struct IDeckLinkVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLink_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLink_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLink_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLink_GetModelName(This,modelName)	\
    ( (This)->lpVtbl -> GetModelName(This,modelName) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLink_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkOutput_INTERFACE_DEFINED__
#define __IDeckLinkOutput_INTERFACE_DEFINED__

/* interface IDeckLinkOutput */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkOutput;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("29228142-EB8C-4141-A621-F74026450955")
    IDeckLinkOutput : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoMode( 
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            /* [out] */ BMDDisplayModeSupport *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScreenPreviewCallback( 
            /* [in] */ IDeckLinkScreenPreviewCallback *previewCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoOutput( 
            BMDDisplayMode displayMode,
            BMDVideoOutputFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVideoOutput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVideoOutputFrameMemoryAllocator( 
            /* [in] */ IDeckLinkMemoryAllocator *theAllocator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateVideoFrame( 
            long width,
            long height,
            long rowBytes,
            BMDPixelFormat pixelFormat,
            BMDFrameFlags flags,
            /* [out] */ IDeckLinkMutableVideoFrame **outFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateAncillaryData( 
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            /* [out] */ IDeckLinkVideoFrameAncillary **outBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisplayVideoFrameSync( 
            /* [in] */ IDeckLinkVideoFrame *theFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduleVideoFrame( 
            /* [in] */ IDeckLinkVideoFrame *theFrame,
            BMDTimeValue displayTime,
            BMDTimeValue displayDuration,
            BMDTimeScale timeScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScheduledFrameCompletionCallback( 
            /* [in] */ IDeckLinkVideoOutputCallback *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBufferedVideoFrameCount( 
            /* [out] */ unsigned long *bufferedFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAudioOutput( 
            BMDAudioSampleRate sampleRate,
            BMDAudioSampleType sampleType,
            unsigned long channelCount,
            BMDAudioOutputStreamType streamType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAudioOutput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteAudioSamplesSync( 
            /* [in] */ void *buffer,
            unsigned long sampleFrameCount,
            /* [out] */ unsigned long *sampleFramesWritten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginAudioPreroll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndAudioPreroll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduleAudioSamples( 
            /* [in] */ void *buffer,
            unsigned long sampleFrameCount,
            BMDTimeValue streamTime,
            BMDTimeScale timeScale,
            /* [out] */ unsigned long *sampleFramesWritten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBufferedAudioSampleFrameCount( 
            /* [out] */ unsigned long *bufferedSampleFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushBufferedAudioSamples( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAudioCallback( 
            /* [in] */ IDeckLinkAudioOutputCallback *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartScheduledPlayback( 
            BMDTimeValue playbackStartTime,
            BMDTimeScale timeScale,
            double playbackSpeed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopScheduledPlayback( 
            BMDTimeValue stopPlaybackAtTime,
            /* [out] */ BMDTimeValue *actualStopTime,
            BMDTimeScale timeScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsScheduledPlaybackRunning( 
            /* [out] */ BOOL *active) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetScheduledStreamTime( 
            BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *streamTime,
            /* [out] */ double *playbackSpeed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceClock( 
            BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkOutputVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkOutput * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkOutput * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkOutput * This);
        
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoMode )( 
            IDeckLinkOutput * This,
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            /* [out] */ BMDDisplayModeSupport *result);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeIterator )( 
            IDeckLinkOutput * This,
            /* [out] */ IDeckLinkDisplayModeIterator **iterator);
        
        HRESULT ( STDMETHODCALLTYPE *SetScreenPreviewCallback )( 
            IDeckLinkOutput * This,
            /* [in] */ IDeckLinkScreenPreviewCallback *previewCallback);
        
        HRESULT ( STDMETHODCALLTYPE *EnableVideoOutput )( 
            IDeckLinkOutput * This,
            BMDDisplayMode displayMode,
            BMDVideoOutputFlags flags);
        
        HRESULT ( STDMETHODCALLTYPE *DisableVideoOutput )( 
            IDeckLinkOutput * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetVideoOutputFrameMemoryAllocator )( 
            IDeckLinkOutput * This,
            /* [in] */ IDeckLinkMemoryAllocator *theAllocator);
        
        HRESULT ( STDMETHODCALLTYPE *CreateVideoFrame )( 
            IDeckLinkOutput * This,
            long width,
            long height,
            long rowBytes,
            BMDPixelFormat pixelFormat,
            BMDFrameFlags flags,
            /* [out] */ IDeckLinkMutableVideoFrame **outFrame);
        
        HRESULT ( STDMETHODCALLTYPE *CreateAncillaryData )( 
            IDeckLinkOutput * This,
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            /* [out] */ IDeckLinkVideoFrameAncillary **outBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *DisplayVideoFrameSync )( 
            IDeckLinkOutput * This,
            /* [in] */ IDeckLinkVideoFrame *theFrame);
        
        HRESULT ( STDMETHODCALLTYPE *ScheduleVideoFrame )( 
            IDeckLinkOutput * This,
            /* [in] */ IDeckLinkVideoFrame *theFrame,
            BMDTimeValue displayTime,
            BMDTimeValue displayDuration,
            BMDTimeScale timeScale);
        
        HRESULT ( STDMETHODCALLTYPE *SetScheduledFrameCompletionCallback )( 
            IDeckLinkOutput * This,
            /* [in] */ IDeckLinkVideoOutputCallback *theCallback);
        
        HRESULT ( STDMETHODCALLTYPE *GetBufferedVideoFrameCount )( 
            IDeckLinkOutput * This,
            /* [out] */ unsigned long *bufferedFrameCount);
        
        HRESULT ( STDMETHODCALLTYPE *EnableAudioOutput )( 
            IDeckLinkOutput * This,
            BMDAudioSampleRate sampleRate,
            BMDAudioSampleType sampleType,
            unsigned long channelCount,
            BMDAudioOutputStreamType streamType);
        
        HRESULT ( STDMETHODCALLTYPE *DisableAudioOutput )( 
            IDeckLinkOutput * This);
        
        HRESULT ( STDMETHODCALLTYPE *WriteAudioSamplesSync )( 
            IDeckLinkOutput * This,
            /* [in] */ void *buffer,
            unsigned long sampleFrameCount,
            /* [out] */ unsigned long *sampleFramesWritten);
        
        HRESULT ( STDMETHODCALLTYPE *BeginAudioPreroll )( 
            IDeckLinkOutput * This);
        
        HRESULT ( STDMETHODCALLTYPE *EndAudioPreroll )( 
            IDeckLinkOutput * This);
        
        HRESULT ( STDMETHODCALLTYPE *ScheduleAudioSamples )( 
            IDeckLinkOutput * This,
            /* [in] */ void *buffer,
            unsigned long sampleFrameCount,
            BMDTimeValue streamTime,
            BMDTimeScale timeScale,
            /* [out] */ unsigned long *sampleFramesWritten);
        
        HRESULT ( STDMETHODCALLTYPE *GetBufferedAudioSampleFrameCount )( 
            IDeckLinkOutput * This,
            /* [out] */ unsigned long *bufferedSampleFrameCount);
        
        HRESULT ( STDMETHODCALLTYPE *FlushBufferedAudioSamples )( 
            IDeckLinkOutput * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetAudioCallback )( 
            IDeckLinkOutput * This,
            /* [in] */ IDeckLinkAudioOutputCallback *theCallback);
        
        HRESULT ( STDMETHODCALLTYPE *StartScheduledPlayback )( 
            IDeckLinkOutput * This,
            BMDTimeValue playbackStartTime,
            BMDTimeScale timeScale,
            double playbackSpeed);
        
        HRESULT ( STDMETHODCALLTYPE *StopScheduledPlayback )( 
            IDeckLinkOutput * This,
            BMDTimeValue stopPlaybackAtTime,
            /* [out] */ BMDTimeValue *actualStopTime,
            BMDTimeScale timeScale);
        
        HRESULT ( STDMETHODCALLTYPE *IsScheduledPlaybackRunning )( 
            IDeckLinkOutput * This,
            /* [out] */ BOOL *active);
        
        HRESULT ( STDMETHODCALLTYPE *GetScheduledStreamTime )( 
            IDeckLinkOutput * This,
            BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *streamTime,
            /* [out] */ double *playbackSpeed);
        
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceClock )( 
            IDeckLinkOutput * This,
            BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame);
        
        END_INTERFACE
    } IDeckLinkOutputVtbl;

    interface IDeckLinkOutput
    {
        CONST_VTBL struct IDeckLinkOutputVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkOutput_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkOutput_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkOutput_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkOutput_DoesSupportVideoMode(This,displayMode,pixelFormat,result)	\
    ( (This)->lpVtbl -> DoesSupportVideoMode(This,displayMode,pixelFormat,result) ) 

#define IDeckLinkOutput_GetDisplayModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDisplayModeIterator(This,iterator) ) 

#define IDeckLinkOutput_SetScreenPreviewCallback(This,previewCallback)	\
    ( (This)->lpVtbl -> SetScreenPreviewCallback(This,previewCallback) ) 

#define IDeckLinkOutput_EnableVideoOutput(This,displayMode,flags)	\
    ( (This)->lpVtbl -> EnableVideoOutput(This,displayMode,flags) ) 

#define IDeckLinkOutput_DisableVideoOutput(This)	\
    ( (This)->lpVtbl -> DisableVideoOutput(This) ) 

#define IDeckLinkOutput_SetVideoOutputFrameMemoryAllocator(This,theAllocator)	\
    ( (This)->lpVtbl -> SetVideoOutputFrameMemoryAllocator(This,theAllocator) ) 

#define IDeckLinkOutput_CreateVideoFrame(This,width,height,rowBytes,pixelFormat,flags,outFrame)	\
    ( (This)->lpVtbl -> CreateVideoFrame(This,width,height,rowBytes,pixelFormat,flags,outFrame) ) 

#define IDeckLinkOutput_CreateAncillaryData(This,displayMode,pixelFormat,outBuffer)	\
    ( (This)->lpVtbl -> CreateAncillaryData(This,displayMode,pixelFormat,outBuffer) ) 

#define IDeckLinkOutput_DisplayVideoFrameSync(This,theFrame)	\
    ( (This)->lpVtbl -> DisplayVideoFrameSync(This,theFrame) ) 

#define IDeckLinkOutput_ScheduleVideoFrame(This,theFrame,displayTime,displayDuration,timeScale)	\
    ( (This)->lpVtbl -> ScheduleVideoFrame(This,theFrame,displayTime,displayDuration,timeScale) ) 

#define IDeckLinkOutput_SetScheduledFrameCompletionCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetScheduledFrameCompletionCallback(This,theCallback) ) 

#define IDeckLinkOutput_GetBufferedVideoFrameCount(This,bufferedFrameCount)	\
    ( (This)->lpVtbl -> GetBufferedVideoFrameCount(This,bufferedFrameCount) ) 

#define IDeckLinkOutput_EnableAudioOutput(This,sampleRate,sampleType,channelCount,streamType)	\
    ( (This)->lpVtbl -> EnableAudioOutput(This,sampleRate,sampleType,channelCount,streamType) ) 

#define IDeckLinkOutput_DisableAudioOutput(This)	\
    ( (This)->lpVtbl -> DisableAudioOutput(This) ) 

#define IDeckLinkOutput_WriteAudioSamplesSync(This,buffer,sampleFrameCount,sampleFramesWritten)	\
    ( (This)->lpVtbl -> WriteAudioSamplesSync(This,buffer,sampleFrameCount,sampleFramesWritten) ) 

#define IDeckLinkOutput_BeginAudioPreroll(This)	\
    ( (This)->lpVtbl -> BeginAudioPreroll(This) ) 

#define IDeckLinkOutput_EndAudioPreroll(This)	\
    ( (This)->lpVtbl -> EndAudioPreroll(This) ) 

#define IDeckLinkOutput_ScheduleAudioSamples(This,buffer,sampleFrameCount,streamTime,timeScale,sampleFramesWritten)	\
    ( (This)->lpVtbl -> ScheduleAudioSamples(This,buffer,sampleFrameCount,streamTime,timeScale,sampleFramesWritten) ) 

#define IDeckLinkOutput_GetBufferedAudioSampleFrameCount(This,bufferedSampleFrameCount)	\
    ( (This)->lpVtbl -> GetBufferedAudioSampleFrameCount(This,bufferedSampleFrameCount) ) 

#define IDeckLinkOutput_FlushBufferedAudioSamples(This)	\
    ( (This)->lpVtbl -> FlushBufferedAudioSamples(This) ) 

#define IDeckLinkOutput_SetAudioCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetAudioCallback(This,theCallback) ) 

#define IDeckLinkOutput_StartScheduledPlayback(This,playbackStartTime,timeScale,playbackSpeed)	\
    ( (This)->lpVtbl -> StartScheduledPlayback(This,playbackStartTime,timeScale,playbackSpeed) ) 

#define IDeckLinkOutput_StopScheduledPlayback(This,stopPlaybackAtTime,actualStopTime,timeScale)	\
    ( (This)->lpVtbl -> StopScheduledPlayback(This,stopPlaybackAtTime,actualStopTime,timeScale) ) 

#define IDeckLinkOutput_IsScheduledPlaybackRunning(This,active)	\
    ( (This)->lpVtbl -> IsScheduledPlaybackRunning(This,active) ) 

#define IDeckLinkOutput_GetScheduledStreamTime(This,desiredTimeScale,streamTime,playbackSpeed)	\
    ( (This)->lpVtbl -> GetScheduledStreamTime(This,desiredTimeScale,streamTime,playbackSpeed) ) 

#define IDeckLinkOutput_GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame)	\
    ( (This)->lpVtbl -> GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkOutput_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkInput_INTERFACE_DEFINED__
#define __IDeckLinkInput_INTERFACE_DEFINED__

/* interface IDeckLinkInput */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkInput;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("300C135A-9F43-48E2-9906-6D7911D93CF1")
    IDeckLinkInput : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoMode( 
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            /* [out] */ BMDDisplayModeSupport *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScreenPreviewCallback( 
            /* [in] */ IDeckLinkScreenPreviewCallback *previewCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoInput( 
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            BMDVideoInputFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVideoInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableVideoFrameCount( 
            /* [out] */ unsigned long *availableFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAudioInput( 
            BMDAudioSampleRate sampleRate,
            BMDAudioSampleType sampleType,
            unsigned long channelCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAudioInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableAudioSampleFrameCount( 
            /* [out] */ unsigned long *availableSampleFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PauseStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCallback( 
            /* [in] */ IDeckLinkInputCallback *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceClock( 
            BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkInputVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkInput * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkInput * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkInput * This);
        
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoMode )( 
            IDeckLinkInput * This,
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            /* [out] */ BMDDisplayModeSupport *result);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeIterator )( 
            IDeckLinkInput * This,
            /* [out] */ IDeckLinkDisplayModeIterator **iterator);
        
        HRESULT ( STDMETHODCALLTYPE *SetScreenPreviewCallback )( 
            IDeckLinkInput * This,
            /* [in] */ IDeckLinkScreenPreviewCallback *previewCallback);
        
        HRESULT ( STDMETHODCALLTYPE *EnableVideoInput )( 
            IDeckLinkInput * This,
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            BMDVideoInputFlags flags);
        
        HRESULT ( STDMETHODCALLTYPE *DisableVideoInput )( 
            IDeckLinkInput * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAvailableVideoFrameCount )( 
            IDeckLinkInput * This,
            /* [out] */ unsigned long *availableFrameCount);
        
        HRESULT ( STDMETHODCALLTYPE *EnableAudioInput )( 
            IDeckLinkInput * This,
            BMDAudioSampleRate sampleRate,
            BMDAudioSampleType sampleType,
            unsigned long channelCount);
        
        HRESULT ( STDMETHODCALLTYPE *DisableAudioInput )( 
            IDeckLinkInput * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAvailableAudioSampleFrameCount )( 
            IDeckLinkInput * This,
            /* [out] */ unsigned long *availableSampleFrameCount);
        
        HRESULT ( STDMETHODCALLTYPE *StartStreams )( 
            IDeckLinkInput * This);
        
        HRESULT ( STDMETHODCALLTYPE *StopStreams )( 
            IDeckLinkInput * This);
        
        HRESULT ( STDMETHODCALLTYPE *PauseStreams )( 
            IDeckLinkInput * This);
        
        HRESULT ( STDMETHODCALLTYPE *FlushStreams )( 
            IDeckLinkInput * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetCallback )( 
            IDeckLinkInput * This,
            /* [in] */ IDeckLinkInputCallback *theCallback);
        
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceClock )( 
            IDeckLinkInput * This,
            BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *hardwareTime,
            /* [out] */ BMDTimeValue *timeInFrame,
            /* [out] */ BMDTimeValue *ticksPerFrame);
        
        END_INTERFACE
    } IDeckLinkInputVtbl;

    interface IDeckLinkInput
    {
        CONST_VTBL struct IDeckLinkInputVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkInput_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkInput_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkInput_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkInput_DoesSupportVideoMode(This,displayMode,pixelFormat,result)	\
    ( (This)->lpVtbl -> DoesSupportVideoMode(This,displayMode,pixelFormat,result) ) 

#define IDeckLinkInput_GetDisplayModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDisplayModeIterator(This,iterator) ) 

#define IDeckLinkInput_SetScreenPreviewCallback(This,previewCallback)	\
    ( (This)->lpVtbl -> SetScreenPreviewCallback(This,previewCallback) ) 

#define IDeckLinkInput_EnableVideoInput(This,displayMode,pixelFormat,flags)	\
    ( (This)->lpVtbl -> EnableVideoInput(This,displayMode,pixelFormat,flags) ) 

#define IDeckLinkInput_DisableVideoInput(This)	\
    ( (This)->lpVtbl -> DisableVideoInput(This) ) 

#define IDeckLinkInput_GetAvailableVideoFrameCount(This,availableFrameCount)	\
    ( (This)->lpVtbl -> GetAvailableVideoFrameCount(This,availableFrameCount) ) 

#define IDeckLinkInput_EnableAudioInput(This,sampleRate,sampleType,channelCount)	\
    ( (This)->lpVtbl -> EnableAudioInput(This,sampleRate,sampleType,channelCount) ) 

#define IDeckLinkInput_DisableAudioInput(This)	\
    ( (This)->lpVtbl -> DisableAudioInput(This) ) 

#define IDeckLinkInput_GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount)	\
    ( (This)->lpVtbl -> GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount) ) 

#define IDeckLinkInput_StartStreams(This)	\
    ( (This)->lpVtbl -> StartStreams(This) ) 

#define IDeckLinkInput_StopStreams(This)	\
    ( (This)->lpVtbl -> StopStreams(This) ) 

#define IDeckLinkInput_PauseStreams(This)	\
    ( (This)->lpVtbl -> PauseStreams(This) ) 

#define IDeckLinkInput_FlushStreams(This)	\
    ( (This)->lpVtbl -> FlushStreams(This) ) 

#define IDeckLinkInput_SetCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetCallback(This,theCallback) ) 

#define IDeckLinkInput_GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame)	\
    ( (This)->lpVtbl -> GetHardwareReferenceClock(This,desiredTimeScale,hardwareTime,timeInFrame,ticksPerFrame) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkInput_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkTimecode_INTERFACE_DEFINED__
#define __IDeckLinkTimecode_INTERFACE_DEFINED__

/* interface IDeckLinkTimecode */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkTimecode;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EFB9BCA6-A521-44F7-BD69-2332F24D9EE6")
    IDeckLinkTimecode : public IUnknown
    {
    public:
        virtual BMDTimecodeBCD STDMETHODCALLTYPE GetBCD( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetComponents( 
            /* [out] */ unsigned char *hours,
            /* [out] */ unsigned char *minutes,
            /* [out] */ unsigned char *seconds,
            /* [out] */ unsigned char *frames) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [out] */ BSTR *timecode) = 0;
        
        virtual BMDTimecodeFlags STDMETHODCALLTYPE GetFlags( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkTimecodeVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkTimecode * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkTimecode * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkTimecode * This);
        
        BMDTimecodeBCD ( STDMETHODCALLTYPE *GetBCD )( 
            IDeckLinkTimecode * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetComponents )( 
            IDeckLinkTimecode * This,
            /* [out] */ unsigned char *hours,
            /* [out] */ unsigned char *minutes,
            /* [out] */ unsigned char *seconds,
            /* [out] */ unsigned char *frames);
        
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkTimecode * This,
            /* [out] */ BSTR *timecode);
        
        BMDTimecodeFlags ( STDMETHODCALLTYPE *GetFlags )( 
            IDeckLinkTimecode * This);
        
        END_INTERFACE
    } IDeckLinkTimecodeVtbl;

    interface IDeckLinkTimecode
    {
        CONST_VTBL struct IDeckLinkTimecodeVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkTimecode_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkTimecode_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkTimecode_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkTimecode_GetBCD(This)	\
    ( (This)->lpVtbl -> GetBCD(This) ) 

#define IDeckLinkTimecode_GetComponents(This,hours,minutes,seconds,frames)	\
    ( (This)->lpVtbl -> GetComponents(This,hours,minutes,seconds,frames) ) 

#define IDeckLinkTimecode_GetString(This,timecode)	\
    ( (This)->lpVtbl -> GetString(This,timecode) ) 

#define IDeckLinkTimecode_GetFlags(This)	\
    ( (This)->lpVtbl -> GetFlags(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkTimecode_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoFrame_INTERFACE_DEFINED__
#define __IDeckLinkVideoFrame_INTERFACE_DEFINED__

/* interface IDeckLinkVideoFrame */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoFrame;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A8D8238E-6B18-4196-99E1-5AF717B83D32")
    IDeckLinkVideoFrame : public IUnknown
    {
    public:
        virtual long STDMETHODCALLTYPE GetWidth( void) = 0;
        
        virtual long STDMETHODCALLTYPE GetHeight( void) = 0;
        
        virtual long STDMETHODCALLTYPE GetRowBytes( void) = 0;
        
        virtual BMDPixelFormat STDMETHODCALLTYPE GetPixelFormat( void) = 0;
        
        virtual BMDFrameFlags STDMETHODCALLTYPE GetFlags( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBytes( 
            /* [out] */ void **buffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTimecode( 
            BMDTimecodeFormat format,
            /* [out] */ IDeckLinkTimecode **timecode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAncillaryData( 
            /* [out] */ IDeckLinkVideoFrameAncillary **ancillary) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoFrameVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoFrame * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoFrame * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoFrame * This);
        
        long ( STDMETHODCALLTYPE *GetWidth )( 
            IDeckLinkVideoFrame * This);
        
        long ( STDMETHODCALLTYPE *GetHeight )( 
            IDeckLinkVideoFrame * This);
        
        long ( STDMETHODCALLTYPE *GetRowBytes )( 
            IDeckLinkVideoFrame * This);
        
        BMDPixelFormat ( STDMETHODCALLTYPE *GetPixelFormat )( 
            IDeckLinkVideoFrame * This);
        
        BMDFrameFlags ( STDMETHODCALLTYPE *GetFlags )( 
            IDeckLinkVideoFrame * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkVideoFrame * This,
            /* [out] */ void **buffer);
        
        HRESULT ( STDMETHODCALLTYPE *GetTimecode )( 
            IDeckLinkVideoFrame * This,
            BMDTimecodeFormat format,
            /* [out] */ IDeckLinkTimecode **timecode);
        
        HRESULT ( STDMETHODCALLTYPE *GetAncillaryData )( 
            IDeckLinkVideoFrame * This,
            /* [out] */ IDeckLinkVideoFrameAncillary **ancillary);
        
        END_INTERFACE
    } IDeckLinkVideoFrameVtbl;

    interface IDeckLinkVideoFrame
    {
        CONST_VTBL struct IDeckLinkVideoFrameVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoFrame_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoFrame_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoFrame_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoFrame_GetWidth(This)	\
    ( (This)->lpVtbl -> GetWidth(This) ) 

#define IDeckLinkVideoFrame_GetHeight(This)	\
    ( (This)->lpVtbl -> GetHeight(This) ) 

#define IDeckLinkVideoFrame_GetRowBytes(This)	\
    ( (This)->lpVtbl -> GetRowBytes(This) ) 

#define IDeckLinkVideoFrame_GetPixelFormat(This)	\
    ( (This)->lpVtbl -> GetPixelFormat(This) ) 

#define IDeckLinkVideoFrame_GetFlags(This)	\
    ( (This)->lpVtbl -> GetFlags(This) ) 

#define IDeckLinkVideoFrame_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#define IDeckLinkVideoFrame_GetTimecode(This,format,timecode)	\
    ( (This)->lpVtbl -> GetTimecode(This,format,timecode) ) 

#define IDeckLinkVideoFrame_GetAncillaryData(This,ancillary)	\
    ( (This)->lpVtbl -> GetAncillaryData(This,ancillary) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoFrame_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkMutableVideoFrame_INTERFACE_DEFINED__
#define __IDeckLinkMutableVideoFrame_INTERFACE_DEFINED__

/* interface IDeckLinkMutableVideoFrame */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkMutableVideoFrame;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("46FCEE00-B4E6-43D0-91C0-023A7FCEB34F")
    IDeckLinkMutableVideoFrame : public IDeckLinkVideoFrame
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetFlags( 
            BMDFrameFlags newFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTimecode( 
            BMDTimecodeFormat format,
            /* [in] */ IDeckLinkTimecode *timecode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetTimecodeFromComponents( 
            BMDTimecodeFormat format,
            unsigned char hours,
            unsigned char minutes,
            unsigned char seconds,
            unsigned char frames,
            BMDTimecodeFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAncillaryData( 
            /* [in] */ IDeckLinkVideoFrameAncillary *ancillary) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkMutableVideoFrameVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkMutableVideoFrame * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkMutableVideoFrame * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkMutableVideoFrame * This);
        
        long ( STDMETHODCALLTYPE *GetWidth )( 
            IDeckLinkMutableVideoFrame * This);
        
        long ( STDMETHODCALLTYPE *GetHeight )( 
            IDeckLinkMutableVideoFrame * This);
        
        long ( STDMETHODCALLTYPE *GetRowBytes )( 
            IDeckLinkMutableVideoFrame * This);
        
        BMDPixelFormat ( STDMETHODCALLTYPE *GetPixelFormat )( 
            IDeckLinkMutableVideoFrame * This);
        
        BMDFrameFlags ( STDMETHODCALLTYPE *GetFlags )( 
            IDeckLinkMutableVideoFrame * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkMutableVideoFrame * This,
            /* [out] */ void **buffer);
        
        HRESULT ( STDMETHODCALLTYPE *GetTimecode )( 
            IDeckLinkMutableVideoFrame * This,
            BMDTimecodeFormat format,
            /* [out] */ IDeckLinkTimecode **timecode);
        
        HRESULT ( STDMETHODCALLTYPE *GetAncillaryData )( 
            IDeckLinkMutableVideoFrame * This,
            /* [out] */ IDeckLinkVideoFrameAncillary **ancillary);
        
        HRESULT ( STDMETHODCALLTYPE *SetFlags )( 
            IDeckLinkMutableVideoFrame * This,
            BMDFrameFlags newFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetTimecode )( 
            IDeckLinkMutableVideoFrame * This,
            BMDTimecodeFormat format,
            /* [in] */ IDeckLinkTimecode *timecode);
        
        HRESULT ( STDMETHODCALLTYPE *SetTimecodeFromComponents )( 
            IDeckLinkMutableVideoFrame * This,
            BMDTimecodeFormat format,
            unsigned char hours,
            unsigned char minutes,
            unsigned char seconds,
            unsigned char frames,
            BMDTimecodeFlags flags);
        
        HRESULT ( STDMETHODCALLTYPE *SetAncillaryData )( 
            IDeckLinkMutableVideoFrame * This,
            /* [in] */ IDeckLinkVideoFrameAncillary *ancillary);
        
        END_INTERFACE
    } IDeckLinkMutableVideoFrameVtbl;

    interface IDeckLinkMutableVideoFrame
    {
        CONST_VTBL struct IDeckLinkMutableVideoFrameVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkMutableVideoFrame_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkMutableVideoFrame_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkMutableVideoFrame_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkMutableVideoFrame_GetWidth(This)	\
    ( (This)->lpVtbl -> GetWidth(This) ) 

#define IDeckLinkMutableVideoFrame_GetHeight(This)	\
    ( (This)->lpVtbl -> GetHeight(This) ) 

#define IDeckLinkMutableVideoFrame_GetRowBytes(This)	\
    ( (This)->lpVtbl -> GetRowBytes(This) ) 

#define IDeckLinkMutableVideoFrame_GetPixelFormat(This)	\
    ( (This)->lpVtbl -> GetPixelFormat(This) ) 

#define IDeckLinkMutableVideoFrame_GetFlags(This)	\
    ( (This)->lpVtbl -> GetFlags(This) ) 

#define IDeckLinkMutableVideoFrame_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#define IDeckLinkMutableVideoFrame_GetTimecode(This,format,timecode)	\
    ( (This)->lpVtbl -> GetTimecode(This,format,timecode) ) 

#define IDeckLinkMutableVideoFrame_GetAncillaryData(This,ancillary)	\
    ( (This)->lpVtbl -> GetAncillaryData(This,ancillary) ) 


#define IDeckLinkMutableVideoFrame_SetFlags(This,newFlags)	\
    ( (This)->lpVtbl -> SetFlags(This,newFlags) ) 

#define IDeckLinkMutableVideoFrame_SetTimecode(This,format,timecode)	\
    ( (This)->lpVtbl -> SetTimecode(This,format,timecode) ) 

#define IDeckLinkMutableVideoFrame_SetTimecodeFromComponents(This,format,hours,minutes,seconds,frames,flags)	\
    ( (This)->lpVtbl -> SetTimecodeFromComponents(This,format,hours,minutes,seconds,frames,flags) ) 

#define IDeckLinkMutableVideoFrame_SetAncillaryData(This,ancillary)	\
    ( (This)->lpVtbl -> SetAncillaryData(This,ancillary) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkMutableVideoFrame_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoInputFrame_INTERFACE_DEFINED__
#define __IDeckLinkVideoInputFrame_INTERFACE_DEFINED__

/* interface IDeckLinkVideoInputFrame */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoInputFrame;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9A74FA41-AE9F-47AC-8CF4-01F42DD59965")
    IDeckLinkVideoInputFrame : public IDeckLinkVideoFrame
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStreamTime( 
            /* [out] */ BMDTimeValue *frameTime,
            /* [out] */ BMDTimeValue *frameDuration,
            BMDTimeScale timeScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceTimestamp( 
            BMDTimeScale timeScale,
            /* [out] */ BMDTimeValue *frameTime,
            /* [out] */ BMDTimeValue *frameDuration) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoInputFrameVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoInputFrame * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoInputFrame * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoInputFrame * This);
        
        long ( STDMETHODCALLTYPE *GetWidth )( 
            IDeckLinkVideoInputFrame * This);
        
        long ( STDMETHODCALLTYPE *GetHeight )( 
            IDeckLinkVideoInputFrame * This);
        
        long ( STDMETHODCALLTYPE *GetRowBytes )( 
            IDeckLinkVideoInputFrame * This);
        
        BMDPixelFormat ( STDMETHODCALLTYPE *GetPixelFormat )( 
            IDeckLinkVideoInputFrame * This);
        
        BMDFrameFlags ( STDMETHODCALLTYPE *GetFlags )( 
            IDeckLinkVideoInputFrame * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkVideoInputFrame * This,
            /* [out] */ void **buffer);
        
        HRESULT ( STDMETHODCALLTYPE *GetTimecode )( 
            IDeckLinkVideoInputFrame * This,
            BMDTimecodeFormat format,
            /* [out] */ IDeckLinkTimecode **timecode);
        
        HRESULT ( STDMETHODCALLTYPE *GetAncillaryData )( 
            IDeckLinkVideoInputFrame * This,
            /* [out] */ IDeckLinkVideoFrameAncillary **ancillary);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamTime )( 
            IDeckLinkVideoInputFrame * This,
            /* [out] */ BMDTimeValue *frameTime,
            /* [out] */ BMDTimeValue *frameDuration,
            BMDTimeScale timeScale);
        
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceTimestamp )( 
            IDeckLinkVideoInputFrame * This,
            BMDTimeScale timeScale,
            /* [out] */ BMDTimeValue *frameTime,
            /* [out] */ BMDTimeValue *frameDuration);
        
        END_INTERFACE
    } IDeckLinkVideoInputFrameVtbl;

    interface IDeckLinkVideoInputFrame
    {
        CONST_VTBL struct IDeckLinkVideoInputFrameVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoInputFrame_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoInputFrame_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoInputFrame_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoInputFrame_GetWidth(This)	\
    ( (This)->lpVtbl -> GetWidth(This) ) 

#define IDeckLinkVideoInputFrame_GetHeight(This)	\
    ( (This)->lpVtbl -> GetHeight(This) ) 

#define IDeckLinkVideoInputFrame_GetRowBytes(This)	\
    ( (This)->lpVtbl -> GetRowBytes(This) ) 

#define IDeckLinkVideoInputFrame_GetPixelFormat(This)	\
    ( (This)->lpVtbl -> GetPixelFormat(This) ) 

#define IDeckLinkVideoInputFrame_GetFlags(This)	\
    ( (This)->lpVtbl -> GetFlags(This) ) 

#define IDeckLinkVideoInputFrame_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#define IDeckLinkVideoInputFrame_GetTimecode(This,format,timecode)	\
    ( (This)->lpVtbl -> GetTimecode(This,format,timecode) ) 

#define IDeckLinkVideoInputFrame_GetAncillaryData(This,ancillary)	\
    ( (This)->lpVtbl -> GetAncillaryData(This,ancillary) ) 


#define IDeckLinkVideoInputFrame_GetStreamTime(This,frameTime,frameDuration,timeScale)	\
    ( (This)->lpVtbl -> GetStreamTime(This,frameTime,frameDuration,timeScale) ) 

#define IDeckLinkVideoInputFrame_GetHardwareReferenceTimestamp(This,timeScale,frameTime,frameDuration)	\
    ( (This)->lpVtbl -> GetHardwareReferenceTimestamp(This,timeScale,frameTime,frameDuration) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoInputFrame_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoFrameAncillary_INTERFACE_DEFINED__
#define __IDeckLinkVideoFrameAncillary_INTERFACE_DEFINED__

/* interface IDeckLinkVideoFrameAncillary */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoFrameAncillary;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("732E723C-D1A4-4E29-9E8E-4A88797A0004")
    IDeckLinkVideoFrameAncillary : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetBufferForVerticalBlankingLine( 
            unsigned long lineNumber,
            /* [out] */ void **buffer) = 0;
        
        virtual BMDPixelFormat STDMETHODCALLTYPE GetPixelFormat( void) = 0;
        
        virtual BMDDisplayMode STDMETHODCALLTYPE GetDisplayMode( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoFrameAncillaryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoFrameAncillary * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoFrameAncillary * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoFrameAncillary * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBufferForVerticalBlankingLine )( 
            IDeckLinkVideoFrameAncillary * This,
            unsigned long lineNumber,
            /* [out] */ void **buffer);
        
        BMDPixelFormat ( STDMETHODCALLTYPE *GetPixelFormat )( 
            IDeckLinkVideoFrameAncillary * This);
        
        BMDDisplayMode ( STDMETHODCALLTYPE *GetDisplayMode )( 
            IDeckLinkVideoFrameAncillary * This);
        
        END_INTERFACE
    } IDeckLinkVideoFrameAncillaryVtbl;

    interface IDeckLinkVideoFrameAncillary
    {
        CONST_VTBL struct IDeckLinkVideoFrameAncillaryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoFrameAncillary_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoFrameAncillary_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoFrameAncillary_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoFrameAncillary_GetBufferForVerticalBlankingLine(This,lineNumber,buffer)	\
    ( (This)->lpVtbl -> GetBufferForVerticalBlankingLine(This,lineNumber,buffer) ) 

#define IDeckLinkVideoFrameAncillary_GetPixelFormat(This)	\
    ( (This)->lpVtbl -> GetPixelFormat(This) ) 

#define IDeckLinkVideoFrameAncillary_GetDisplayMode(This)	\
    ( (This)->lpVtbl -> GetDisplayMode(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoFrameAncillary_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkAudioInputPacket_INTERFACE_DEFINED__
#define __IDeckLinkAudioInputPacket_INTERFACE_DEFINED__

/* interface IDeckLinkAudioInputPacket */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkAudioInputPacket;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E43D5870-2894-11DE-8C30-0800200C9A66")
    IDeckLinkAudioInputPacket : public IUnknown
    {
    public:
        virtual long STDMETHODCALLTYPE GetSampleFrameCount( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBytes( 
            /* [out] */ void **buffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPacketTime( 
            /* [out] */ BMDTimeValue *packetTime,
            BMDTimeScale timeScale) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkAudioInputPacketVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkAudioInputPacket * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkAudioInputPacket * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkAudioInputPacket * This);
        
        long ( STDMETHODCALLTYPE *GetSampleFrameCount )( 
            IDeckLinkAudioInputPacket * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkAudioInputPacket * This,
            /* [out] */ void **buffer);
        
        HRESULT ( STDMETHODCALLTYPE *GetPacketTime )( 
            IDeckLinkAudioInputPacket * This,
            /* [out] */ BMDTimeValue *packetTime,
            BMDTimeScale timeScale);
        
        END_INTERFACE
    } IDeckLinkAudioInputPacketVtbl;

    interface IDeckLinkAudioInputPacket
    {
        CONST_VTBL struct IDeckLinkAudioInputPacketVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkAudioInputPacket_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkAudioInputPacket_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkAudioInputPacket_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkAudioInputPacket_GetSampleFrameCount(This)	\
    ( (This)->lpVtbl -> GetSampleFrameCount(This) ) 

#define IDeckLinkAudioInputPacket_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#define IDeckLinkAudioInputPacket_GetPacketTime(This,packetTime,timeScale)	\
    ( (This)->lpVtbl -> GetPacketTime(This,packetTime,timeScale) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkAudioInputPacket_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkScreenPreviewCallback_INTERFACE_DEFINED__
#define __IDeckLinkScreenPreviewCallback_INTERFACE_DEFINED__

/* interface IDeckLinkScreenPreviewCallback */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkScreenPreviewCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("373F499D-4B4D-4518-AD22-6354E5A5825E")
    IDeckLinkScreenPreviewCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DrawFrame( 
            /* [in] */ IDeckLinkVideoFrame *theFrame) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkScreenPreviewCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkScreenPreviewCallback * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkScreenPreviewCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkScreenPreviewCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *DrawFrame )( 
            IDeckLinkScreenPreviewCallback * This,
            /* [in] */ IDeckLinkVideoFrame *theFrame);
        
        END_INTERFACE
    } IDeckLinkScreenPreviewCallbackVtbl;

    interface IDeckLinkScreenPreviewCallback
    {
        CONST_VTBL struct IDeckLinkScreenPreviewCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkScreenPreviewCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkScreenPreviewCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkScreenPreviewCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkScreenPreviewCallback_DrawFrame(This,theFrame)	\
    ( (This)->lpVtbl -> DrawFrame(This,theFrame) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkScreenPreviewCallback_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkGLScreenPreviewHelper_INTERFACE_DEFINED__
#define __IDeckLinkGLScreenPreviewHelper_INTERFACE_DEFINED__

/* interface IDeckLinkGLScreenPreviewHelper */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkGLScreenPreviewHelper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("BA575CD9-A15E-497B-B2C2-F9AFE7BE4EBA")
    IDeckLinkGLScreenPreviewHelper : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE InitializeGL( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PaintGL( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFrame( 
            /* [in] */ IDeckLinkVideoFrame *theFrame) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkGLScreenPreviewHelperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkGLScreenPreviewHelper * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkGLScreenPreviewHelper * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkGLScreenPreviewHelper * This);
        
        HRESULT ( STDMETHODCALLTYPE *InitializeGL )( 
            IDeckLinkGLScreenPreviewHelper * This);
        
        HRESULT ( STDMETHODCALLTYPE *PaintGL )( 
            IDeckLinkGLScreenPreviewHelper * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetFrame )( 
            IDeckLinkGLScreenPreviewHelper * This,
            /* [in] */ IDeckLinkVideoFrame *theFrame);
        
        END_INTERFACE
    } IDeckLinkGLScreenPreviewHelperVtbl;

    interface IDeckLinkGLScreenPreviewHelper
    {
        CONST_VTBL struct IDeckLinkGLScreenPreviewHelperVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkGLScreenPreviewHelper_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkGLScreenPreviewHelper_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkGLScreenPreviewHelper_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkGLScreenPreviewHelper_InitializeGL(This)	\
    ( (This)->lpVtbl -> InitializeGL(This) ) 

#define IDeckLinkGLScreenPreviewHelper_PaintGL(This)	\
    ( (This)->lpVtbl -> PaintGL(This) ) 

#define IDeckLinkGLScreenPreviewHelper_SetFrame(This,theFrame)	\
    ( (This)->lpVtbl -> SetFrame(This,theFrame) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkGLScreenPreviewHelper_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkConfiguration_INTERFACE_DEFINED__
#define __IDeckLinkConfiguration_INTERFACE_DEFINED__

/* interface IDeckLinkConfiguration */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkConfiguration;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B8EAD569-B764-47F0-A73F-AE40DF6CBF10")
    IDeckLinkConfiguration : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetConfigurationValidator( 
            /* [out] */ IDeckLinkConfiguration **configObject) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteConfigurationToPreferences( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVideoOutputFormat( 
            BMDVideoConnection videoOutputConnection) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsVideoOutputActive( 
            BMDVideoConnection videoOutputConnection,
            /* [out] */ BOOL *active) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAnalogVideoOutputFlags( 
            BMDAnalogVideoFlags analogVideoFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAnalogVideoOutputFlags( 
            /* [out] */ BMDAnalogVideoFlags *analogVideoFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableFieldFlickerRemovalWhenPaused( 
            BOOL enable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsEnabledFieldFlickerRemovalWhenPaused( 
            /* [out] */ BOOL *enabled) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Set444And3GBpsVideoOutput( 
            BOOL enable444VideoOutput,
            BOOL enable3GbsOutput) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Get444And3GBpsVideoOutput( 
            /* [out] */ BOOL *is444VideoOutputEnabled,
            /* [out] */ BOOL *threeGbsOutputEnabled) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVideoOutputConversionMode( 
            BMDVideoOutputConversionMode conversionMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVideoOutputConversionMode( 
            /* [out] */ BMDVideoOutputConversionMode *conversionMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Set_HD1080p24_to_HD1080i5994_Conversion( 
            BOOL enable) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Get_HD1080p24_to_HD1080i5994_Conversion( 
            /* [out] */ BOOL *enabled) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVideoInputFormat( 
            BMDVideoConnection videoInputFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVideoInputFormat( 
            /* [out] */ BMDVideoConnection *videoInputFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAnalogVideoInputFlags( 
            BMDAnalogVideoFlags analogVideoFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAnalogVideoInputFlags( 
            /* [out] */ BMDAnalogVideoFlags *analogVideoFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVideoInputConversionMode( 
            BMDVideoInputConversionMode conversionMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVideoInputConversionMode( 
            /* [out] */ BMDVideoInputConversionMode *conversionMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetBlackVideoOutputDuringCapture( 
            BOOL blackOutInCapture) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBlackVideoOutputDuringCapture( 
            /* [out] */ BOOL *blackOutInCapture) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Set32PulldownSequenceInitialTimecodeFrame( 
            unsigned long aFrameTimecode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Get32PulldownSequenceInitialTimecodeFrame( 
            /* [out] */ unsigned long *aFrameTimecode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVancSourceLineMapping( 
            unsigned long activeLine1VANCsource,
            unsigned long activeLine2VANCsource,
            unsigned long activeLine3VANCsource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetVancSourceLineMapping( 
            /* [out] */ unsigned long *activeLine1VANCsource,
            /* [out] */ unsigned long *activeLine2VANCsource,
            /* [out] */ unsigned long *activeLine3VANCsource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAudioInputFormat( 
            BMDAudioConnection audioInputFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAudioInputFormat( 
            /* [out] */ BMDAudioConnection *audioInputFormat) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkConfigurationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkConfiguration * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkConfiguration * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkConfiguration * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetConfigurationValidator )( 
            IDeckLinkConfiguration * This,
            /* [out] */ IDeckLinkConfiguration **configObject);
        
        HRESULT ( STDMETHODCALLTYPE *WriteConfigurationToPreferences )( 
            IDeckLinkConfiguration * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetVideoOutputFormat )( 
            IDeckLinkConfiguration * This,
            BMDVideoConnection videoOutputConnection);
        
        HRESULT ( STDMETHODCALLTYPE *IsVideoOutputActive )( 
            IDeckLinkConfiguration * This,
            BMDVideoConnection videoOutputConnection,
            /* [out] */ BOOL *active);
        
        HRESULT ( STDMETHODCALLTYPE *SetAnalogVideoOutputFlags )( 
            IDeckLinkConfiguration * This,
            BMDAnalogVideoFlags analogVideoFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetAnalogVideoOutputFlags )( 
            IDeckLinkConfiguration * This,
            /* [out] */ BMDAnalogVideoFlags *analogVideoFlags);
        
        HRESULT ( STDMETHODCALLTYPE *EnableFieldFlickerRemovalWhenPaused )( 
            IDeckLinkConfiguration * This,
            BOOL enable);
        
        HRESULT ( STDMETHODCALLTYPE *IsEnabledFieldFlickerRemovalWhenPaused )( 
            IDeckLinkConfiguration * This,
            /* [out] */ BOOL *enabled);
        
        HRESULT ( STDMETHODCALLTYPE *Set444And3GBpsVideoOutput )( 
            IDeckLinkConfiguration * This,
            BOOL enable444VideoOutput,
            BOOL enable3GbsOutput);
        
        HRESULT ( STDMETHODCALLTYPE *Get444And3GBpsVideoOutput )( 
            IDeckLinkConfiguration * This,
            /* [out] */ BOOL *is444VideoOutputEnabled,
            /* [out] */ BOOL *threeGbsOutputEnabled);
        
        HRESULT ( STDMETHODCALLTYPE *SetVideoOutputConversionMode )( 
            IDeckLinkConfiguration * This,
            BMDVideoOutputConversionMode conversionMode);
        
        HRESULT ( STDMETHODCALLTYPE *GetVideoOutputConversionMode )( 
            IDeckLinkConfiguration * This,
            /* [out] */ BMDVideoOutputConversionMode *conversionMode);
        
        HRESULT ( STDMETHODCALLTYPE *Set_HD1080p24_to_HD1080i5994_Conversion )( 
            IDeckLinkConfiguration * This,
            BOOL enable);
        
        HRESULT ( STDMETHODCALLTYPE *Get_HD1080p24_to_HD1080i5994_Conversion )( 
            IDeckLinkConfiguration * This,
            /* [out] */ BOOL *enabled);
        
        HRESULT ( STDMETHODCALLTYPE *SetVideoInputFormat )( 
            IDeckLinkConfiguration * This,
            BMDVideoConnection videoInputFormat);
        
        HRESULT ( STDMETHODCALLTYPE *GetVideoInputFormat )( 
            IDeckLinkConfiguration * This,
            /* [out] */ BMDVideoConnection *videoInputFormat);
        
        HRESULT ( STDMETHODCALLTYPE *SetAnalogVideoInputFlags )( 
            IDeckLinkConfiguration * This,
            BMDAnalogVideoFlags analogVideoFlags);
        
        HRESULT ( STDMETHODCALLTYPE *GetAnalogVideoInputFlags )( 
            IDeckLinkConfiguration * This,
            /* [out] */ BMDAnalogVideoFlags *analogVideoFlags);
        
        HRESULT ( STDMETHODCALLTYPE *SetVideoInputConversionMode )( 
            IDeckLinkConfiguration * This,
            BMDVideoInputConversionMode conversionMode);
        
        HRESULT ( STDMETHODCALLTYPE *GetVideoInputConversionMode )( 
            IDeckLinkConfiguration * This,
            /* [out] */ BMDVideoInputConversionMode *conversionMode);
        
        HRESULT ( STDMETHODCALLTYPE *SetBlackVideoOutputDuringCapture )( 
            IDeckLinkConfiguration * This,
            BOOL blackOutInCapture);
        
        HRESULT ( STDMETHODCALLTYPE *GetBlackVideoOutputDuringCapture )( 
            IDeckLinkConfiguration * This,
            /* [out] */ BOOL *blackOutInCapture);
        
        HRESULT ( STDMETHODCALLTYPE *Set32PulldownSequenceInitialTimecodeFrame )( 
            IDeckLinkConfiguration * This,
            unsigned long aFrameTimecode);
        
        HRESULT ( STDMETHODCALLTYPE *Get32PulldownSequenceInitialTimecodeFrame )( 
            IDeckLinkConfiguration * This,
            /* [out] */ unsigned long *aFrameTimecode);
        
        HRESULT ( STDMETHODCALLTYPE *SetVancSourceLineMapping )( 
            IDeckLinkConfiguration * This,
            unsigned long activeLine1VANCsource,
            unsigned long activeLine2VANCsource,
            unsigned long activeLine3VANCsource);
        
        HRESULT ( STDMETHODCALLTYPE *GetVancSourceLineMapping )( 
            IDeckLinkConfiguration * This,
            /* [out] */ unsigned long *activeLine1VANCsource,
            /* [out] */ unsigned long *activeLine2VANCsource,
            /* [out] */ unsigned long *activeLine3VANCsource);
        
        HRESULT ( STDMETHODCALLTYPE *SetAudioInputFormat )( 
            IDeckLinkConfiguration * This,
            BMDAudioConnection audioInputFormat);
        
        HRESULT ( STDMETHODCALLTYPE *GetAudioInputFormat )( 
            IDeckLinkConfiguration * This,
            /* [out] */ BMDAudioConnection *audioInputFormat);
        
        END_INTERFACE
    } IDeckLinkConfigurationVtbl;

    interface IDeckLinkConfiguration
    {
        CONST_VTBL struct IDeckLinkConfigurationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkConfiguration_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkConfiguration_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkConfiguration_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkConfiguration_GetConfigurationValidator(This,configObject)	\
    ( (This)->lpVtbl -> GetConfigurationValidator(This,configObject) ) 

#define IDeckLinkConfiguration_WriteConfigurationToPreferences(This)	\
    ( (This)->lpVtbl -> WriteConfigurationToPreferences(This) ) 

#define IDeckLinkConfiguration_SetVideoOutputFormat(This,videoOutputConnection)	\
    ( (This)->lpVtbl -> SetVideoOutputFormat(This,videoOutputConnection) ) 

#define IDeckLinkConfiguration_IsVideoOutputActive(This,videoOutputConnection,active)	\
    ( (This)->lpVtbl -> IsVideoOutputActive(This,videoOutputConnection,active) ) 

#define IDeckLinkConfiguration_SetAnalogVideoOutputFlags(This,analogVideoFlags)	\
    ( (This)->lpVtbl -> SetAnalogVideoOutputFlags(This,analogVideoFlags) ) 

#define IDeckLinkConfiguration_GetAnalogVideoOutputFlags(This,analogVideoFlags)	\
    ( (This)->lpVtbl -> GetAnalogVideoOutputFlags(This,analogVideoFlags) ) 

#define IDeckLinkConfiguration_EnableFieldFlickerRemovalWhenPaused(This,enable)	\
    ( (This)->lpVtbl -> EnableFieldFlickerRemovalWhenPaused(This,enable) ) 

#define IDeckLinkConfiguration_IsEnabledFieldFlickerRemovalWhenPaused(This,enabled)	\
    ( (This)->lpVtbl -> IsEnabledFieldFlickerRemovalWhenPaused(This,enabled) ) 

#define IDeckLinkConfiguration_Set444And3GBpsVideoOutput(This,enable444VideoOutput,enable3GbsOutput)	\
    ( (This)->lpVtbl -> Set444And3GBpsVideoOutput(This,enable444VideoOutput,enable3GbsOutput) ) 

#define IDeckLinkConfiguration_Get444And3GBpsVideoOutput(This,is444VideoOutputEnabled,threeGbsOutputEnabled)	\
    ( (This)->lpVtbl -> Get444And3GBpsVideoOutput(This,is444VideoOutputEnabled,threeGbsOutputEnabled) ) 

#define IDeckLinkConfiguration_SetVideoOutputConversionMode(This,conversionMode)	\
    ( (This)->lpVtbl -> SetVideoOutputConversionMode(This,conversionMode) ) 

#define IDeckLinkConfiguration_GetVideoOutputConversionMode(This,conversionMode)	\
    ( (This)->lpVtbl -> GetVideoOutputConversionMode(This,conversionMode) ) 

#define IDeckLinkConfiguration_Set_HD1080p24_to_HD1080i5994_Conversion(This,enable)	\
    ( (This)->lpVtbl -> Set_HD1080p24_to_HD1080i5994_Conversion(This,enable) ) 

#define IDeckLinkConfiguration_Get_HD1080p24_to_HD1080i5994_Conversion(This,enabled)	\
    ( (This)->lpVtbl -> Get_HD1080p24_to_HD1080i5994_Conversion(This,enabled) ) 

#define IDeckLinkConfiguration_SetVideoInputFormat(This,videoInputFormat)	\
    ( (This)->lpVtbl -> SetVideoInputFormat(This,videoInputFormat) ) 

#define IDeckLinkConfiguration_GetVideoInputFormat(This,videoInputFormat)	\
    ( (This)->lpVtbl -> GetVideoInputFormat(This,videoInputFormat) ) 

#define IDeckLinkConfiguration_SetAnalogVideoInputFlags(This,analogVideoFlags)	\
    ( (This)->lpVtbl -> SetAnalogVideoInputFlags(This,analogVideoFlags) ) 

#define IDeckLinkConfiguration_GetAnalogVideoInputFlags(This,analogVideoFlags)	\
    ( (This)->lpVtbl -> GetAnalogVideoInputFlags(This,analogVideoFlags) ) 

#define IDeckLinkConfiguration_SetVideoInputConversionMode(This,conversionMode)	\
    ( (This)->lpVtbl -> SetVideoInputConversionMode(This,conversionMode) ) 

#define IDeckLinkConfiguration_GetVideoInputConversionMode(This,conversionMode)	\
    ( (This)->lpVtbl -> GetVideoInputConversionMode(This,conversionMode) ) 

#define IDeckLinkConfiguration_SetBlackVideoOutputDuringCapture(This,blackOutInCapture)	\
    ( (This)->lpVtbl -> SetBlackVideoOutputDuringCapture(This,blackOutInCapture) ) 

#define IDeckLinkConfiguration_GetBlackVideoOutputDuringCapture(This,blackOutInCapture)	\
    ( (This)->lpVtbl -> GetBlackVideoOutputDuringCapture(This,blackOutInCapture) ) 

#define IDeckLinkConfiguration_Set32PulldownSequenceInitialTimecodeFrame(This,aFrameTimecode)	\
    ( (This)->lpVtbl -> Set32PulldownSequenceInitialTimecodeFrame(This,aFrameTimecode) ) 

#define IDeckLinkConfiguration_Get32PulldownSequenceInitialTimecodeFrame(This,aFrameTimecode)	\
    ( (This)->lpVtbl -> Get32PulldownSequenceInitialTimecodeFrame(This,aFrameTimecode) ) 

#define IDeckLinkConfiguration_SetVancSourceLineMapping(This,activeLine1VANCsource,activeLine2VANCsource,activeLine3VANCsource)	\
    ( (This)->lpVtbl -> SetVancSourceLineMapping(This,activeLine1VANCsource,activeLine2VANCsource,activeLine3VANCsource) ) 

#define IDeckLinkConfiguration_GetVancSourceLineMapping(This,activeLine1VANCsource,activeLine2VANCsource,activeLine3VANCsource)	\
    ( (This)->lpVtbl -> GetVancSourceLineMapping(This,activeLine1VANCsource,activeLine2VANCsource,activeLine3VANCsource) ) 

#define IDeckLinkConfiguration_SetAudioInputFormat(This,audioInputFormat)	\
    ( (This)->lpVtbl -> SetAudioInputFormat(This,audioInputFormat) ) 

#define IDeckLinkConfiguration_GetAudioInputFormat(This,audioInputFormat)	\
    ( (This)->lpVtbl -> GetAudioInputFormat(This,audioInputFormat) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkConfiguration_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkAttributes_INTERFACE_DEFINED__
#define __IDeckLinkAttributes_INTERFACE_DEFINED__

/* interface IDeckLinkAttributes */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkAttributes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ABC11843-D966-44CB-96E2-A1CB5D3135C4")
    IDeckLinkAttributes : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFlag( 
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ BOOL *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInt( 
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ LONGLONG *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFloat( 
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ double *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetString( 
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ BSTR *value) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkAttributesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkAttributes * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkAttributes * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkAttributes * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetFlag )( 
            IDeckLinkAttributes * This,
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ BOOL *value);
        
        HRESULT ( STDMETHODCALLTYPE *GetInt )( 
            IDeckLinkAttributes * This,
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ LONGLONG *value);
        
        HRESULT ( STDMETHODCALLTYPE *GetFloat )( 
            IDeckLinkAttributes * This,
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ double *value);
        
        HRESULT ( STDMETHODCALLTYPE *GetString )( 
            IDeckLinkAttributes * This,
            /* [in] */ BMDDeckLinkAttributeID cfgID,
            /* [out] */ BSTR *value);
        
        END_INTERFACE
    } IDeckLinkAttributesVtbl;

    interface IDeckLinkAttributes
    {
        CONST_VTBL struct IDeckLinkAttributesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkAttributes_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkAttributes_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkAttributes_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkAttributes_GetFlag(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFlag(This,cfgID,value) ) 

#define IDeckLinkAttributes_GetInt(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetInt(This,cfgID,value) ) 

#define IDeckLinkAttributes_GetFloat(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetFloat(This,cfgID,value) ) 

#define IDeckLinkAttributes_GetString(This,cfgID,value)	\
    ( (This)->lpVtbl -> GetString(This,cfgID,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkAttributes_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkKeyer_INTERFACE_DEFINED__
#define __IDeckLinkKeyer_INTERFACE_DEFINED__

/* interface IDeckLinkKeyer */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkKeyer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("89AFCAF5-65F8-421E-98F7-96FE5F5BFBA3")
    IDeckLinkKeyer : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Enable( 
            /* [in] */ BOOL isExternal) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetLevel( 
            /* [in] */ unsigned char level) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RampUp( 
            /* [in] */ unsigned long numberOfFrames) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE RampDown( 
            /* [in] */ unsigned long numberOfFrames) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Disable( void) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkKeyerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkKeyer * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkKeyer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkKeyer * This);
        
        HRESULT ( STDMETHODCALLTYPE *Enable )( 
            IDeckLinkKeyer * This,
            /* [in] */ BOOL isExternal);
        
        HRESULT ( STDMETHODCALLTYPE *SetLevel )( 
            IDeckLinkKeyer * This,
            /* [in] */ unsigned char level);
        
        HRESULT ( STDMETHODCALLTYPE *RampUp )( 
            IDeckLinkKeyer * This,
            /* [in] */ unsigned long numberOfFrames);
        
        HRESULT ( STDMETHODCALLTYPE *RampDown )( 
            IDeckLinkKeyer * This,
            /* [in] */ unsigned long numberOfFrames);
        
        HRESULT ( STDMETHODCALLTYPE *Disable )( 
            IDeckLinkKeyer * This);
        
        END_INTERFACE
    } IDeckLinkKeyerVtbl;

    interface IDeckLinkKeyer
    {
        CONST_VTBL struct IDeckLinkKeyerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkKeyer_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkKeyer_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkKeyer_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkKeyer_Enable(This,isExternal)	\
    ( (This)->lpVtbl -> Enable(This,isExternal) ) 

#define IDeckLinkKeyer_SetLevel(This,level)	\
    ( (This)->lpVtbl -> SetLevel(This,level) ) 

#define IDeckLinkKeyer_RampUp(This,numberOfFrames)	\
    ( (This)->lpVtbl -> RampUp(This,numberOfFrames) ) 

#define IDeckLinkKeyer_RampDown(This,numberOfFrames)	\
    ( (This)->lpVtbl -> RampDown(This,numberOfFrames) ) 

#define IDeckLinkKeyer_Disable(This)	\
    ( (This)->lpVtbl -> Disable(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkKeyer_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_CDeckLinkIterator;

#ifdef __cplusplus

class DECLSPEC_UUID("D9EDA3B3-2887-41FA-B724-017CF1EB1D37")
CDeckLinkIterator;
#endif

EXTERN_C const CLSID CLSID_CDeckLinkGLScreenPreviewHelper;

#ifdef __cplusplus

class DECLSPEC_UUID("D398CEE7-4434-4CA3-9BA6-5AE34556B905")
CDeckLinkGLScreenPreviewHelper;
#endif

#ifndef __IDeckLinkDisplayModeIterator_v7_1_INTERFACE_DEFINED__
#define __IDeckLinkDisplayModeIterator_v7_1_INTERFACE_DEFINED__

/* interface IDeckLinkDisplayModeIterator_v7_1 */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkDisplayModeIterator_v7_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("B28131B6-59AC-4857-B5AC-CD75D5883E2F")
    IDeckLinkDisplayModeIterator_v7_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( 
            /* [out] */ IDeckLinkDisplayMode_v7_1 **deckLinkDisplayMode) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkDisplayModeIterator_v7_1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkDisplayModeIterator_v7_1 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkDisplayModeIterator_v7_1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkDisplayModeIterator_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IDeckLinkDisplayModeIterator_v7_1 * This,
            /* [out] */ IDeckLinkDisplayMode_v7_1 **deckLinkDisplayMode);
        
        END_INTERFACE
    } IDeckLinkDisplayModeIterator_v7_1Vtbl;

    interface IDeckLinkDisplayModeIterator_v7_1
    {
        CONST_VTBL struct IDeckLinkDisplayModeIterator_v7_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkDisplayModeIterator_v7_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkDisplayModeIterator_v7_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkDisplayModeIterator_v7_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkDisplayModeIterator_v7_1_Next(This,deckLinkDisplayMode)	\
    ( (This)->lpVtbl -> Next(This,deckLinkDisplayMode) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkDisplayModeIterator_v7_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkDisplayMode_v7_1_INTERFACE_DEFINED__
#define __IDeckLinkDisplayMode_v7_1_INTERFACE_DEFINED__

/* interface IDeckLinkDisplayMode_v7_1 */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkDisplayMode_v7_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AF0CD6D5-8376-435E-8433-54F9DD530AC3")
    IDeckLinkDisplayMode_v7_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ BSTR *name) = 0;
        
        virtual BMDDisplayMode STDMETHODCALLTYPE GetDisplayMode( void) = 0;
        
        virtual long STDMETHODCALLTYPE GetWidth( void) = 0;
        
        virtual long STDMETHODCALLTYPE GetHeight( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameRate( 
            /* [out] */ BMDTimeValue *frameDuration,
            /* [out] */ BMDTimeScale *timeScale) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkDisplayMode_v7_1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkDisplayMode_v7_1 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkDisplayMode_v7_1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkDisplayMode_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IDeckLinkDisplayMode_v7_1 * This,
            /* [out] */ BSTR *name);
        
        BMDDisplayMode ( STDMETHODCALLTYPE *GetDisplayMode )( 
            IDeckLinkDisplayMode_v7_1 * This);
        
        long ( STDMETHODCALLTYPE *GetWidth )( 
            IDeckLinkDisplayMode_v7_1 * This);
        
        long ( STDMETHODCALLTYPE *GetHeight )( 
            IDeckLinkDisplayMode_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameRate )( 
            IDeckLinkDisplayMode_v7_1 * This,
            /* [out] */ BMDTimeValue *frameDuration,
            /* [out] */ BMDTimeScale *timeScale);
        
        END_INTERFACE
    } IDeckLinkDisplayMode_v7_1Vtbl;

    interface IDeckLinkDisplayMode_v7_1
    {
        CONST_VTBL struct IDeckLinkDisplayMode_v7_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkDisplayMode_v7_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkDisplayMode_v7_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkDisplayMode_v7_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkDisplayMode_v7_1_GetName(This,name)	\
    ( (This)->lpVtbl -> GetName(This,name) ) 

#define IDeckLinkDisplayMode_v7_1_GetDisplayMode(This)	\
    ( (This)->lpVtbl -> GetDisplayMode(This) ) 

#define IDeckLinkDisplayMode_v7_1_GetWidth(This)	\
    ( (This)->lpVtbl -> GetWidth(This) ) 

#define IDeckLinkDisplayMode_v7_1_GetHeight(This)	\
    ( (This)->lpVtbl -> GetHeight(This) ) 

#define IDeckLinkDisplayMode_v7_1_GetFrameRate(This,frameDuration,timeScale)	\
    ( (This)->lpVtbl -> GetFrameRate(This,frameDuration,timeScale) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkDisplayMode_v7_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoFrame_v7_1_INTERFACE_DEFINED__
#define __IDeckLinkVideoFrame_v7_1_INTERFACE_DEFINED__

/* interface IDeckLinkVideoFrame_v7_1 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoFrame_v7_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("333F3A10-8C2D-43CF-B79D-46560FEEA1CE")
    IDeckLinkVideoFrame_v7_1 : public IUnknown
    {
    public:
        virtual long STDMETHODCALLTYPE GetWidth( void) = 0;
        
        virtual long STDMETHODCALLTYPE GetHeight( void) = 0;
        
        virtual long STDMETHODCALLTYPE GetRowBytes( void) = 0;
        
        virtual BMDPixelFormat STDMETHODCALLTYPE GetPixelFormat( void) = 0;
        
        virtual BMDFrameFlags STDMETHODCALLTYPE GetFlags( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBytes( 
            void **buffer) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoFrame_v7_1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoFrame_v7_1 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoFrame_v7_1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoFrame_v7_1 * This);
        
        long ( STDMETHODCALLTYPE *GetWidth )( 
            IDeckLinkVideoFrame_v7_1 * This);
        
        long ( STDMETHODCALLTYPE *GetHeight )( 
            IDeckLinkVideoFrame_v7_1 * This);
        
        long ( STDMETHODCALLTYPE *GetRowBytes )( 
            IDeckLinkVideoFrame_v7_1 * This);
        
        BMDPixelFormat ( STDMETHODCALLTYPE *GetPixelFormat )( 
            IDeckLinkVideoFrame_v7_1 * This);
        
        BMDFrameFlags ( STDMETHODCALLTYPE *GetFlags )( 
            IDeckLinkVideoFrame_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkVideoFrame_v7_1 * This,
            void **buffer);
        
        END_INTERFACE
    } IDeckLinkVideoFrame_v7_1Vtbl;

    interface IDeckLinkVideoFrame_v7_1
    {
        CONST_VTBL struct IDeckLinkVideoFrame_v7_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoFrame_v7_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoFrame_v7_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoFrame_v7_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoFrame_v7_1_GetWidth(This)	\
    ( (This)->lpVtbl -> GetWidth(This) ) 

#define IDeckLinkVideoFrame_v7_1_GetHeight(This)	\
    ( (This)->lpVtbl -> GetHeight(This) ) 

#define IDeckLinkVideoFrame_v7_1_GetRowBytes(This)	\
    ( (This)->lpVtbl -> GetRowBytes(This) ) 

#define IDeckLinkVideoFrame_v7_1_GetPixelFormat(This)	\
    ( (This)->lpVtbl -> GetPixelFormat(This) ) 

#define IDeckLinkVideoFrame_v7_1_GetFlags(This)	\
    ( (This)->lpVtbl -> GetFlags(This) ) 

#define IDeckLinkVideoFrame_v7_1_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoFrame_v7_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoInputFrame_v7_1_INTERFACE_DEFINED__
#define __IDeckLinkVideoInputFrame_v7_1_INTERFACE_DEFINED__

/* interface IDeckLinkVideoInputFrame_v7_1 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoInputFrame_v7_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C8B41D95-8848-40EE-9B37-6E3417FB114B")
    IDeckLinkVideoInputFrame_v7_1 : public IDeckLinkVideoFrame_v7_1
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFrameTime( 
            BMDTimeValue *frameTime,
            BMDTimeValue *frameDuration,
            BMDTimeScale timeScale) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoInputFrame_v7_1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoInputFrame_v7_1 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoInputFrame_v7_1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoInputFrame_v7_1 * This);
        
        long ( STDMETHODCALLTYPE *GetWidth )( 
            IDeckLinkVideoInputFrame_v7_1 * This);
        
        long ( STDMETHODCALLTYPE *GetHeight )( 
            IDeckLinkVideoInputFrame_v7_1 * This);
        
        long ( STDMETHODCALLTYPE *GetRowBytes )( 
            IDeckLinkVideoInputFrame_v7_1 * This);
        
        BMDPixelFormat ( STDMETHODCALLTYPE *GetPixelFormat )( 
            IDeckLinkVideoInputFrame_v7_1 * This);
        
        BMDFrameFlags ( STDMETHODCALLTYPE *GetFlags )( 
            IDeckLinkVideoInputFrame_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkVideoInputFrame_v7_1 * This,
            void **buffer);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameTime )( 
            IDeckLinkVideoInputFrame_v7_1 * This,
            BMDTimeValue *frameTime,
            BMDTimeValue *frameDuration,
            BMDTimeScale timeScale);
        
        END_INTERFACE
    } IDeckLinkVideoInputFrame_v7_1Vtbl;

    interface IDeckLinkVideoInputFrame_v7_1
    {
        CONST_VTBL struct IDeckLinkVideoInputFrame_v7_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoInputFrame_v7_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoInputFrame_v7_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoInputFrame_v7_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoInputFrame_v7_1_GetWidth(This)	\
    ( (This)->lpVtbl -> GetWidth(This) ) 

#define IDeckLinkVideoInputFrame_v7_1_GetHeight(This)	\
    ( (This)->lpVtbl -> GetHeight(This) ) 

#define IDeckLinkVideoInputFrame_v7_1_GetRowBytes(This)	\
    ( (This)->lpVtbl -> GetRowBytes(This) ) 

#define IDeckLinkVideoInputFrame_v7_1_GetPixelFormat(This)	\
    ( (This)->lpVtbl -> GetPixelFormat(This) ) 

#define IDeckLinkVideoInputFrame_v7_1_GetFlags(This)	\
    ( (This)->lpVtbl -> GetFlags(This) ) 

#define IDeckLinkVideoInputFrame_v7_1_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 


#define IDeckLinkVideoInputFrame_v7_1_GetFrameTime(This,frameTime,frameDuration,timeScale)	\
    ( (This)->lpVtbl -> GetFrameTime(This,frameTime,frameDuration,timeScale) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoInputFrame_v7_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkAudioInputPacket_v7_1_INTERFACE_DEFINED__
#define __IDeckLinkAudioInputPacket_v7_1_INTERFACE_DEFINED__

/* interface IDeckLinkAudioInputPacket_v7_1 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkAudioInputPacket_v7_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("C86DE4F6-A29F-42E3-AB3A-1363E29F0788")
    IDeckLinkAudioInputPacket_v7_1 : public IUnknown
    {
    public:
        virtual long STDMETHODCALLTYPE GetSampleCount( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBytes( 
            void **buffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAudioPacketTime( 
            BMDTimeValue *packetTime,
            BMDTimeScale timeScale) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkAudioInputPacket_v7_1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkAudioInputPacket_v7_1 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkAudioInputPacket_v7_1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkAudioInputPacket_v7_1 * This);
        
        long ( STDMETHODCALLTYPE *GetSampleCount )( 
            IDeckLinkAudioInputPacket_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkAudioInputPacket_v7_1 * This,
            void **buffer);
        
        HRESULT ( STDMETHODCALLTYPE *GetAudioPacketTime )( 
            IDeckLinkAudioInputPacket_v7_1 * This,
            BMDTimeValue *packetTime,
            BMDTimeScale timeScale);
        
        END_INTERFACE
    } IDeckLinkAudioInputPacket_v7_1Vtbl;

    interface IDeckLinkAudioInputPacket_v7_1
    {
        CONST_VTBL struct IDeckLinkAudioInputPacket_v7_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkAudioInputPacket_v7_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkAudioInputPacket_v7_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkAudioInputPacket_v7_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkAudioInputPacket_v7_1_GetSampleCount(This)	\
    ( (This)->lpVtbl -> GetSampleCount(This) ) 

#define IDeckLinkAudioInputPacket_v7_1_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#define IDeckLinkAudioInputPacket_v7_1_GetAudioPacketTime(This,packetTime,timeScale)	\
    ( (This)->lpVtbl -> GetAudioPacketTime(This,packetTime,timeScale) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkAudioInputPacket_v7_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoOutputCallback_v7_1_INTERFACE_DEFINED__
#define __IDeckLinkVideoOutputCallback_v7_1_INTERFACE_DEFINED__

/* interface IDeckLinkVideoOutputCallback_v7_1 */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoOutputCallback_v7_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EBD01AFA-E4B0-49C6-A01D-EDB9D1B55FD9")
    IDeckLinkVideoOutputCallback_v7_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE ScheduledFrameCompleted( 
            /* [in] */ IDeckLinkVideoFrame_v7_1 *completedFrame,
            /* [in] */ BMDOutputFrameCompletionResult result) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoOutputCallback_v7_1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoOutputCallback_v7_1 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoOutputCallback_v7_1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoOutputCallback_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *ScheduledFrameCompleted )( 
            IDeckLinkVideoOutputCallback_v7_1 * This,
            /* [in] */ IDeckLinkVideoFrame_v7_1 *completedFrame,
            /* [in] */ BMDOutputFrameCompletionResult result);
        
        END_INTERFACE
    } IDeckLinkVideoOutputCallback_v7_1Vtbl;

    interface IDeckLinkVideoOutputCallback_v7_1
    {
        CONST_VTBL struct IDeckLinkVideoOutputCallback_v7_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoOutputCallback_v7_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoOutputCallback_v7_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoOutputCallback_v7_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoOutputCallback_v7_1_ScheduledFrameCompleted(This,completedFrame,result)	\
    ( (This)->lpVtbl -> ScheduledFrameCompleted(This,completedFrame,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoOutputCallback_v7_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkInputCallback_v7_1_INTERFACE_DEFINED__
#define __IDeckLinkInputCallback_v7_1_INTERFACE_DEFINED__

/* interface IDeckLinkInputCallback_v7_1 */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkInputCallback_v7_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7F94F328-5ED4-4E9F-9729-76A86BDC99CC")
    IDeckLinkInputCallback_v7_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived( 
            /* [in] */ IDeckLinkVideoInputFrame_v7_1 *videoFrame,
            /* [in] */ IDeckLinkAudioInputPacket_v7_1 *audioPacket) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkInputCallback_v7_1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkInputCallback_v7_1 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkInputCallback_v7_1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkInputCallback_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *VideoInputFrameArrived )( 
            IDeckLinkInputCallback_v7_1 * This,
            /* [in] */ IDeckLinkVideoInputFrame_v7_1 *videoFrame,
            /* [in] */ IDeckLinkAudioInputPacket_v7_1 *audioPacket);
        
        END_INTERFACE
    } IDeckLinkInputCallback_v7_1Vtbl;

    interface IDeckLinkInputCallback_v7_1
    {
        CONST_VTBL struct IDeckLinkInputCallback_v7_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkInputCallback_v7_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkInputCallback_v7_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkInputCallback_v7_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkInputCallback_v7_1_VideoInputFrameArrived(This,videoFrame,audioPacket)	\
    ( (This)->lpVtbl -> VideoInputFrameArrived(This,videoFrame,audioPacket) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkInputCallback_v7_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkOutput_v7_1_INTERFACE_DEFINED__
#define __IDeckLinkOutput_v7_1_INTERFACE_DEFINED__

/* interface IDeckLinkOutput_v7_1 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkOutput_v7_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AE5B3E9B-4E1E-4535-B6E8-480FF52F6CE5")
    IDeckLinkOutput_v7_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoMode( 
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            /* [out] */ BMDDisplayModeSupport *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator_v7_1 **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoOutput( 
            BMDDisplayMode displayMode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVideoOutput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVideoOutputFrameMemoryAllocator( 
            /* [in] */ IDeckLinkMemoryAllocator *theAllocator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateVideoFrame( 
            long width,
            long height,
            long rowBytes,
            BMDPixelFormat pixelFormat,
            BMDFrameFlags flags,
            IDeckLinkVideoFrame_v7_1 **outFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateVideoFrameFromBuffer( 
            void *buffer,
            long width,
            long height,
            long rowBytes,
            BMDPixelFormat pixelFormat,
            BMDFrameFlags flags,
            IDeckLinkVideoFrame_v7_1 **outFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisplayVideoFrameSync( 
            IDeckLinkVideoFrame_v7_1 *theFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduleVideoFrame( 
            IDeckLinkVideoFrame_v7_1 *theFrame,
            BMDTimeValue displayTime,
            BMDTimeValue displayDuration,
            BMDTimeScale timeScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScheduledFrameCompletionCallback( 
            /* [in] */ IDeckLinkVideoOutputCallback_v7_1 *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAudioOutput( 
            BMDAudioSampleRate sampleRate,
            BMDAudioSampleType sampleType,
            unsigned long channelCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAudioOutput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteAudioSamplesSync( 
            void *buffer,
            unsigned long sampleFrameCount,
            /* [out] */ unsigned long *sampleFramesWritten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginAudioPreroll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndAudioPreroll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduleAudioSamples( 
            void *buffer,
            unsigned long sampleFrameCount,
            BMDTimeValue streamTime,
            BMDTimeScale timeScale,
            /* [out] */ unsigned long *sampleFramesWritten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBufferedAudioSampleFrameCount( 
            /* [out] */ unsigned long *bufferedSampleCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushBufferedAudioSamples( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAudioCallback( 
            /* [in] */ IDeckLinkAudioOutputCallback *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartScheduledPlayback( 
            BMDTimeValue playbackStartTime,
            BMDTimeScale timeScale,
            double playbackSpeed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopScheduledPlayback( 
            BMDTimeValue stopPlaybackAtTime,
            BMDTimeValue *actualStopTime,
            BMDTimeScale timeScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceClock( 
            BMDTimeScale desiredTimeScale,
            BMDTimeValue *elapsedTimeSinceSchedulerBegan) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkOutput_v7_1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkOutput_v7_1 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkOutput_v7_1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkOutput_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoMode )( 
            IDeckLinkOutput_v7_1 * This,
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            /* [out] */ BMDDisplayModeSupport *result);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeIterator )( 
            IDeckLinkOutput_v7_1 * This,
            /* [out] */ IDeckLinkDisplayModeIterator_v7_1 **iterator);
        
        HRESULT ( STDMETHODCALLTYPE *EnableVideoOutput )( 
            IDeckLinkOutput_v7_1 * This,
            BMDDisplayMode displayMode);
        
        HRESULT ( STDMETHODCALLTYPE *DisableVideoOutput )( 
            IDeckLinkOutput_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetVideoOutputFrameMemoryAllocator )( 
            IDeckLinkOutput_v7_1 * This,
            /* [in] */ IDeckLinkMemoryAllocator *theAllocator);
        
        HRESULT ( STDMETHODCALLTYPE *CreateVideoFrame )( 
            IDeckLinkOutput_v7_1 * This,
            long width,
            long height,
            long rowBytes,
            BMDPixelFormat pixelFormat,
            BMDFrameFlags flags,
            IDeckLinkVideoFrame_v7_1 **outFrame);
        
        HRESULT ( STDMETHODCALLTYPE *CreateVideoFrameFromBuffer )( 
            IDeckLinkOutput_v7_1 * This,
            void *buffer,
            long width,
            long height,
            long rowBytes,
            BMDPixelFormat pixelFormat,
            BMDFrameFlags flags,
            IDeckLinkVideoFrame_v7_1 **outFrame);
        
        HRESULT ( STDMETHODCALLTYPE *DisplayVideoFrameSync )( 
            IDeckLinkOutput_v7_1 * This,
            IDeckLinkVideoFrame_v7_1 *theFrame);
        
        HRESULT ( STDMETHODCALLTYPE *ScheduleVideoFrame )( 
            IDeckLinkOutput_v7_1 * This,
            IDeckLinkVideoFrame_v7_1 *theFrame,
            BMDTimeValue displayTime,
            BMDTimeValue displayDuration,
            BMDTimeScale timeScale);
        
        HRESULT ( STDMETHODCALLTYPE *SetScheduledFrameCompletionCallback )( 
            IDeckLinkOutput_v7_1 * This,
            /* [in] */ IDeckLinkVideoOutputCallback_v7_1 *theCallback);
        
        HRESULT ( STDMETHODCALLTYPE *EnableAudioOutput )( 
            IDeckLinkOutput_v7_1 * This,
            BMDAudioSampleRate sampleRate,
            BMDAudioSampleType sampleType,
            unsigned long channelCount);
        
        HRESULT ( STDMETHODCALLTYPE *DisableAudioOutput )( 
            IDeckLinkOutput_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *WriteAudioSamplesSync )( 
            IDeckLinkOutput_v7_1 * This,
            void *buffer,
            unsigned long sampleFrameCount,
            /* [out] */ unsigned long *sampleFramesWritten);
        
        HRESULT ( STDMETHODCALLTYPE *BeginAudioPreroll )( 
            IDeckLinkOutput_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *EndAudioPreroll )( 
            IDeckLinkOutput_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *ScheduleAudioSamples )( 
            IDeckLinkOutput_v7_1 * This,
            void *buffer,
            unsigned long sampleFrameCount,
            BMDTimeValue streamTime,
            BMDTimeScale timeScale,
            /* [out] */ unsigned long *sampleFramesWritten);
        
        HRESULT ( STDMETHODCALLTYPE *GetBufferedAudioSampleFrameCount )( 
            IDeckLinkOutput_v7_1 * This,
            /* [out] */ unsigned long *bufferedSampleCount);
        
        HRESULT ( STDMETHODCALLTYPE *FlushBufferedAudioSamples )( 
            IDeckLinkOutput_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetAudioCallback )( 
            IDeckLinkOutput_v7_1 * This,
            /* [in] */ IDeckLinkAudioOutputCallback *theCallback);
        
        HRESULT ( STDMETHODCALLTYPE *StartScheduledPlayback )( 
            IDeckLinkOutput_v7_1 * This,
            BMDTimeValue playbackStartTime,
            BMDTimeScale timeScale,
            double playbackSpeed);
        
        HRESULT ( STDMETHODCALLTYPE *StopScheduledPlayback )( 
            IDeckLinkOutput_v7_1 * This,
            BMDTimeValue stopPlaybackAtTime,
            BMDTimeValue *actualStopTime,
            BMDTimeScale timeScale);
        
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceClock )( 
            IDeckLinkOutput_v7_1 * This,
            BMDTimeScale desiredTimeScale,
            BMDTimeValue *elapsedTimeSinceSchedulerBegan);
        
        END_INTERFACE
    } IDeckLinkOutput_v7_1Vtbl;

    interface IDeckLinkOutput_v7_1
    {
        CONST_VTBL struct IDeckLinkOutput_v7_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkOutput_v7_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkOutput_v7_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkOutput_v7_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkOutput_v7_1_DoesSupportVideoMode(This,displayMode,pixelFormat,result)	\
    ( (This)->lpVtbl -> DoesSupportVideoMode(This,displayMode,pixelFormat,result) ) 

#define IDeckLinkOutput_v7_1_GetDisplayModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDisplayModeIterator(This,iterator) ) 

#define IDeckLinkOutput_v7_1_EnableVideoOutput(This,displayMode)	\
    ( (This)->lpVtbl -> EnableVideoOutput(This,displayMode) ) 

#define IDeckLinkOutput_v7_1_DisableVideoOutput(This)	\
    ( (This)->lpVtbl -> DisableVideoOutput(This) ) 

#define IDeckLinkOutput_v7_1_SetVideoOutputFrameMemoryAllocator(This,theAllocator)	\
    ( (This)->lpVtbl -> SetVideoOutputFrameMemoryAllocator(This,theAllocator) ) 

#define IDeckLinkOutput_v7_1_CreateVideoFrame(This,width,height,rowBytes,pixelFormat,flags,outFrame)	\
    ( (This)->lpVtbl -> CreateVideoFrame(This,width,height,rowBytes,pixelFormat,flags,outFrame) ) 

#define IDeckLinkOutput_v7_1_CreateVideoFrameFromBuffer(This,buffer,width,height,rowBytes,pixelFormat,flags,outFrame)	\
    ( (This)->lpVtbl -> CreateVideoFrameFromBuffer(This,buffer,width,height,rowBytes,pixelFormat,flags,outFrame) ) 

#define IDeckLinkOutput_v7_1_DisplayVideoFrameSync(This,theFrame)	\
    ( (This)->lpVtbl -> DisplayVideoFrameSync(This,theFrame) ) 

#define IDeckLinkOutput_v7_1_ScheduleVideoFrame(This,theFrame,displayTime,displayDuration,timeScale)	\
    ( (This)->lpVtbl -> ScheduleVideoFrame(This,theFrame,displayTime,displayDuration,timeScale) ) 

#define IDeckLinkOutput_v7_1_SetScheduledFrameCompletionCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetScheduledFrameCompletionCallback(This,theCallback) ) 

#define IDeckLinkOutput_v7_1_EnableAudioOutput(This,sampleRate,sampleType,channelCount)	\
    ( (This)->lpVtbl -> EnableAudioOutput(This,sampleRate,sampleType,channelCount) ) 

#define IDeckLinkOutput_v7_1_DisableAudioOutput(This)	\
    ( (This)->lpVtbl -> DisableAudioOutput(This) ) 

#define IDeckLinkOutput_v7_1_WriteAudioSamplesSync(This,buffer,sampleFrameCount,sampleFramesWritten)	\
    ( (This)->lpVtbl -> WriteAudioSamplesSync(This,buffer,sampleFrameCount,sampleFramesWritten) ) 

#define IDeckLinkOutput_v7_1_BeginAudioPreroll(This)	\
    ( (This)->lpVtbl -> BeginAudioPreroll(This) ) 

#define IDeckLinkOutput_v7_1_EndAudioPreroll(This)	\
    ( (This)->lpVtbl -> EndAudioPreroll(This) ) 

#define IDeckLinkOutput_v7_1_ScheduleAudioSamples(This,buffer,sampleFrameCount,streamTime,timeScale,sampleFramesWritten)	\
    ( (This)->lpVtbl -> ScheduleAudioSamples(This,buffer,sampleFrameCount,streamTime,timeScale,sampleFramesWritten) ) 

#define IDeckLinkOutput_v7_1_GetBufferedAudioSampleFrameCount(This,bufferedSampleCount)	\
    ( (This)->lpVtbl -> GetBufferedAudioSampleFrameCount(This,bufferedSampleCount) ) 

#define IDeckLinkOutput_v7_1_FlushBufferedAudioSamples(This)	\
    ( (This)->lpVtbl -> FlushBufferedAudioSamples(This) ) 

#define IDeckLinkOutput_v7_1_SetAudioCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetAudioCallback(This,theCallback) ) 

#define IDeckLinkOutput_v7_1_StartScheduledPlayback(This,playbackStartTime,timeScale,playbackSpeed)	\
    ( (This)->lpVtbl -> StartScheduledPlayback(This,playbackStartTime,timeScale,playbackSpeed) ) 

#define IDeckLinkOutput_v7_1_StopScheduledPlayback(This,stopPlaybackAtTime,actualStopTime,timeScale)	\
    ( (This)->lpVtbl -> StopScheduledPlayback(This,stopPlaybackAtTime,actualStopTime,timeScale) ) 

#define IDeckLinkOutput_v7_1_GetHardwareReferenceClock(This,desiredTimeScale,elapsedTimeSinceSchedulerBegan)	\
    ( (This)->lpVtbl -> GetHardwareReferenceClock(This,desiredTimeScale,elapsedTimeSinceSchedulerBegan) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkOutput_v7_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkInput_v7_1_INTERFACE_DEFINED__
#define __IDeckLinkInput_v7_1_INTERFACE_DEFINED__

/* interface IDeckLinkInput_v7_1 */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkInput_v7_1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2B54EDEF-5B32-429F-BA11-BB990596EACD")
    IDeckLinkInput_v7_1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoMode( 
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            /* [out] */ BMDDisplayModeSupport *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator_v7_1 **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoInput( 
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            BMDVideoInputFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVideoInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAudioInput( 
            BMDAudioSampleRate sampleRate,
            BMDAudioSampleType sampleType,
            unsigned long channelCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAudioInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReadAudioSamples( 
            void *buffer,
            unsigned long sampleFrameCount,
            /* [out] */ unsigned long *sampleFramesRead,
            /* [out] */ BMDTimeValue *audioPacketTime,
            BMDTimeScale timeScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBufferedAudioSampleFrameCount( 
            /* [out] */ unsigned long *bufferedSampleCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PauseStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCallback( 
            /* [in] */ IDeckLinkInputCallback_v7_1 *theCallback) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkInput_v7_1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkInput_v7_1 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkInput_v7_1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkInput_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoMode )( 
            IDeckLinkInput_v7_1 * This,
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            /* [out] */ BMDDisplayModeSupport *result);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeIterator )( 
            IDeckLinkInput_v7_1 * This,
            /* [out] */ IDeckLinkDisplayModeIterator_v7_1 **iterator);
        
        HRESULT ( STDMETHODCALLTYPE *EnableVideoInput )( 
            IDeckLinkInput_v7_1 * This,
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            BMDVideoInputFlags flags);
        
        HRESULT ( STDMETHODCALLTYPE *DisableVideoInput )( 
            IDeckLinkInput_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *EnableAudioInput )( 
            IDeckLinkInput_v7_1 * This,
            BMDAudioSampleRate sampleRate,
            BMDAudioSampleType sampleType,
            unsigned long channelCount);
        
        HRESULT ( STDMETHODCALLTYPE *DisableAudioInput )( 
            IDeckLinkInput_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *ReadAudioSamples )( 
            IDeckLinkInput_v7_1 * This,
            void *buffer,
            unsigned long sampleFrameCount,
            /* [out] */ unsigned long *sampleFramesRead,
            /* [out] */ BMDTimeValue *audioPacketTime,
            BMDTimeScale timeScale);
        
        HRESULT ( STDMETHODCALLTYPE *GetBufferedAudioSampleFrameCount )( 
            IDeckLinkInput_v7_1 * This,
            /* [out] */ unsigned long *bufferedSampleCount);
        
        HRESULT ( STDMETHODCALLTYPE *StartStreams )( 
            IDeckLinkInput_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *StopStreams )( 
            IDeckLinkInput_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *PauseStreams )( 
            IDeckLinkInput_v7_1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetCallback )( 
            IDeckLinkInput_v7_1 * This,
            /* [in] */ IDeckLinkInputCallback_v7_1 *theCallback);
        
        END_INTERFACE
    } IDeckLinkInput_v7_1Vtbl;

    interface IDeckLinkInput_v7_1
    {
        CONST_VTBL struct IDeckLinkInput_v7_1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkInput_v7_1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkInput_v7_1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkInput_v7_1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkInput_v7_1_DoesSupportVideoMode(This,displayMode,pixelFormat,result)	\
    ( (This)->lpVtbl -> DoesSupportVideoMode(This,displayMode,pixelFormat,result) ) 

#define IDeckLinkInput_v7_1_GetDisplayModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDisplayModeIterator(This,iterator) ) 

#define IDeckLinkInput_v7_1_EnableVideoInput(This,displayMode,pixelFormat,flags)	\
    ( (This)->lpVtbl -> EnableVideoInput(This,displayMode,pixelFormat,flags) ) 

#define IDeckLinkInput_v7_1_DisableVideoInput(This)	\
    ( (This)->lpVtbl -> DisableVideoInput(This) ) 

#define IDeckLinkInput_v7_1_EnableAudioInput(This,sampleRate,sampleType,channelCount)	\
    ( (This)->lpVtbl -> EnableAudioInput(This,sampleRate,sampleType,channelCount) ) 

#define IDeckLinkInput_v7_1_DisableAudioInput(This)	\
    ( (This)->lpVtbl -> DisableAudioInput(This) ) 

#define IDeckLinkInput_v7_1_ReadAudioSamples(This,buffer,sampleFrameCount,sampleFramesRead,audioPacketTime,timeScale)	\
    ( (This)->lpVtbl -> ReadAudioSamples(This,buffer,sampleFrameCount,sampleFramesRead,audioPacketTime,timeScale) ) 

#define IDeckLinkInput_v7_1_GetBufferedAudioSampleFrameCount(This,bufferedSampleCount)	\
    ( (This)->lpVtbl -> GetBufferedAudioSampleFrameCount(This,bufferedSampleCount) ) 

#define IDeckLinkInput_v7_1_StartStreams(This)	\
    ( (This)->lpVtbl -> StartStreams(This) ) 

#define IDeckLinkInput_v7_1_StopStreams(This)	\
    ( (This)->lpVtbl -> StopStreams(This) ) 

#define IDeckLinkInput_v7_1_PauseStreams(This)	\
    ( (This)->lpVtbl -> PauseStreams(This) ) 

#define IDeckLinkInput_v7_1_SetCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetCallback(This,theCallback) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkInput_v7_1_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkInputCallback_v7_3_INTERFACE_DEFINED__
#define __IDeckLinkInputCallback_v7_3_INTERFACE_DEFINED__

/* interface IDeckLinkInputCallback_v7_3 */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkInputCallback_v7_3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("FD6F311D-4D00-444B-9ED4-1F25B5730AD0")
    IDeckLinkInputCallback_v7_3 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged( 
            /* [in] */ BMDVideoInputFormatChangedEvents notificationEvents,
            /* [in] */ IDeckLinkDisplayMode *newDisplayMode,
            /* [in] */ BMDDetectedVideoInputFormatFlags detectedSignalFlags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived( 
            /* [in] */ IDeckLinkVideoInputFrame_v7_3 *videoFrame,
            /* [in] */ IDeckLinkAudioInputPacket *audioPacket) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkInputCallback_v7_3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkInputCallback_v7_3 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkInputCallback_v7_3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkInputCallback_v7_3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *VideoInputFormatChanged )( 
            IDeckLinkInputCallback_v7_3 * This,
            /* [in] */ BMDVideoInputFormatChangedEvents notificationEvents,
            /* [in] */ IDeckLinkDisplayMode *newDisplayMode,
            /* [in] */ BMDDetectedVideoInputFormatFlags detectedSignalFlags);
        
        HRESULT ( STDMETHODCALLTYPE *VideoInputFrameArrived )( 
            IDeckLinkInputCallback_v7_3 * This,
            /* [in] */ IDeckLinkVideoInputFrame_v7_3 *videoFrame,
            /* [in] */ IDeckLinkAudioInputPacket *audioPacket);
        
        END_INTERFACE
    } IDeckLinkInputCallback_v7_3Vtbl;

    interface IDeckLinkInputCallback_v7_3
    {
        CONST_VTBL struct IDeckLinkInputCallback_v7_3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkInputCallback_v7_3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkInputCallback_v7_3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkInputCallback_v7_3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkInputCallback_v7_3_VideoInputFormatChanged(This,notificationEvents,newDisplayMode,detectedSignalFlags)	\
    ( (This)->lpVtbl -> VideoInputFormatChanged(This,notificationEvents,newDisplayMode,detectedSignalFlags) ) 

#define IDeckLinkInputCallback_v7_3_VideoInputFrameArrived(This,videoFrame,audioPacket)	\
    ( (This)->lpVtbl -> VideoInputFrameArrived(This,videoFrame,audioPacket) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkInputCallback_v7_3_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkOutput_v7_3_INTERFACE_DEFINED__
#define __IDeckLinkOutput_v7_3_INTERFACE_DEFINED__

/* interface IDeckLinkOutput_v7_3 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkOutput_v7_3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("271C65E3-C323-4344-A30F-D908BCB20AA3")
    IDeckLinkOutput_v7_3 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoMode( 
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            /* [out] */ BMDDisplayModeSupport *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScreenPreviewCallback( 
            /* [in] */ IDeckLinkScreenPreviewCallback *previewCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoOutput( 
            BMDDisplayMode displayMode,
            BMDVideoOutputFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVideoOutput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetVideoOutputFrameMemoryAllocator( 
            /* [in] */ IDeckLinkMemoryAllocator *theAllocator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateVideoFrame( 
            long width,
            long height,
            long rowBytes,
            BMDPixelFormat pixelFormat,
            BMDFrameFlags flags,
            /* [out] */ IDeckLinkMutableVideoFrame **outFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateAncillaryData( 
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            /* [out] */ IDeckLinkVideoFrameAncillary **outBuffer) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisplayVideoFrameSync( 
            /* [in] */ IDeckLinkVideoFrame *theFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduleVideoFrame( 
            /* [in] */ IDeckLinkVideoFrame *theFrame,
            BMDTimeValue displayTime,
            BMDTimeValue displayDuration,
            BMDTimeScale timeScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScheduledFrameCompletionCallback( 
            /* [in] */ IDeckLinkVideoOutputCallback *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBufferedVideoFrameCount( 
            /* [out] */ unsigned long *bufferedFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAudioOutput( 
            BMDAudioSampleRate sampleRate,
            BMDAudioSampleType sampleType,
            unsigned long channelCount,
            BMDAudioOutputStreamType streamType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAudioOutput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE WriteAudioSamplesSync( 
            /* [in] */ void *buffer,
            unsigned long sampleFrameCount,
            /* [out] */ unsigned long *sampleFramesWritten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE BeginAudioPreroll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EndAudioPreroll( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ScheduleAudioSamples( 
            /* [in] */ void *buffer,
            unsigned long sampleFrameCount,
            BMDTimeValue streamTime,
            BMDTimeScale timeScale,
            /* [out] */ unsigned long *sampleFramesWritten) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBufferedAudioSampleFrameCount( 
            /* [out] */ unsigned long *bufferedSampleFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushBufferedAudioSamples( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetAudioCallback( 
            /* [in] */ IDeckLinkAudioOutputCallback *theCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartScheduledPlayback( 
            BMDTimeValue playbackStartTime,
            BMDTimeScale timeScale,
            double playbackSpeed) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopScheduledPlayback( 
            BMDTimeValue stopPlaybackAtTime,
            /* [out] */ BMDTimeValue *actualStopTime,
            BMDTimeScale timeScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsScheduledPlaybackRunning( 
            /* [out] */ BOOL *active) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHardwareReferenceClock( 
            BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *elapsedTimeSinceSchedulerBegan) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkOutput_v7_3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkOutput_v7_3 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkOutput_v7_3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkOutput_v7_3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoMode )( 
            IDeckLinkOutput_v7_3 * This,
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            /* [out] */ BMDDisplayModeSupport *result);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeIterator )( 
            IDeckLinkOutput_v7_3 * This,
            /* [out] */ IDeckLinkDisplayModeIterator **iterator);
        
        HRESULT ( STDMETHODCALLTYPE *SetScreenPreviewCallback )( 
            IDeckLinkOutput_v7_3 * This,
            /* [in] */ IDeckLinkScreenPreviewCallback *previewCallback);
        
        HRESULT ( STDMETHODCALLTYPE *EnableVideoOutput )( 
            IDeckLinkOutput_v7_3 * This,
            BMDDisplayMode displayMode,
            BMDVideoOutputFlags flags);
        
        HRESULT ( STDMETHODCALLTYPE *DisableVideoOutput )( 
            IDeckLinkOutput_v7_3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetVideoOutputFrameMemoryAllocator )( 
            IDeckLinkOutput_v7_3 * This,
            /* [in] */ IDeckLinkMemoryAllocator *theAllocator);
        
        HRESULT ( STDMETHODCALLTYPE *CreateVideoFrame )( 
            IDeckLinkOutput_v7_3 * This,
            long width,
            long height,
            long rowBytes,
            BMDPixelFormat pixelFormat,
            BMDFrameFlags flags,
            /* [out] */ IDeckLinkMutableVideoFrame **outFrame);
        
        HRESULT ( STDMETHODCALLTYPE *CreateAncillaryData )( 
            IDeckLinkOutput_v7_3 * This,
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            /* [out] */ IDeckLinkVideoFrameAncillary **outBuffer);
        
        HRESULT ( STDMETHODCALLTYPE *DisplayVideoFrameSync )( 
            IDeckLinkOutput_v7_3 * This,
            /* [in] */ IDeckLinkVideoFrame *theFrame);
        
        HRESULT ( STDMETHODCALLTYPE *ScheduleVideoFrame )( 
            IDeckLinkOutput_v7_3 * This,
            /* [in] */ IDeckLinkVideoFrame *theFrame,
            BMDTimeValue displayTime,
            BMDTimeValue displayDuration,
            BMDTimeScale timeScale);
        
        HRESULT ( STDMETHODCALLTYPE *SetScheduledFrameCompletionCallback )( 
            IDeckLinkOutput_v7_3 * This,
            /* [in] */ IDeckLinkVideoOutputCallback *theCallback);
        
        HRESULT ( STDMETHODCALLTYPE *GetBufferedVideoFrameCount )( 
            IDeckLinkOutput_v7_3 * This,
            /* [out] */ unsigned long *bufferedFrameCount);
        
        HRESULT ( STDMETHODCALLTYPE *EnableAudioOutput )( 
            IDeckLinkOutput_v7_3 * This,
            BMDAudioSampleRate sampleRate,
            BMDAudioSampleType sampleType,
            unsigned long channelCount,
            BMDAudioOutputStreamType streamType);
        
        HRESULT ( STDMETHODCALLTYPE *DisableAudioOutput )( 
            IDeckLinkOutput_v7_3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *WriteAudioSamplesSync )( 
            IDeckLinkOutput_v7_3 * This,
            /* [in] */ void *buffer,
            unsigned long sampleFrameCount,
            /* [out] */ unsigned long *sampleFramesWritten);
        
        HRESULT ( STDMETHODCALLTYPE *BeginAudioPreroll )( 
            IDeckLinkOutput_v7_3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *EndAudioPreroll )( 
            IDeckLinkOutput_v7_3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *ScheduleAudioSamples )( 
            IDeckLinkOutput_v7_3 * This,
            /* [in] */ void *buffer,
            unsigned long sampleFrameCount,
            BMDTimeValue streamTime,
            BMDTimeScale timeScale,
            /* [out] */ unsigned long *sampleFramesWritten);
        
        HRESULT ( STDMETHODCALLTYPE *GetBufferedAudioSampleFrameCount )( 
            IDeckLinkOutput_v7_3 * This,
            /* [out] */ unsigned long *bufferedSampleFrameCount);
        
        HRESULT ( STDMETHODCALLTYPE *FlushBufferedAudioSamples )( 
            IDeckLinkOutput_v7_3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetAudioCallback )( 
            IDeckLinkOutput_v7_3 * This,
            /* [in] */ IDeckLinkAudioOutputCallback *theCallback);
        
        HRESULT ( STDMETHODCALLTYPE *StartScheduledPlayback )( 
            IDeckLinkOutput_v7_3 * This,
            BMDTimeValue playbackStartTime,
            BMDTimeScale timeScale,
            double playbackSpeed);
        
        HRESULT ( STDMETHODCALLTYPE *StopScheduledPlayback )( 
            IDeckLinkOutput_v7_3 * This,
            BMDTimeValue stopPlaybackAtTime,
            /* [out] */ BMDTimeValue *actualStopTime,
            BMDTimeScale timeScale);
        
        HRESULT ( STDMETHODCALLTYPE *IsScheduledPlaybackRunning )( 
            IDeckLinkOutput_v7_3 * This,
            /* [out] */ BOOL *active);
        
        HRESULT ( STDMETHODCALLTYPE *GetHardwareReferenceClock )( 
            IDeckLinkOutput_v7_3 * This,
            BMDTimeScale desiredTimeScale,
            /* [out] */ BMDTimeValue *elapsedTimeSinceSchedulerBegan);
        
        END_INTERFACE
    } IDeckLinkOutput_v7_3Vtbl;

    interface IDeckLinkOutput_v7_3
    {
        CONST_VTBL struct IDeckLinkOutput_v7_3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkOutput_v7_3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkOutput_v7_3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkOutput_v7_3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkOutput_v7_3_DoesSupportVideoMode(This,displayMode,pixelFormat,result)	\
    ( (This)->lpVtbl -> DoesSupportVideoMode(This,displayMode,pixelFormat,result) ) 

#define IDeckLinkOutput_v7_3_GetDisplayModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDisplayModeIterator(This,iterator) ) 

#define IDeckLinkOutput_v7_3_SetScreenPreviewCallback(This,previewCallback)	\
    ( (This)->lpVtbl -> SetScreenPreviewCallback(This,previewCallback) ) 

#define IDeckLinkOutput_v7_3_EnableVideoOutput(This,displayMode,flags)	\
    ( (This)->lpVtbl -> EnableVideoOutput(This,displayMode,flags) ) 

#define IDeckLinkOutput_v7_3_DisableVideoOutput(This)	\
    ( (This)->lpVtbl -> DisableVideoOutput(This) ) 

#define IDeckLinkOutput_v7_3_SetVideoOutputFrameMemoryAllocator(This,theAllocator)	\
    ( (This)->lpVtbl -> SetVideoOutputFrameMemoryAllocator(This,theAllocator) ) 

#define IDeckLinkOutput_v7_3_CreateVideoFrame(This,width,height,rowBytes,pixelFormat,flags,outFrame)	\
    ( (This)->lpVtbl -> CreateVideoFrame(This,width,height,rowBytes,pixelFormat,flags,outFrame) ) 

#define IDeckLinkOutput_v7_3_CreateAncillaryData(This,displayMode,pixelFormat,outBuffer)	\
    ( (This)->lpVtbl -> CreateAncillaryData(This,displayMode,pixelFormat,outBuffer) ) 

#define IDeckLinkOutput_v7_3_DisplayVideoFrameSync(This,theFrame)	\
    ( (This)->lpVtbl -> DisplayVideoFrameSync(This,theFrame) ) 

#define IDeckLinkOutput_v7_3_ScheduleVideoFrame(This,theFrame,displayTime,displayDuration,timeScale)	\
    ( (This)->lpVtbl -> ScheduleVideoFrame(This,theFrame,displayTime,displayDuration,timeScale) ) 

#define IDeckLinkOutput_v7_3_SetScheduledFrameCompletionCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetScheduledFrameCompletionCallback(This,theCallback) ) 

#define IDeckLinkOutput_v7_3_GetBufferedVideoFrameCount(This,bufferedFrameCount)	\
    ( (This)->lpVtbl -> GetBufferedVideoFrameCount(This,bufferedFrameCount) ) 

#define IDeckLinkOutput_v7_3_EnableAudioOutput(This,sampleRate,sampleType,channelCount,streamType)	\
    ( (This)->lpVtbl -> EnableAudioOutput(This,sampleRate,sampleType,channelCount,streamType) ) 

#define IDeckLinkOutput_v7_3_DisableAudioOutput(This)	\
    ( (This)->lpVtbl -> DisableAudioOutput(This) ) 

#define IDeckLinkOutput_v7_3_WriteAudioSamplesSync(This,buffer,sampleFrameCount,sampleFramesWritten)	\
    ( (This)->lpVtbl -> WriteAudioSamplesSync(This,buffer,sampleFrameCount,sampleFramesWritten) ) 

#define IDeckLinkOutput_v7_3_BeginAudioPreroll(This)	\
    ( (This)->lpVtbl -> BeginAudioPreroll(This) ) 

#define IDeckLinkOutput_v7_3_EndAudioPreroll(This)	\
    ( (This)->lpVtbl -> EndAudioPreroll(This) ) 

#define IDeckLinkOutput_v7_3_ScheduleAudioSamples(This,buffer,sampleFrameCount,streamTime,timeScale,sampleFramesWritten)	\
    ( (This)->lpVtbl -> ScheduleAudioSamples(This,buffer,sampleFrameCount,streamTime,timeScale,sampleFramesWritten) ) 

#define IDeckLinkOutput_v7_3_GetBufferedAudioSampleFrameCount(This,bufferedSampleFrameCount)	\
    ( (This)->lpVtbl -> GetBufferedAudioSampleFrameCount(This,bufferedSampleFrameCount) ) 

#define IDeckLinkOutput_v7_3_FlushBufferedAudioSamples(This)	\
    ( (This)->lpVtbl -> FlushBufferedAudioSamples(This) ) 

#define IDeckLinkOutput_v7_3_SetAudioCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetAudioCallback(This,theCallback) ) 

#define IDeckLinkOutput_v7_3_StartScheduledPlayback(This,playbackStartTime,timeScale,playbackSpeed)	\
    ( (This)->lpVtbl -> StartScheduledPlayback(This,playbackStartTime,timeScale,playbackSpeed) ) 

#define IDeckLinkOutput_v7_3_StopScheduledPlayback(This,stopPlaybackAtTime,actualStopTime,timeScale)	\
    ( (This)->lpVtbl -> StopScheduledPlayback(This,stopPlaybackAtTime,actualStopTime,timeScale) ) 

#define IDeckLinkOutput_v7_3_IsScheduledPlaybackRunning(This,active)	\
    ( (This)->lpVtbl -> IsScheduledPlaybackRunning(This,active) ) 

#define IDeckLinkOutput_v7_3_GetHardwareReferenceClock(This,desiredTimeScale,elapsedTimeSinceSchedulerBegan)	\
    ( (This)->lpVtbl -> GetHardwareReferenceClock(This,desiredTimeScale,elapsedTimeSinceSchedulerBegan) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkOutput_v7_3_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkInput_v7_3_INTERFACE_DEFINED__
#define __IDeckLinkInput_v7_3_INTERFACE_DEFINED__

/* interface IDeckLinkInput_v7_3 */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkInput_v7_3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("4973F012-9925-458C-871C-18774CDBBECB")
    IDeckLinkInput_v7_3 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DoesSupportVideoMode( 
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            /* [out] */ BMDDisplayModeSupport *result) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeIterator( 
            /* [out] */ IDeckLinkDisplayModeIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetScreenPreviewCallback( 
            /* [in] */ IDeckLinkScreenPreviewCallback *previewCallback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableVideoInput( 
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            BMDVideoInputFlags flags) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVideoInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableVideoFrameCount( 
            /* [out] */ unsigned long *availableFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EnableAudioInput( 
            BMDAudioSampleRate sampleRate,
            BMDAudioSampleType sampleType,
            unsigned long channelCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableAudioInput( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAvailableAudioSampleFrameCount( 
            /* [out] */ unsigned long *availableSampleFrameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StartStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE StopStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PauseStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushStreams( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCallback( 
            /* [in] */ IDeckLinkInputCallback_v7_3 *theCallback) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkInput_v7_3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkInput_v7_3 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkInput_v7_3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkInput_v7_3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *DoesSupportVideoMode )( 
            IDeckLinkInput_v7_3 * This,
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            /* [out] */ BMDDisplayModeSupport *result);
        
        HRESULT ( STDMETHODCALLTYPE *GetDisplayModeIterator )( 
            IDeckLinkInput_v7_3 * This,
            /* [out] */ IDeckLinkDisplayModeIterator **iterator);
        
        HRESULT ( STDMETHODCALLTYPE *SetScreenPreviewCallback )( 
            IDeckLinkInput_v7_3 * This,
            /* [in] */ IDeckLinkScreenPreviewCallback *previewCallback);
        
        HRESULT ( STDMETHODCALLTYPE *EnableVideoInput )( 
            IDeckLinkInput_v7_3 * This,
            BMDDisplayMode displayMode,
            BMDPixelFormat pixelFormat,
            BMDVideoInputFlags flags);
        
        HRESULT ( STDMETHODCALLTYPE *DisableVideoInput )( 
            IDeckLinkInput_v7_3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAvailableVideoFrameCount )( 
            IDeckLinkInput_v7_3 * This,
            /* [out] */ unsigned long *availableFrameCount);
        
        HRESULT ( STDMETHODCALLTYPE *EnableAudioInput )( 
            IDeckLinkInput_v7_3 * This,
            BMDAudioSampleRate sampleRate,
            BMDAudioSampleType sampleType,
            unsigned long channelCount);
        
        HRESULT ( STDMETHODCALLTYPE *DisableAudioInput )( 
            IDeckLinkInput_v7_3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAvailableAudioSampleFrameCount )( 
            IDeckLinkInput_v7_3 * This,
            /* [out] */ unsigned long *availableSampleFrameCount);
        
        HRESULT ( STDMETHODCALLTYPE *StartStreams )( 
            IDeckLinkInput_v7_3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *StopStreams )( 
            IDeckLinkInput_v7_3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *PauseStreams )( 
            IDeckLinkInput_v7_3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *FlushStreams )( 
            IDeckLinkInput_v7_3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetCallback )( 
            IDeckLinkInput_v7_3 * This,
            /* [in] */ IDeckLinkInputCallback_v7_3 *theCallback);
        
        END_INTERFACE
    } IDeckLinkInput_v7_3Vtbl;

    interface IDeckLinkInput_v7_3
    {
        CONST_VTBL struct IDeckLinkInput_v7_3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkInput_v7_3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkInput_v7_3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkInput_v7_3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkInput_v7_3_DoesSupportVideoMode(This,displayMode,pixelFormat,result)	\
    ( (This)->lpVtbl -> DoesSupportVideoMode(This,displayMode,pixelFormat,result) ) 

#define IDeckLinkInput_v7_3_GetDisplayModeIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetDisplayModeIterator(This,iterator) ) 

#define IDeckLinkInput_v7_3_SetScreenPreviewCallback(This,previewCallback)	\
    ( (This)->lpVtbl -> SetScreenPreviewCallback(This,previewCallback) ) 

#define IDeckLinkInput_v7_3_EnableVideoInput(This,displayMode,pixelFormat,flags)	\
    ( (This)->lpVtbl -> EnableVideoInput(This,displayMode,pixelFormat,flags) ) 

#define IDeckLinkInput_v7_3_DisableVideoInput(This)	\
    ( (This)->lpVtbl -> DisableVideoInput(This) ) 

#define IDeckLinkInput_v7_3_GetAvailableVideoFrameCount(This,availableFrameCount)	\
    ( (This)->lpVtbl -> GetAvailableVideoFrameCount(This,availableFrameCount) ) 

#define IDeckLinkInput_v7_3_EnableAudioInput(This,sampleRate,sampleType,channelCount)	\
    ( (This)->lpVtbl -> EnableAudioInput(This,sampleRate,sampleType,channelCount) ) 

#define IDeckLinkInput_v7_3_DisableAudioInput(This)	\
    ( (This)->lpVtbl -> DisableAudioInput(This) ) 

#define IDeckLinkInput_v7_3_GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount)	\
    ( (This)->lpVtbl -> GetAvailableAudioSampleFrameCount(This,availableSampleFrameCount) ) 

#define IDeckLinkInput_v7_3_StartStreams(This)	\
    ( (This)->lpVtbl -> StartStreams(This) ) 

#define IDeckLinkInput_v7_3_StopStreams(This)	\
    ( (This)->lpVtbl -> StopStreams(This) ) 

#define IDeckLinkInput_v7_3_PauseStreams(This)	\
    ( (This)->lpVtbl -> PauseStreams(This) ) 

#define IDeckLinkInput_v7_3_FlushStreams(This)	\
    ( (This)->lpVtbl -> FlushStreams(This) ) 

#define IDeckLinkInput_v7_3_SetCallback(This,theCallback)	\
    ( (This)->lpVtbl -> SetCallback(This,theCallback) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkInput_v7_3_INTERFACE_DEFINED__ */


#ifndef __IDeckLinkVideoInputFrame_v7_3_INTERFACE_DEFINED__
#define __IDeckLinkVideoInputFrame_v7_3_INTERFACE_DEFINED__

/* interface IDeckLinkVideoInputFrame_v7_3 */
/* [helpstring][local][uuid][object] */ 


EXTERN_C const IID IID_IDeckLinkVideoInputFrame_v7_3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("CF317790-2894-11DE-8C30-0800200C9A66")
    IDeckLinkVideoInputFrame_v7_3 : public IDeckLinkVideoFrame
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetStreamTime( 
            /* [out] */ BMDTimeValue *frameTime,
            /* [out] */ BMDTimeValue *frameDuration,
            BMDTimeScale timeScale) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDeckLinkVideoInputFrame_v7_3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDeckLinkVideoInputFrame_v7_3 * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDeckLinkVideoInputFrame_v7_3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDeckLinkVideoInputFrame_v7_3 * This);
        
        long ( STDMETHODCALLTYPE *GetWidth )( 
            IDeckLinkVideoInputFrame_v7_3 * This);
        
        long ( STDMETHODCALLTYPE *GetHeight )( 
            IDeckLinkVideoInputFrame_v7_3 * This);
        
        long ( STDMETHODCALLTYPE *GetRowBytes )( 
            IDeckLinkVideoInputFrame_v7_3 * This);
        
        BMDPixelFormat ( STDMETHODCALLTYPE *GetPixelFormat )( 
            IDeckLinkVideoInputFrame_v7_3 * This);
        
        BMDFrameFlags ( STDMETHODCALLTYPE *GetFlags )( 
            IDeckLinkVideoInputFrame_v7_3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBytes )( 
            IDeckLinkVideoInputFrame_v7_3 * This,
            /* [out] */ void **buffer);
        
        HRESULT ( STDMETHODCALLTYPE *GetTimecode )( 
            IDeckLinkVideoInputFrame_v7_3 * This,
            BMDTimecodeFormat format,
            /* [out] */ IDeckLinkTimecode **timecode);
        
        HRESULT ( STDMETHODCALLTYPE *GetAncillaryData )( 
            IDeckLinkVideoInputFrame_v7_3 * This,
            /* [out] */ IDeckLinkVideoFrameAncillary **ancillary);
        
        HRESULT ( STDMETHODCALLTYPE *GetStreamTime )( 
            IDeckLinkVideoInputFrame_v7_3 * This,
            /* [out] */ BMDTimeValue *frameTime,
            /* [out] */ BMDTimeValue *frameDuration,
            BMDTimeScale timeScale);
        
        END_INTERFACE
    } IDeckLinkVideoInputFrame_v7_3Vtbl;

    interface IDeckLinkVideoInputFrame_v7_3
    {
        CONST_VTBL struct IDeckLinkVideoInputFrame_v7_3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDeckLinkVideoInputFrame_v7_3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDeckLinkVideoInputFrame_v7_3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDeckLinkVideoInputFrame_v7_3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDeckLinkVideoInputFrame_v7_3_GetWidth(This)	\
    ( (This)->lpVtbl -> GetWidth(This) ) 

#define IDeckLinkVideoInputFrame_v7_3_GetHeight(This)	\
    ( (This)->lpVtbl -> GetHeight(This) ) 

#define IDeckLinkVideoInputFrame_v7_3_GetRowBytes(This)	\
    ( (This)->lpVtbl -> GetRowBytes(This) ) 

#define IDeckLinkVideoInputFrame_v7_3_GetPixelFormat(This)	\
    ( (This)->lpVtbl -> GetPixelFormat(This) ) 

#define IDeckLinkVideoInputFrame_v7_3_GetFlags(This)	\
    ( (This)->lpVtbl -> GetFlags(This) ) 

#define IDeckLinkVideoInputFrame_v7_3_GetBytes(This,buffer)	\
    ( (This)->lpVtbl -> GetBytes(This,buffer) ) 

#define IDeckLinkVideoInputFrame_v7_3_GetTimecode(This,format,timecode)	\
    ( (This)->lpVtbl -> GetTimecode(This,format,timecode) ) 

#define IDeckLinkVideoInputFrame_v7_3_GetAncillaryData(This,ancillary)	\
    ( (This)->lpVtbl -> GetAncillaryData(This,ancillary) ) 


#define IDeckLinkVideoInputFrame_v7_3_GetStreamTime(This,frameTime,frameDuration,timeScale)	\
    ( (This)->lpVtbl -> GetStreamTime(This,frameTime,frameDuration,timeScale) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDeckLinkVideoInputFrame_v7_3_INTERFACE_DEFINED__ */

#endif /* __DeckLinkAPI_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


