/*
 $Id: BlueDriver_p.h,v 1.24.2.4 2009/08/29 04:31:59 tim Exp $
*/
#pragma once 
#define _BLUEDRIVER_P_H__
#define BLUE_UINT32  	unsigned int 
#define BLUE_INT32  	int 
#define BLUE_UINT8 	unsigned char
#define BLUE_INT8 	char
#define BLUE_UINT16	unsigned short
#define BLUE_INT16	short
#pragma once
#ifdef BLUE_LINUX_CODE
#define BLUE_UINT64 unsigned long long 
//#include <asm/types.h>
#else
#define BLUE_UINT64	unsigned __int64
#endif 


/**
 * This contains the enumerators that can be used to set the cards \n
 * video output and also to determine the video mode of the incoming \n
 * video signal.
 */ 
typedef enum _EVideoMode
{
	VID_FMT_PAL=0,
	VID_FMT_NTSC=1,
	VID_FMT_576I_5000=0,	/**< 720  x 576  50       Interlaced */
	VID_FMT_486I_5994=1,	/**< 720  x 486  60/1.001 Interlaced */
	VID_FMT_720P_5994,		/**< 1280 x 720  60/1.001 Progressive */
	VID_FMT_720P_6000,		/**< 1280 x 720  60       Progressive */
	VID_FMT_1080PSF_2397,	/**< 1920 x 1080 24/1.001 Segment Frame */
	VID_FMT_1080PSF_2400,	/**< 1920 x 1080 24       Segment Frame */
	VID_FMT_1080P_2397,		/**< 1920 x 1080 24/1.001 Progressive */
	VID_FMT_1080P_2400,		/**< 1920 x 1080 24       Progressive */
	VID_FMT_1080I_5000,		/**< 1920 x 1080 50       Interlaced */
	VID_FMT_1080I_5994,		/**< 1920 x 1080 60/1.001 Interlaced */
	VID_FMT_1080I_6000,		/**< 1920 x 1080 60       Interlaced */
	VID_FMT_1080P_2500,		/**< 1920 x 1080 25       Progressive */
	VID_FMT_1080P_2997,		/**< 1920 x 1080 30/1.001 Progressive */
	VID_FMT_1080P_3000,		/**< 1920 x 1080 30       Progressive */
	VID_FMT_HSDL_1498,		/**< 2048 x 1556 15/1.0   Segment Frame */
	VID_FMT_HSDL_1500,		/**< 2048 x 1556 15		Segment Frame */
	VID_FMT_720P_5000,
	VID_FMT_720P_2398,
	VID_FMT_720P_2400,
	VID_FMT_2048_1080PSF_2397=19,
	VID_FMT_2048_1080PSF_2400=20,
	VID_FMT_2048_1080P_2397=21,
	VID_FMT_2048_1080P_2400=22,
	VID_FMT_1080PSF_2500=23,
	VID_FMT_1080PSF_2997=24,
	VID_FMT_1080PSF_3000=25,
	VID_FMT_1080P_5000=26,
	VID_FMT_1080P_5994=27,
	VID_FMT_1080P_6000=28,
	VID_FMT_720P_2500=29,
	VID_FMT_720P_2997=30,
	VID_FMT_720P_3000=31,
	VID_FMT_INVALID=32
} EVideoMode;

typedef enum _EMemoryFormat
{
	MEM_FMT_ARGB=0,
	MEM_FMT_BV10=1,
	MEM_FMT_BV8=2,
	MEM_FMT_YUVS=MEM_FMT_BV8,
	MEM_FMT_V210=3,	// Iridium HD (BAG1)
	MEM_FMT_RGBA=4,
	MEM_FMT_CINEON_LITTLE_ENDIAN=5,
	MEM_FMT_ARGB_PC=6,
	MEM_FMT_BGRA=MEM_FMT_ARGB_PC,
	MEM_FMT_CINEON=7,
	MEM_FMT_2VUY=8,
	MEM_FMT_BGR=9,
	MEM_FMT_INVALID=10
} EMemoryFormat;

typedef enum _EUpdateMethod
{
	UPD_FMT_FIELD=0,
	UPD_FMT_FRAME,
	UPD_FMT_FRAME_DISPLAY_FIELD1,
	UPD_FMT_FRAME_DISPLAY_FIELD2,
	UPD_FMT_INVALID
} EUpdateMethod;

typedef enum _EResoFormat
{
	RES_FMT_NORMAL=0,
	RES_FMT_HALF,
	RES_FMT_INVALID
} EResoFormat;

typedef enum _ECardType
{
	CRD_BLUEDEEP_LT=0,		// D64 Lite
	CRD_BLUEDEEP_SD,		// Iridium SD
	CRD_BLUEDEEP_AV,		// Iridium AV
	CRD_BLUEDEEP_IO,		// D64 Full
	CRD_BLUEWILD_AV,		// D64 AV
	CRD_IRIDIUM_HD,			// * Iridium HD
	CRD_BLUEWILD_RT,		// D64 RT
	CRD_BLUEWILD_HD,		// * BadAss G2
	CRD_REDDEVIL,			// Iridium Full
	CRD_BLUEDEEP_HD,		// * BadAss G2 variant, proposed, reserved
	CRD_BLUE_EPOCH_2K = CRD_BLUEDEEP_HD,
	CRD_BLUE_EPOCH_2K_HORIZON=CRD_BLUE_EPOCH_2K,
	CRD_BLUEDEEP_HDS,		// * BadAss G2 variant, proposed, reserved
	CRD_BLUE_ENVY,			// Mini Din 
	CRD_BLUE_PRIDE,			//Mini Din Output 
	CRD_BLUE_GREED,
	CRD_BLUE_INGEST,
	CRD_BLUE_SD_DUALLINK,
	CRD_BLUE_CATALYST,
	CRD_BLUE_SD_DUALLINK_PRO,
	CRD_BLUE_SD_INGEST_PRO,
	CRD_BLUE_SD_DEEPBLUE_LITE_PRO,
	CRD_BLUE_SD_SINGLELINK_PRO,
	CRD_BLUE_SD_IRIDIUM_AV_PRO,
	CRD_BLUE_SD_FIDELITY,
	CRD_BLUE_SD_FOCUS,
	CRD_BLUE_SD_PRIME,
	CRD_BLUE_EPOCH_2K_CORE,
	CRD_BLUE_EPOCH_2K_ULTRA,
	CRD_BLUE_EPOCH_HORIZON,
	CRD_BLUE_EPOCH_CORE,
	CRD_BLUE_EPOCH_ULTRA,
	CRD_BLUE_CREATE_HD,
	CRD_BLUE_CREATE_2K,
	CRD_BLUE_CREATE_2K_ULTRA,
	CRD_INVALID
} ECardType;


typedef enum _EHDCardSubType
{	
	CRD_HD_FURY=1,
	CRD_HD_VENGENCE=2,
	CRD_HD_IRIDIUM_XP=3,
	CRD_HD_IRIDIUM = 4,
	CRD_HD_LUST=5,
	CRD_HD_INVALID
}EHDCardSubType;

/* To be used by the new audio interface only */
typedef enum _BlueAudioChannelDesc
{
	MONO_FLAG		= 0xC0000000,
	MONO_CHANNEL_1	= 0x00000001,
	MONO_CHANNEL_2	= 0x00000002,
	MONO_CHANNEL_3	= 0x00000004,
	MONO_CHANNEL_4	= 0x00000008,
	MONO_CHANNEL_5	= 0x00000010,
	MONO_CHANNEL_6	= 0x00000020,
	MONO_CHANNEL_7	= 0x00000040,
	MONO_CHANNEL_8	= 0x00000080,
	MONO_CHANNEL_9	= 0x00000100,// to be used by analog audio output channels 
	MONO_CHANNEL_10	= 0x00000200,// to be used by analog audio output channels 
	MONO_CHANNEL_11	= 0x00000400,//actual channel 9
	MONO_CHANNEL_12	= 0x00000800,//actual channel 10
	MONO_CHANNEL_13	= 0x00001000,//actual channel 11
	MONO_CHANNEL_14	= 0x00002000,//actual channel 12
	MONO_CHANNEL_15	= 0x00004000,//actual channel 13
	MONO_CHANNEL_16	= 0x00008000,//actual channel 14
	MONO_CHANNEL_17	= 0x00010000,//actual channel 15
	MONO_CHANNEL_18	= 0x00020000 //actual channel 16
}BlueAudioChannelDesc;


//----------------------------------------------------------------------------
typedef enum
{
	AUDIO_CHANNEL_LOOPING_OFF		= 0x00000000,
	AUDIO_CHANNEL_LOOPING			= 0x00000001,
	AUDIO_CHANNEL_LITTLEENDIAN		= 0x00000000,
	AUDIO_CHANNEL_BIGENDIAN			= 0x00000002,
	AUDIO_CHANNEL_OFFSET_IN_BYTES	= 0x00000004,
	AUDIO_CHANNEL_16BIT				= 0x00000008,
	AUDIO_CHANNEL_BLIP_PENDING		= 0x00000010,
	AUDIO_CHANNEL_BLIP_COMPLETE		= 0x00000020,
	AUDIO_CHANNEL_SELECT_CHANNEL	= 0x00000040,
	AUDIO_CHANNEL_24BIT				= 0x00000080
} EAudioFlags;

/**
@desc Used to select Audio input source on new generation SD cards 
@remarks
This enumerator works only when used with ReadAudioSample function.
*/
typedef enum
{
	BLUE_AUDIO_AES=0, /**< Used to select All 8 channels of Digital Audio using AES/AES3id  connector*/
	BLUE_AUDIO_ANALOG=1,/**< Used to select Analog audio*/
	BLUE_AUDIO_SDIA=2, /**< Used to select Emb audio from SDI A */
	BLUE_AUDIO_EMBEDDED=BLUE_AUDIO_SDIA,
	BLUE_AUDIO_SDIB=3, /**< Used to select Emb audio from SDI B */
	BLUE_AUDIO_AES_PAIR0=4,
	BLUE_AUDIO_AES_PAIR1=5,
	BLUE_AUDIO_AES_PAIR2=6,
	BLUE_AUDIO_AES_PAIR3=7,
	BLUE_AUDIO_INVALID=8
} Blue_Audio_Connector_Type;

typedef enum _EAudioRate
{
	AUDIO_SAMPLE_RATE_48K=48000,
	AUDIO_SAMPLE_RATE_96K=96000,
	AUDIO_SAMPLE_RATE_UNKNOWN=-1
} EAudioRate;

typedef enum _EConnectorSignalColorSpace
{
	RGB_ON_CONNECTOR=0x00400000,
	YUV_ON_CONNECTOR=0
}EConnectorSignalColorSpace;

typedef enum _EDualLinkSignalFormatType
{
	Signal_FormatType_4224=0,
	Signal_FormatType_4444=1,
	Signal_FormatType_Independent_422=10,
	Signal_FormatType_Dependent_422=11,
	Signal_FormatType_Key_Key=0x8000
}EDualLinkSignalFormatType;

typedef enum _EPreDefinedColorSpaceMatrix
{
	UNITY_MATRIX=0,
	MATRIX_709_CGR=1,
	MATRIX_RGB_TO_YUV_709_CGR=MATRIX_709_CGR,
	MATRIX_709=2,
	MATRIX_RGB_TO_YUV_709=MATRIX_709,
	RGB_FULL_RGB_SMPTE=3,
	MATRIX_601_CGR=4,
	MATRIX_RGB_TO_YUV_601_CGR=MATRIX_601_CGR,
	MATRIX_601=5,
	MATRIX_RGB_TO_YUV_601=MATRIX_601,
	MATRIX_SMPTE_274_CGR=6,
	MATRIX_SMPTE_274=7,
	MATRIX_VUYA=8,
	UNITY_MATRIX_INPUT=9,
	MATRIX_YUV_TO_RGB_709_CGR=10,
	MATRIX_YUV_TO_RGB_709=11,
	RGB_SMPTE_RGB_FULL=12,
	MATRIX_YUV_TO_RGB_601_CGR=13,
	MATRIX_YUV_TO_RGB_601=14,
	MATRIX_USER_DEFINED=15,
}EPreDefinedColorSpaceMatrix;

#ifndef BLUE_LINUX_CODE
typedef enum
{
	BLUE_FIFO_CLOSED=0,
	BLUE_FIFO_STARTING=1,
	BLUE_FIFO_RUNNING=2,
	BLUE_FIFO_STOPPING=3,
	BLUE_FIFO_PASSIVE=5,
}BlueVideoFifoStatus;
#endif
typedef enum _ERGBDataRange
{
	CGR_RANGE=0, //0-255
	SMPTE_RANGE=1 //16-235
}ERGBDataRange;

typedef enum _EHD_XCONNECTOR_MODE
{
	SD_SDI=1,
	HD_SDI=2
}EHD_XCONNECTOR_MODE;

typedef enum _EImageOrientation
{
	ImageOrientation_Normal=0,	
	ImageOrientation_VerticalFlip=1,
	ImageOrientation_Invalid=2,
}EImageOrientation;

typedef enum _EBlueGenlockSource
{
	BlueGenlockBNC=0,
	BlueSDIBNC=0x10000,
	BlueSDI_B_BNC=BlueSDIBNC,
	BlueSDI_A_BNC=0x20000,
	BlueAnalog_BNC=0x40000,
	BlueSoftware=0x80000,
}EBlueGenlockSource;

typedef enum _EBlueVideoChannel
{
	BLUE_VIDEOCHANNEL_A=0,
	BLUE_VIDEO_OUTPUT_CHANNEL_A=BLUE_VIDEOCHANNEL_A,
	
	BLUE_VIDEOCHANNEL_B=1,
	BLUE_VIDEO_OUTPUT_CHANNEL_B=BLUE_VIDEOCHANNEL_B,
	
	BLUE_VIDEOCHANNEL_C=2,
	BLUE_VIDEO_INPUT_CHANNEL_A=BLUE_VIDEOCHANNEL_C,

	BLUE_VIDEOCHANNEL_D=3,
	BLUE_VIDEO_INPUT_CHANNEL_B=BLUE_VIDEOCHANNEL_D,
	
	BLUE_OUTPUT_MEM_MODULE_A=BLUE_VIDEO_OUTPUT_CHANNEL_A,
	BLUE_OUTPUT_MEM_MODULE_B=BLUE_VIDEO_OUTPUT_CHANNEL_B,
	BLUE_INPUT_MEM_MODULE_A=BLUE_VIDEO_INPUT_CHANNEL_A,
	BLUE_INPUT_MEM_MODULE_B=BLUE_VIDEO_INPUT_CHANNEL_B,
	BLUE_JETSTREAM_SCALER_MODULE_0=0x10,
	BLUE_JETSTREAM_SCALER_MODULE_1=0x11,
	BLUE_JETSTREAM_SCALER_MODULE_2=0x12,
	BLUE_JETSTREAM_SCALER_MODULE_3=0x13,

	BLUE_VIDEOCHANNEL_INVALID=30
}EBlueVideoChannel;

typedef enum _EBlueVideoRouting
{
	BLUE_VIDEO_LINK_INVALID=0,
	BLUE_SDI_A_LINK1=4,
	BLUE_SDI_A_LINK2=5,
	BLUE_SDI_B_LINK1=6,
	BLUE_SDI_B_LINK2=7,
	BLUE_ANALOG_LINK1=8,
	BLUE_ANALOG_LINK2=9,
	BLUE_SDI_A_SINGLE_LINK=BLUE_SDI_A_LINK1,
	BLUE_SDI_B_SINGLE_LINK=BLUE_SDI_B_LINK1,
	BLUE_ANALOG_SINGLE_LINK=BLUE_ANALOG_LINK1

}EBlueVideoRouting;

typedef enum
{
	BLUE_FIFO_NULL_ATTRIBUTE=0x0,
	BLUE_FIFO_ECHOPORT_ENABLED=0x1,
	BLUE_FIFO_STEPMODE = 0x2,
	BLUE_FIFO_LOOPMODE = 0x4
}BlueVideoFifo_Attributes;

typedef enum _BlueAudioOutputDest
{
	Blue_AnalogAudio_Output=0x0,
	Blue_AES_Output=0x80000000,
	Blue_Emb_Output=0x40000000,
}BlueAudioOutputDest;


typedef enum _BlueAudioInputSource
{
	Blue_AES=0x10,
	Blue_AnalogAudio=0x20,
	Blue_SDIA_Embed=0x40,
	Blue_SDIB_Embed=0x80,
}BlueAudioInputSource;

typedef enum _EBlueConnectorIdentifier
{
	BLUE_CONNECTOR_INVALID = -1,
	
	// BNC connectors in order from top to bottom of shield
	BLUE_CONNECTOR_BNC_A = 0,    // BNC closest to top of shield
	BLUE_CONNECTOR_BNC_B,
	BLUE_CONNECTOR_BNC_C,
	BLUE_CONNECTOR_BNC_D,
	BLUE_CONNECTOR_BNC_E,
	BLUE_CONNECTOR_BNC_F,
	BLUE_CONNECTOR_GENLOCK,
	
	BLUE_CONNECTOR_ANALOG_VIDEO_1 = 100,
	BLUE_CONNECTOR_ANALOG_VIDEO_2,
	BLUE_CONNECTOR_ANALOG_VIDEO_3,
	BLUE_CONNECTOR_ANALOG_VIDEO_4,
	BLUE_CONNECTOR_ANALOG_VIDEO_5,
	BLUE_CONNECTOR_ANALOG_VIDEO_6,

	BLUE_CONNECTOR_DVID_1 = 200,
	BLUE_CONNECTOR_SDI_OUTPUT_A= BLUE_CONNECTOR_DVID_1,
	BLUE_CONNECTOR_DVID_2,
	BLUE_CONNECTOR_SDI_OUTPUT_B= BLUE_CONNECTOR_DVID_2,
	BLUE_CONNECTOR_DVID_3,
	BLUE_CONNECTOR_SDI_INPUT_A= BLUE_CONNECTOR_DVID_3,
	BLUE_CONNECTOR_DVID_4,
	BLUE_CONNECTOR_SDI_INPUT_B= BLUE_CONNECTOR_DVID_4,
	BLUE_CONNECTOR_DVID_5,

	BLUE_CONNECTOR_AES = 300,
	BLUE_CONNECTOR_ANALOG_AUDIO_1,
	BLUE_CONNECTOR_ANALOG_AUDIO_2,

	BLUE_CONNECTOR_RESOURCE_BLOCK=0x400,
	BLUE_CONNECTOR_JETSTREAM_SCALER_0=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_JETSTREAM_SCALER_MODULE_0),
	BLUE_CONNECTOR_JETSTREAM_SCALER_1=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_JETSTREAM_SCALER_MODULE_1),
	BLUE_CONNECTOR_JETSTREAM_SCALER_2=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_JETSTREAM_SCALER_MODULE_2),
	BLUE_CONNECTOR_JETSTREAM_SCALER_3=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_JETSTREAM_SCALER_MODULE_3),

	BLUE_CONNECTOR_OUTPUT_MEM_MODULE_A=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_OUTPUT_MEM_MODULE_A),
	BLUE_CONNECTOR_OUTPUT_MEM_MODULE_B=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_OUTPUT_MEM_MODULE_B),
	BLUE_CONNECTOR_INPUT_MEM_MODULE_A=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_INPUT_MEM_MODULE_A),
	BLUE_CONNECTOR_INPUT_MEM_MODULE_B=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_INPUT_MEM_MODULE_B),
	
}EBlueConnectorIdentifier;

typedef enum _EBlueConnectorSignalDirection
{
	BLUE_CONNECTOR_SIGNAL_INVALID=-1,
	BLUE_CONNECTOR_SIGNAL_INPUT=0,
	BLUE_CONNECTOR_SIGNAL_OUTPUT=1,
}EBlueConnectorSignalDirection;

typedef enum _EBlueConnectorProperty
{
	BLUE_INVALID_CONNECTOR_PROPERTY = -1,

	//signal property
	BLUE_CONNECTOR_PROP_INPUT_SIGNAL=0,
	BLUE_CONNECTOR_PROP_OUTPUT_SIGNAL=1,

	// Video output
	BLUE_CONNECTOR_PROP_SDI = 0,
	BLUE_CONNECTOR_PROP_YUV_Y,
	BLUE_CONNECTOR_PROP_YUV_U,
	BLUE_CONNECTOR_PROP_YUV_V,
	BLUE_CONNECTOR_PROP_RGB_R,
	BLUE_CONNECTOR_PROP_RGB_G,
	BLUE_CONNECTOR_PROP_RGB_B,
	BLUE_CONNECTOR_PROP_CVBS,
	BLUE_CONNECTOR_PROP_SVIDEO_Y,
	BLUE_CONNECTOR_PROP_SVIDEO_C,
	
	// Audio output
	BLUE_CONNECTOR_PROP_AUDIO_AES = 0x2000,
	BLUE_CONNECTOR_PROP_AUDIO_EMBEDDED,
	BLUE_CONNECTOR_PROP_AUDIO_ANALOG,

	
	BLUE_CONNECTOR_PROP_SINGLE_LINK=0x3000,
	BLUE_CONNECTOR_PROP_DUALLINK_LINK_1,
	BLUE_CONNECTOR_PROP_DUALLINK_LINK_2,
	BLUE_CONNECTOR_PROP_DUALLINK_LINK,
}EBlueConnectorProperty;

/*
typedef enum _BLUE_AUDIOINPUT_SOURCE
{
	BLUE_AES_AUDIO_INPUT=0x10000,
	BLUE_ANALOG_AUDIO_INPUT=0x20000,
	BLUE_SDIA_AUDIO_INPUT=0x30000,
	BLUE_SDIB_AUDIO_INPUT=0x40000
}BLUE_AUDIOINPUT_SOURCE;
*/

typedef enum _EBlueCardProperty
{
	VIDEO_DUAL_LINK_OUTPUT=0,
	VIDEO_DUAL_LINK_INPUT=1,
	VIDEO_DUAL_LINK_OUTPUT_SIGNAL_FORMAT_TYPE=2,	//EDualLinkSignalFormatType
	VIDEO_DUAL_LINK_INPUT_SIGNAL_FORMAT_TYPE=3,	//EDualLinkSignalFormatType
	VIDEO_OUTPUT_SIGNAL_COLOR_SPACE=4,			//EConnectorSignalColorSpace
	VIDEO_INPUT_SIGNAL_COLOR_SPACE=5,				//EConnectorSignalColorSpace
	VIDEO_MEMORY_FORMAT=6, 						//EMemoryFormat
	VIDEO_MODE=7,									//EVideoMode
	VIDEO_UPDATE_TYPE=8,							//EUpdateMethod
	VIDEO_ENGINE=9,
	VIDEO_IMAGE_ORIENTATION=10,
	VIDEO_USER_DEFINED_COLOR_MATRIX=11,
	VIDEO_PREDEFINED_COLOR_MATRIX=12,				//EPreDefinedColorSpaceMatrix
	VIDEO_RGB_DATA_RANGE=13,						//ERGBDataRange
	VIDEO_KEY_OVER_BLACK=14,
	VIDEO_KEY_OVER_INPUT_SIGNAL=15,
	VIDEO_SET_DOWN_CONVERTER_VIDEO_MODE=16,		//EHD_XCONNECTOR_MODE
	VIDEO_LETTER_BOX=17,
	VIDEO_PILLOR_BOX_LEFT=18,
	VIDEO_PILLOR_BOX_RIGHT=19,
	VIDEO_PILLOR_BOX_TOP=20,
	VIDEO_PILLOR_BOX_BOTTOM=21,
	VIDEO_SAFE_PICTURE=22,
	VIDEO_SAFE_TITLE=23,
	VIDEO_INPUT_SIGNAL_VIDEO_MODE=24,
	VIDEO_COLOR_MATRIX_MODE=25,
	VIDEO_OUTPUT_MAIN_LUT=26,
	VIDEO_OUTPUT_AUX_LUT=27,
	VIDEO_LTC=28,
	VIDEO_GPIO=29,	
	VIDEO_PLAYBACK_FIFO_STATUS=30,
	RS422_RX_BUFFER_LENGTH=31,
	RS422_RX_BUFFER_FLUSH=32,
	VIDEO_INPUT_UPDATE_TYPE=33,
	VIDEO_INPUT_MEMORY_FORMAT=34,
	VIDEO_GENLOCK_SIGNAL=35,
	AUDIO_OUTPUT_PROP=36,
	AUDIO_CHANNEL_ROUTING=AUDIO_OUTPUT_PROP,
	AUDIO_INPUT_PROP=37,
	VIDEO_ENABLE_LETTERBOX=38,
	VIDEO_DUALLINK_OUTPUT_INVERT_KEY_COLOR=39,
	VIDEO_DUALLINK_OUTPUT_DEFAULT_KEY_COLOR=40,
	VIDEO_BLACKGENERATOR=41,
	VIDEO_INPUTFRAMESTORE_IMAGE_ORIENTATION=42,
	VIDEO_INPUT_SOURCE_SELECTION=43,
	DEFAULT_VIDEO_OUTPUT_CHANNEL=44,
	DEFAULT_VIDEO_INPUT_CHANNEL=45,
	VIDEO_REFERENCE_SIGNAL_TIMING=46,
	EMBEDEDDED_AUDIO_OUTPUT=47,
	EMBEDDED_AUDIO_OUTPUT=EMBEDEDDED_AUDIO_OUTPUT,
	VIDEO_PLAYBACK_FIFO_FREE_STATUS=48,
	VIDEO_IMAGE_WIDTH=49,
	VIDEO_IMAGE_HEIGHT=50,
	VIDEO_SCALER_MODE=51,
	AVAIL_AUDIO_INPUT_SAMPLE_COUNT=52,
	VIDEO_PLAYBACK_FIFO_ENGINE_STATUS=53,
	VIDEO_CAPTURE_FIFO_ENGINE_STATUS=54,
	VIDEO_2K_1556_PANSCAN=55,
	VIDEO_OUTPUT_ENGINE=56,
	VIDEO_INPUT_ENGINE=57,
	BYPASS_RELAY_A_ENABLE=58,
	BYPASS_RELAY_B_ENABLE=59,
	VIDEO_PREMULTIPLIER=60,
	VIDEO_PLAYBACK_START_TRIGGER_POINT=61,
	GENLOCK_TIMING=62,
	VIDEO_IMAGE_PITCH=63,
	VIDEO_IMAGE_OFFSET=64,
	VIDEO_INPUT_IMAGE_WIDTH=65,
	VIDEO_INPUT_IMAGE_HEIGHT=66,
	VIDEO_INPUT_IMAGE_PITCH=67,
	VIDEO_INPUT_IMAGE_OFFSET=68,
	TIMECODE_RP188=69,
	BOARD_TEMPERATURE=70,
	MR2_ROUTING=71,
	SAVEAS_POWERUP_SETTINGS=72,
	VIDEO_CARDPROPERTY_INVALID=100
}EBlueCardProperty;


typedef enum _EAnalogConnectorSignalType
{
	ANALOG_OUTPUTSIGNAL_CVBS_Y_C=1,
	ANALOG_OUTPUTSIGNAL_COMPONENT=2,
	ANALOG_OUTPUTSIGNAL_RGB=4
}EAnalogConnectorSignalType;


typedef enum _EAnalogInputConnectorType 
{
/* Composite input */
	ANALOG_VIDEO_INPUT_CVBS_AIN1=0x00, //only available on Mini COAX 
	ANALOG_VIDEO_INPUT_CVBS_AIN2=0x01, //available on both Mini COAX and Mini DIN
	ANALOG_VIDEO_INPUT_CVBS_AIN3=0x02, //available on both Mini COAX and Mini DIN
	ANALOG_VIDEO_INPUT_CVBS_AIN4=0x03, //only available on Mini COAX 
	ANALOG_VIDEO_INPUT_CVBS_AIN5=0x04, //only available on Mini COAX 
	ANALOG_VIDEO_INPUT_CVBS_AIN6=0x05, //available on both Mini COAX and Mini DIN 

/*svideo input*/
//Y_C is a synonym for svideo
	ANALOG_VIDEO_INPUT_Y_C_AIN1_AIN4=0x06, //only available on Mini COAX
	ANALOG_VIDEO_INPUT_Y_C_AIN2_AIN5=0x07, //only available on Mini COAX
	ANALOG_VIDEO_INPUT_Y_C_AIN3_AIN6=0x08, //available on both Mini COAX and Mini DIN

/*YUV input*/
	ANALOG_VIDEO_INPUT_YUV_AIN1_AIN4_AIN5=0x09, //only available on Mini COAX
	ANALOG_VIDEO_INPUT_YUV_AIN2_AIN3_AIN6=0x0a, //available on both Mini COAX and Mini DIN
	ANALOG_VIDEO_INPUT_USE_SDI_A=0x6F,	
	ANALOG_VIDEO_INPUT_USE_SDI=0x7F,
	GENERIC_ANALOG_VIDEO_SOURCE=0x8F,
	ANALOG_VIDEO_INPUT_USE_SDI_B=ANALOG_VIDEO_INPUT_USE_SDI
}EAnalogInputConnectorType;


typedef enum {
	ANALOG_VIDEO_INPUT_CONNECTOR,//EAnalogInputConnectorType
	ANALOG_VIDEO_INPUT_PED,
	ANALOG_VIDEO_INPUT_BRIGHTNESS,
	ANALOG_VIDEO_INPUT_HUE,
	ANALOG_VIDEO_INPUT_LUMA_GAIN,
	ANALOG_VIDEO_INPUT_CHROMA_GAIN,
	ANALOG_VIDEO_INPUT_AUTO_GAIN,
	ANALOG_VIDEO_INPUT_LOAD_DEFAULT_SETTING,	
	ANALOG_VIDEO_OUTPUT_PED,
	ANALOG_VIDEO_OUTPUT_BRIGHTNESS,
	ANALOG_VIDEO_OUTPUT_HUE,
	ANALOG_VIDEO_OUTPUT_LUMA_GAIN,
	ANALOG_VIDEO_OUTPUT_CHROMA_GAIN,	
	ANALOG_VIDEO_OUTPUT_SHARPNESS,
	ANALOG_VIDEO_OUTPUT_AUTO_GAIN,
	ANALOG_VIDEO_OUTPUT_LOAD_DEFAULT_SETTING,
	ANALOG_VIDEO_OUTPUT_SIGNAL_TYPE,//_EAnalogConnectorSignalType
	ANALOG_LOAD_BLUEFISH_DEFAULT_SETTING,
	ANALOG_SET_AS_POWERUP_SETTINGS,
	ANALOG_LOAD_POWERUP_SETTINGS,
	ANALOG_CONNECTOR_STATUS
} AnalogCard_Property;

typedef struct {
	BLUE_INT32 inputConnector;			//ANALOG_VIDEO_INPUT_CONNECTOR, EAnalogInputConnectorType
	BLUE_INT32 inputPed;				//ANALOG_VIDEO_INPUT_PED,
	BLUE_INT32 inputBrightness;		//ANALOG_VIDEO_INPUT_BRIGHTNESS,
	BLUE_INT32 inputHue;				//ANALOG_VIDEO_INPUT_HUE,
	BLUE_INT32 inputLumaGain;			//ANALOG_VIDEO_INPUT_LUMA_GAIN,
	BLUE_INT32 inputChromaGain;		//ANALOG_VIDEO_INPUT_CHROMA_GAIN,
	BLUE_INT32 inputAutoGain;			//ANALOG_VIDEO_INPUT_AUTO_GAIN,
	BLUE_INT32 outputPed;				//ANALOG_VIDEO_OUTPUT_PED,
	BLUE_INT32 outputBrightness;		//ANALOG_VIDEO_OUTPUT_BRIGHTNESS,
	BLUE_INT32 outputHue;				//ANALOG_VIDEO_OUTPUT_HUE,
	BLUE_INT32 outputYGain;			//ANALOG_VIDEO_OUTPUT_Y_GAIN,
	BLUE_INT32 outputUGain;			//ANALOG_VIDEO_OUTPUT_U_GAIN,
	BLUE_INT32 outputVGain;			//ANALOG_VIDEO_OUTPUT_V_GAIN,
	BLUE_INT32 outputSharpness;		//ANALOG_VIDEO_OUTPUT_SHARPNESS,
	BLUE_INT32 outputAutoGain;			//ANALOG_VIDEO_OUTPUT_AUTO_GAIN,
	BLUE_INT32 outputSignalTypes;		//EAnalogConnectorSignalType
}AnalogCardState;

//----------------------------------------------------------------------------------------------------
// Used to determine how video interrupts are handled, used in IOCTL_REDDEVIL_VIDEO_ENGINE
typedef enum _EEngineMode
{
	VIDEO_ENGINE_FRAMESTORE=0,	// framestore engine
	VIDEO_ENGINE_PLAYBACK=1,	// Playback engine
	VIDEO_ENGINE_CAPTURE=2,		// Capture engine
	VIDEO_ENGINE_PAGEFLIP=3,	// page flipper a mod of CHU viewport 
	VIDEO_ENGINE_DUPLEX=4,		// Full Duplex video
	VIDEO_ENGINE_INVALID
} EEngineMode;


typedef enum _EBlueEmbAudioOutput
{
	blue_emb_audio_enable=0x1,	// Switches off/on  the whole HANC output from connecotrs associated with the channel
	blue_auto_aes_to_emb_audio_encoder=0x2, //control whether the auto aes to emb thread should be running or not.
	blue_emb_audio_group1_enable=0x4,
	blue_emb_audio_group2_enable=0x8,
	blue_emb_audio_group3_enable=0x10,
	blue_emb_audio_group4_enable=0x20,
	blue_enable_hanc_timestamp_pkt = 0x40
}EBlueEmbAudioOutput;


// Buffer Target enumerations
typedef enum _EBufferTarget
{
	BUFFER_TARGET_VIDEO=0,		// Generic R/W DMA
	BUFFER_TARGET_AUDIO,		// Special processing required for audio
	BUFFER_TARGET_VIDEO_8BIT,	// Special R/W DMA utilising 8 bit aperture
	BUFFER_TARGET_VIDEO_HALF,	// Special R/W DMA every second line (currently unused)
	BUFFER_TARGET_VIDEO_OUT,	// Updates video out register on DMA completion for write 
	BUFFER_TARGET_INVALID
} EBufferTarget;

#define	BUFFER_TYPE_VIDEO		(0)
#define	BUFFER_TYPE_AUDIO		(1)
#define	BUFFER_TYPE_VIDEO_8BIT	(2)	// use this when assigning a buffer to indicate DMA from aperture!
#define	BUFFER_TYPE_VIDEO_OUT	(3)	// On DMA start set video output address to DMA target
#define	BUFFER_TYPE_VIDEO_HALF	(4)	// DMA every second line...

// Buffer identifiers
#define	BUFFER_ID_AUDIO_IN		(0)
#define	BUFFER_ID_AUDIO_OUT		(1)
#define	BUFFER_ID_VIDEO0		(2)
#define	BUFFER_ID_VIDEO1		(3)
#define	BUFFER_ID_VIDEO2		(4)
#define	BUFFER_ID_VIDEO3		(5)

//#define	BUFFER_ID_USER_BASE		(6)



#define VIDEO_BORDER_TOP		(0x10000000)
#define VIDEO_BORDER_BOTTOM		(0x20000000)
#define VIDEO_BORDER_LEFT		(0x40000000)
#define VIDEO_BORDER_RIGHT		(0x80000000)

typedef struct _AnalogCardPropStruct
{
	BLUE_UINT32 VideoChannel;
	BLUE_INT32  prop;
	BLUE_INT32  value;
	BLUE_INT32  minValue;
	BLUE_INT32  maxValue;
	BLUE_INT32  bReadFlag;
}AnalogCardPropStruct;

typedef enum _EConnectorSignalFormatType
{
	Signal_Type_4444 =1,
	Signal_Type_4224 =0,
	Signal_Type_422=2
}EConnectorSignalFormatType;

typedef enum _EDMADirection
{
	DMA_WRITE=0,
	DMA_READ=1,
	DMA_INVALID=2
}EDMADirection;			


typedef enum _MatrixColType
{
	COL_BLUE_PB=0,
	COL_RED_PR=1,
	COL_GREEN_Y=2
}MatrixColType;


struct VideoFeature_struct 
{
	BLUE_UINT32  Type;                   // Bluefish card type
	BLUE_UINT32  CardSubType;		
	BLUE_UINT32  Bus;                    // Which PIC bus (bridge) it is on
	BLUE_UINT32  Slot;                   // Which slot card is plugged into
	BLUE_UINT32  Feature;                //  Look at  the _EBlueFishCardFeatures  definition to know what each bit mean
};

// Bits defining supported features that can be used with VideoFeature_struct
#define VIDEO_CAPS_INPUT_SDI					(0x00000001)    // Capable of input of SDI Video
#define VIDEO_CAPS_OUTPUT_SDI					(0x00000002)    // Capable of output of SDI Video
#define VIDEO_CAPS_INPUT_COMP					(0x00000004)    // Capable of capturing Composite Video input
#define VIDEO_CAPS_OUTPUT_COMP					(0x00000008)    // Capable of capturing Composite Video output

#define VIDEO_CAPS_INPUT_YUV					(0x00000010)    // Capable of capturing Component Video input
#define VIDEO_CAPS_OUTPUT_YUV					(0x00000020)    // Capable of capturing Component Video output
#define VIDEO_CAPS_INPUT_SVIDEO					(0x00000040)    // Capable of capturing SVideo input
#define VIDEO_CAPS_OUTPUT_SVIDEO				(0x00000080)    // Capable of capturing SVideo output

#define VIDEO_CAPS_GENLOCK						(0x00000100)    // Able to adjust Vert & Horiz timing
#define VIDEO_CAPS_VERTICAL_FLIP				(0x00000200)    // Able to flip rasterisation
#define VIDEO_CAPS_KEY_OUTPUT					(0x00000400)    // Video keying output capable
#define VIDEO_CAPS_4444_OUTPUT					(0x00000800)    // Capable of outputting 4444 (dual link)

#define VIDEO_CAPS_DUALLINK_INPUT				(0x00001000)	// Dual Link input  
#define VIDEO_CAPS_INTERNAL_KEYER				(0x00002000)	// Has got an internal Keyer
#define VIDEO_CAPS_RGB_COLORSPACE_SDI_CONN		(0x00004000)	// Support RGB colorspace  in on an  SDI connector 
#define VIDEO_CAPS_HAS_PILLOR_BOX				(0x00008000)	// Has got support for pillor box

#define VIDEO_CAPS_OUTPUT_RGB 					(0x00010000)    // Has Analog RGB output connector 
#define VIDEO_CAPS_SCALED_RGB 					(0x00020000)    // Can scale RGB colour space
#define AUDIO_CAPS_PLAYBACK 					(0x00040000)	// Has got audio output
#define AUDIO_CAPS_CAPTURE 						(0x00080000)

#define VIDEO_CAPS_DOWNCONVERTER				(0x00100000)
#define VIDEO_CAPS_DUALOUTPUT_422_IND_STREAM			(0x00200000)	// Specifies whether the card supports Dual Indepenedent 422 output streams
#define VIDEO_CAPS_DUALINPUT_422_IND_STREAM			(0x00400000)	// Specifies whether the card supports Dual Indepenedent 422 input streams

#define VIDEO_CAPS_VBI_OUTPUT					(0x00800000)
#define VIDEO_CAPS_VBI_INPUT					(0x01000000)

#define VIDEO_CAPS_HANC_OUTPUT					(0x02000000)
#define VIDEO_CAPS_HANC_INPUT					(0x04000000)
/*
#define VIDEO_CAPS_DUALOUTPUT_422_IND_STREAM	(0x00100000)	// Specifies whether the card supports Dual Indepenedent 422 output streams
#define VIDEO_CAPS_DUALINPUT_422_IND_STREAM		(0x00200000)	// Specifies whether the card supports Dual Indepenedent 422 input streams

#define VIDEO_CAPS_VBI_OUTPUT					(0x00400000)
#define VIDEO_CAPS_VBI_INPUT					(0x00800000)

#define VIDEO_CAPS_HANC_OUTPUT					(0x01000000)
#define VIDEO_CAPS_HANC_INPUT					(0x02000000)
*/

#define BLUE_CARD_BUFFER_TYPE_OFFSET		(12)
#define BLUE_DMA_DATA_TYPE_OFFSET			(16)
#define BLUE_DMA_FLAGS_OFFSET				(20)
#define GetDMACardBufferId(value)			(value & 0xFFF)
#define GetCardBufferType(value)			((value & 0xF000) >> BLUE_CARD_BUFFER_TYPE_OFFSET)
#define GetDMADataType(value)				((value & 0xF0000) >> BLUE_DMA_DATA_TYPE_OFFSET)
#define GetDMAFlags(value)					((value & 0xF00000) >> (BLUE_DMA_FLAGS_OFFSET))

#define BlueImage_VBI_DMABuffer(BufferId,DataType)				( (((ULONG)DataType&0xF)<<(ULONG)BLUE_DMA_DATA_TYPE_OFFSET)| \
																( BLUE_CARDBUFFER_IMAGE_VBI<<(ULONG)BLUE_CARD_BUFFER_TYPE_OFFSET) | \
																( ((ULONG)BufferId&0xFFF)) |0)

#define BlueImage_DMABuffer(BufferId,DataType)					( (((ULONG)DataType&0xF)<<(ULONG)BLUE_DMA_DATA_TYPE_OFFSET)| \
																( BLUE_CARDBUFFER_IMAGE<<(ULONG)BLUE_CARD_BUFFER_TYPE_OFFSET) | \
																( ((ULONG)BufferId&0xFFF)) |0)

#define BlueImage_VBI_HANC_DMABuffer(BufferId,DataType)			( (((ULONG)DataType&0xF)<<(ULONG)BLUE_DMA_DATA_TYPE_OFFSET)| \
																( BLUE_CARDBUFFER_IMAGE_VBI_HANC<<(ULONG)BLUE_CARD_BUFFER_TYPE_OFFSET) | \
																( ((ULONG)BufferId&0xFFF)) |0)

#define BlueImage_HANC_DMABuffer(BufferId,DataType)				( (((ULONG)DataType&0xF)<<(ULONG)BLUE_DMA_DATA_TYPE_OFFSET)| \
																( BLUE_CARDBUFFER_IMAGE_HANC<<(ULONG)BLUE_CARD_BUFFER_TYPE_OFFSET) | \
																( ((ULONG)BufferId&0xFFF)) |0)


#define BlueBuffer_Image_VBI(BufferId)				(((BLUE_CARDBUFFER_IMAGE_VBI)<<BLUE_CARD_BUFFER_TYPE_OFFSET)|((BufferId&0xFFF))|0)
#define BlueBuffer_Image(BufferId)					(((BLUE_CARDBUFFER_IMAGE)<<BLUE_CARD_BUFFER_TYPE_OFFSET)|((BufferId&0xFFF))|0)
#define BlueBuffer_Image_VBI_HANC(BufferId)			(((BLUE_CARDBUFFER_IMAGE_VBI_HANC)<<BLUE_CARD_BUFFER_TYPE_OFFSET)|((BufferId&0xFFF))|0)
#define BlueBuffer_Image_HANC(BufferId)				(((BLUE_CARDBUFFER_IMAGE_HANC)<<BLUE_CARD_BUFFER_TYPE_OFFSET)|((BufferId&0xFFF))|0)
#define BlueBuffer_HANC(BufferId)					(((BLUE_CARDBUFFER_HANC)<<BLUE_CARD_BUFFER_TYPE_OFFSET)|((BufferId&0xFFF))|0)

#define BYPASS_RELAY_A								(0x00000001) // enable bypass relay channel a when loading driver , only used in linux 
#define BYPASS_RELAY_B								(0x00000002) // enable bypass relay channel a when loading driver , only used in linux 
typedef enum _EDMACardBufferType
{
	BLUE_CARDBUFFER_IMAGE=0,
	BLUE_CARDBUFFER_IMAGE_VBI_HANC=1,
	BLUE_CARDBUFFER_IMAGE_VBI=2,
	BLUE_CARDBUFFER_AUDIO_OUT=3,
	BLUE_CARDBUFFER_AUDIO_IN=4,
	BLUE_CARDBUFFER_HANC=5,
	BLUE_CARDBUFFER_IMAGE_HANC=6,
	BLUE_CARDBUFFER_INVALID=6
}EDMACardBufferType;

typedef enum _EDMADataType
{
	BLUE_DATA_FRAME=0,
	BLUE_DATA_IMAGE=0,
	BLUE_DATA_FIELD1=1,
	BLUE_DATA_FIELD2=2,
	BLUE_DATA_VBI=3,
	BLUE_DATA_HANC=4,
	BLUE_DATA_AUDIO_IN=5,
	BLUE_DATA_AUDIO_OUT=6,
	BLUE_DATA_FRAME_RDOM=7,
	BLUE_DMADATA_INVALID=8
}EDMADataType;

typedef struct _AUXILLARY_VIDEO_INFO
{
	BLUE_UINT32  video_channel_id;
	BLUE_UINT32  lUniqueId;
	BLUE_UINT32  lInfoType;
	BLUE_UINT32  lMemFmt;
	BLUE_UINT32  lGpio;
	BLUE_UINT64  lLTC;
}Auxillary_Video_Info;


typedef enum _EBlueVideoAuxInfoType
{
	BLUE_VIDEO_AUX_MEMFMT_CHANGE=1,
	BLUE_VIDEO_AUX_UPDATE_LTC=2,
	BLUE_VIDEO_AUX_UPDATE_GPIO=4,
}EBlueVideoAuxInfoType;
// Max of 4 bits 

#define GET_ANALOG_AUDIO_LEFT_ROUTINGCHANNEL(value)				(value&0xFF)
#define GET_ANALOG_AUDIO_RIGHT_ROUTINGCHANNEL(value)			((value&0xFF00)>>8)
#define SET_ANALOG_AUDIO_ROUTINGCHANNEL(left,right)				(((right & 0xFF)<<8)|(left & 0xFF))
#define SET_AUDIO_OUTPUT_ROUTINGCHANNEL(output_type,src_channel_id,_output_channel_id) ((1<<31)|((output_type&3)<<29)|((src_channel_id & 0x7F)<<16)|((_output_channel_id &0x3f)<<23))
#define GET_AUDIO_OUTPUT_SRC_CHANNEL_ROUTING(value)				((value>>16) & 0x7F)
#define GET_AUDIO_OUTPUT_CHANNEL_ROUTING(value)					((value>>23) & 0x3F)
#define GET_AUDIO_OUTPUT_TYPE_ROUTING(value)					((value & 0x60000000)>>29)

#define AUDIO_INPUT_SOURCE_SELECT_FLAG							(1<<16)	
#define AUDIO_INPUT_SOURCE_SELECT(SynchCount,AudioInputSource)	(AUDIO_INPUT_SOURCE_SELECT_FLAG|(SynchCount)|(AudioInputSource<<17))

struct blue_video_connection_routing_struct
{
	BLUE_UINT32 video_channel;
	BLUE_UINT32 duallink_flag;
	BLUE_UINT32 link1_connector;
	BLUE_UINT32 link2_connector;
};

#pragma pack(push, video_sync_struct, 1)
typedef struct _blue_video_sync_struct
{
	BLUE_UINT32	sync_wait_type;// field or frame
	BLUE_UINT32	video_channel; // which video channel interrupt should the interrupt wait for 
	BLUE_UINT32	timeout_video_msc;	//if the current video msc is equal to this one insert it into the queue.
	BLUE_UINT32	video_msc;		//current video msc
	BLUE_UINT8  pad[32];
}blue_video_sync_struct;
#pragma pack(pop,video_sync_struct)


typedef enum _EBlueLUTType
{
	BLUE_MAIN_LUT_B_Pb=0,
	BLUE_MAIN_LUT_G_Y=1,
	BLUE_MAIN_LUT_R_Pr=2,
	BLUE_AUX_LUT_B_Pb=3,
	BLUE_AUX_LUT_G_Y=4,
	BLUE_AUX_LUT_R_Pr=5,
}EBlueLUTType;

#pragma pack(push, video_frame, 1)
struct blue_videoframe_info
{
	BLUE_UINT64 ltcTimeCode;
	unsigned long videochannel;
	unsigned long BufferId;
	unsigned long Count;
	unsigned long DroppedFrameCount;
};

struct blue_videoframe_info_ex
{
	BLUE_UINT64 ltcTimeCode;
	unsigned long videochannel;
	long BufferId;
	unsigned long Count;
	unsigned long DroppedFrameCount;
	unsigned long nFrameTimeStamp;
	unsigned long nVideoSignalType;
	unsigned char pad[32];
};

struct blue_1d_lookup_table_struct
{
	BLUE_UINT32 nVideoChannel;
	BLUE_UINT32  nLUTId;
	BLUE_UINT16 * pLUTData;
	BLUE_UINT32  nLUTElementCount;	
	BLUE_UINT8	pad[256];
};
#pragma pack(pop, video_frame)

#pragma pack(push, blue_dma_request, 1)
struct blue_dma_request_struct
{
	unsigned char * pBuffer;
	BLUE_UINT32 video_channel;
	BLUE_UINT32	BufferId;
	unsigned int BufferDataType;
	unsigned int FrameType;
	unsigned int BufferSize;
	unsigned int Offset;
	unsigned long	BytesTransferred;
	unsigned char pad[64];
};
typedef struct _SerialPort_struct
{
	unsigned char 	Buffer[64];
	unsigned int	nBufLength;
	unsigned int	nSerialPortId;
	unsigned int	bFlag; // SerialPort_struct_flags 
	unsigned short	sTimeOut; 
}SerialPort_struct;

#pragma pack(pop, blue_dma_request)

typedef enum _blue_output_hanc_ioctl_enum
{
	blue_get_output_hanc_buffer=0,
	blue_put_output_hanc_buffer=1,
	blue_get_valid_silent_hanc_data_status=3,
	blue_set_valid_silent_hanc_data_status=4,
	blue_start_output_fifo=5,
	blue_stop_output_fifo=6,
	blue_init_output_fifo=7,
	blue_get_queues_info=8,
	blue_get_output_fifo_info=blue_get_queues_info,
	blue_get_output_fifo_status=9,

}blue_output_hanc_ioctl_enum;

typedef enum _blue_input_hanc_ioctl_enum
{
	blue_get_input_hanc_buffer=0,
	blue_start_input_fifo=3,
	blue_stop_input_fifo=4,
	blue_init_input_fifo=5,
	blue_playthru_input_fifo=6,
	blue_release_input_hanc_buffer=7,
	blue_map_input_hanc_buffer=8,
	blue_unmap_input_hanc_buffer=9,
	blue_get_info_input_hanc_fifo=10,
	blue_get_input_rp188=11,
	blue_get_input_fifo_status=12,
}blue_input_hanc_ioctl_enum;


#define HANC_PLAYBACK_INIT				(0x00000001)
#define HANC_PLAYBACK_START				(0x00000002)
#define HANC_PLAYBACK_STOP				(0x00000004)

#define HANC_CAPTURE_INIT				(0x00000010)
#define HANC_CAPTURE_START				(0x00000020)
#define HANC_CAPTURE_STOP				(0x00000040)
#define HANC_CAPTURE_PLAYTHRU			(0x00000080)


typedef enum _EOracFPGAConfigCMD
{
	ORAC_FPGA_CONFIG_CMD_ERASE_SECTOR=0,
	ORAC_FPGA_CONFIG_CMD_UNLOCK_SECTOR=1,
	ORAC_FPGA_CONFIG_CMD_WRITE_DATA=2,
	ORAC_FPGA_CONFIG_CMD_STATUS=3,
	ORAC_FPGA_CONFIG_CMD_READMODE=4,
	ORAC_FPGA_CONFIG_RAW_WRITE=5,
	ORAC_FPGA_CONFIG_RAW_READ=6,
	ORAC_FPGA_CONFIG_CMD_READ_DATA=7
}EOracFPGAConfigCMD;


#define ANALOG_CHANNEL_0 MONO_CHANNEL_9
#define ANALOG_CHANNEL_1 MONO_CHANNEL_10

/*Assumes that the data is in stereo pairs not individual samples*/
#define STEREO_PAIR_1	(MONO_CHANNEL_1|MONO_CHANNEL_2) /* Mono Channel 1 & Mono channel 2* together*/
#define STEREO_PAIR_2	(MONO_CHANNEL_3|MONO_CHANNEL_4) /* Mono Channel 3 & Mono Channel 4* together*/
#define STEREO_PAIR_3	(MONO_CHANNEL_5|MONO_CHANNEL_6) /* Mono Channel 5 & Mono Channel 6* together*/
#define STEREO_PAIR_4	(MONO_CHANNEL_7|MONO_CHANNEL_8) /* Mono Channel 7 & Mono Channel 8* together*/

#define ANALOG_AUDIO_PAIR (ANALOG_CHANNEL_0|ANALOG_CHANNEL_1)

#define BLUE_LITTLE_ENDIAN	0
#define BLUE_BIG_ENDIAN		1

#define GREED_SILENT_HANC_BUFFER1	250
#define GREED_SILENT_HANC_BUFFER2	251



typedef enum _EEpochRoutingElements
{
	EPOCH_SRC_DEST_SCALER_0=0x1,
	EPOCH_SRC_DEST_SCALER_1,
	EPOCH_SRC_DEST_SCALER_2,
	EPOCH_SRC_DEST_SCALER_3,

	EPOCH_SRC_SDI_INPUT_A,
	EPOCH_SRC_SDI_INPUT_B,
	EPOCH_SRC_SDI_INPUT_C,
	EPOCH_SRC_SDI_INPUT_D,

	EPOCH_DEST_SDI_OUTPUT_A,
	EPOCH_DEST_SDI_OUTPUT_B,
	EPOCH_DEST_SDI_OUTPUT_C,
	EPOCH_DEST_SDI_OUTPUT_D,

	EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHA,
	EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHB,

	EPOCH_DEST_INPUT_MEM_INTERFACE_CHA,
	EPOCH_DEST_INPUT_MEM_INTERFACE_CHB,

}EEpochRoutingElements;



#define VPEnableFieldCountTrigger		((BLUE_UINT64)1<<63)
#define VPTriggerGetFieldCount(value)	((BLUE_UINT64)value & 0xFFFFFFFF)

typedef enum _EBlueScalerFilterType
{
	BlueScalerHorizontalYFilter=1,
	BlueScalerHorizontalCFilter=2,
	BlueScalerVerticalYFilter=3,
	BlueScalerVerticalCFilter=4,
}EBlueScalerFilterType;



#define SET_EPOCH_SCALER_MODE(scaler_id,video_mode) ((scaler_id <<16)|video_mode)
#define GET_EPOCH_SCALER_MODE(value)				(value&0xFFFF)
#define GET_EPOCH_SCALER_ID(value)					((value&0xFFFF0000)>>16)

// use these macros for retreiving the temp and fan speed.
// on epoch range of cards.
#define EPOCH_CORE_TEMP(value)					(value & 0xFFFF)
#define EPOCH_BOARD_TEMP(value)					((value>>16) & 0xFF)
#define EPOCH_FAN_SPEED(value)					((value>>24) & 0xFF)

// use these macro for doing the MR2 routing 
// on epoch range of cards.
#define EPOCH_SET_ROUTING(routing_src,routing_dest,data_link_type) ((routing_src & 0xFF) | ((routing_dest & 0xFF)<<8) | ((data_link_type&0xFF)<<16))
#define EPOCH_ROUTING_GET_SRC_DATA(value)		(value & 0xFF)
#define EPOCH_ROUTING_GET_DEST_DATA(value)		((value>>8) & 0xFF)
#define EPOCH_ROUTING_GET_LINK_TYPE_DATA(value)	((value>>16) & 0xFF)