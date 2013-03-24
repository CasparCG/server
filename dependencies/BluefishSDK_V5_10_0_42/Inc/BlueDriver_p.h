/*
 $Id: BlueDriver_p.h,v 1.62.2.16 2011/10/26 05:33:18 tim Exp $
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

//#define ORAC_FILMPOST_FIRMWARE_PRODUCTID	(0x1)
//#define ORAC_BROADCAST_FIRMWARE_PRODUCTID	(0x2)
//#define ORAC_ASI_FIRMWARE_PRODUCTID			(0x3)
//#define ORAC_4SDIINPUT_FIRMWARE_PRODUCTID	(0x4)
//#define ORAC_4SDIOUTPUT_FIRMWARE_PRODUCTID	(0x5)



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
	VID_FMT_HSDL_1500,		/**< 2048 x 1556 15			Segment Frame */
	VID_FMT_720P_5000,		/**< 1280 x 720  50			Progressive */
	VID_FMT_720P_2398,		/**< 1280 x 720  24/1.001	Progressive */
	VID_FMT_720P_2400,		/**< 1280 x 720  24	Progressive */
	VID_FMT_2048_1080PSF_2397=19,	/**< 2048 x 1080 24/1.001	Segment Frame */
	VID_FMT_2048_1080PSF_2400=20,	/**< 2048 x 1080 24	Segment Frame */
	VID_FMT_2048_1080P_2397=21,	/**< 2048 x 1080 24/1.001	progressive */ 
	VID_FMT_2048_1080P_2400=22,	/**< 2048 x 1080 24	progressive  */
	VID_FMT_1080PSF_2500=23,
	VID_FMT_1080PSF_2997=24,
	VID_FMT_1080PSF_3000=25,
	VID_FMT_1080P_5000=26,
	VID_FMT_1080P_5994=27,
	VID_FMT_1080P_6000=28,
	VID_FMT_720P_2500=29,
	VID_FMT_720P_2997=30,
	VID_FMT_720P_3000=31,
	VID_FMT_DVB_ASI=32,
	VID_FMT_2048_1080PSF_2500=33,
	VID_FMT_2048_1080PSF_2997=34,
	VID_FMT_2048_1080PSF_3000=35,
	VID_FMT_2048_1080P_2500=36,
	VID_FMT_2048_1080P_2997=37,
	VID_FMT_2048_1080P_3000=38,
	VID_FMT_2048_1080P_5000=39,
	VID_FMT_2048_1080P_5994=40,
	VID_FMT_2048_1080P_6000=41,
	VID_FMT_INVALID=42
} EVideoMode;

/**
@brief Use these enumerators to set the pixel format 
		that should be used by the video input and output 
		framestores.
*/
typedef enum _EMemoryFormat
{
	MEM_FMT_ARGB=0, /**< ARGB 4:4:4:4 */
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
	MEM_FMT_BGR_16_16_16=10,
	MEM_FMT_BGRA_16_16_16_16=11,
	MEM_FMT_VUYA_4444=12,
	MEM_FMT_V216=13,
	MEM_FMT_Y210=14,
	MEM_FMT_Y216=15,
	MEM_FMT_INVALID=16
} EMemoryFormat;

/**
@brief Used to control the video update type, 
		whether the card should capture/playback a
		video frame or field.
*/
typedef enum _EUpdateMethod
{
	UPD_FMT_FIELD=0,
	UPD_FMT_FRAME,
	UPD_FMT_FRAME_DISPLAY_FIELD1,
	UPD_FMT_FRAME_DISPLAY_FIELD2,
	UPD_FMT_INVALID,
	UPD_FMT_FLAG_RETURN_CURRENT_UNIQUEID=0x80000000,/**< if this flag is used on epoch cards, function would 
													return the unique id of the current frame as the return value.*/
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
	CRD_BLUE_CREATE_3D = CRD_BLUE_CREATE_2K,
	CRD_BLUE_CREATE_3D_ULTRA = CRD_BLUE_CREATE_2K_ULTRA,
	CRD_BLUE_SUPER_NOVA,
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

enum EEpochFirmwareProductID
{
	ORAC_FILMPOST_FIRMWARE_PRODUCTID=(0x1),				//Epoch/Create, standard firmware
	ORAC_BROADCAST_FIRMWARE_PRODUCTID=(0x2),			//Epoch
	ORAC_ASI_FIRMWARE_PRODUCTID=(0x3),					//Epoch
	ORAC_4SDIINPUT_FIRMWARE_PRODUCTID=(0x4),			//SuperNova
	ORAC_4SDIOUTPUT_FIRMWARE_PRODUCTID=(0x5),			//SuperNova
	ORAC_2SDIINPUT_2SDIOUTPUT_FIRMWARE_PRODUCTID=(0x6),	//SuperNova
};

/**< @brief Use this enumerator to select the audio channels that should be captured  or played back.
*/
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
/**<
Use this enumeraotor to set the type of pcm audio data.
*/
typedef enum
{
	AUDIO_CHANNEL_LOOPING_OFF		= 0x00000000, /**< deprecated not used any more */
	AUDIO_CHANNEL_LOOPING			= 0x00000001,/**< deprecated not used any more */
	AUDIO_CHANNEL_LITTLEENDIAN		= 0x00000000, /**< if the audio data is little endian this flag must be set*/
	AUDIO_CHANNEL_BIGENDIAN			= 0x00000002,/**< if the audio data is big  endian this flag must be set*/
	AUDIO_CHANNEL_OFFSET_IN_BYTES	= 0x00000004,/**< deprecated not used any more */
	AUDIO_CHANNEL_16BIT				= 0x00000008, /**< if the audio channel bit depth is 16 bits this flag must be set*/
	AUDIO_CHANNEL_BLIP_PENDING		= 0x00000010,/**< deprecated not used any more */
	AUDIO_CHANNEL_BLIP_COMPLETE		= 0x00000020,/**< deprecated not used any more */
	AUDIO_CHANNEL_SELECT_CHANNEL	= 0x00000040,/**< deprecated not used any more */
	AUDIO_CHANNEL_24BIT				= 0x00000080/**< if the audio channel bit depth is 24 bits this flag must be set*/
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
	BLUE_AUDIO_AES_PAIR0=4, /**< Used to select stereo pair 0 as audio input source. This is only supported on SD Greed Derivative cards.*/
	BLUE_AUDIO_AES_PAIR1=5,/**< Used to select stereo pair 1 as audio input source. This is only supported on SD Greed Derivative cards.*/
	BLUE_AUDIO_AES_PAIR2=6,/**< Used to select stereo pair 2 as audio input source. This is only supported on SD Greed Derivative cards.*/
	BLUE_AUDIO_AES_PAIR3=7,/**< Used to select stereo pair 3 as audio input source. This is only supported on SD Greed Derivative cards.*/
	BLUE_AUDIO_SDIC=8, /**< Used to select Emb audio from SDI C */
	BLUE_AUDIO_SDID=9, /**< Used to select Emb audio from SDI D */
	BLUE_AUDIO_INVALID=10
} Blue_Audio_Connector_Type;

typedef enum _EAudioRate
{
	AUDIO_SAMPLE_RATE_48K=48000,
	AUDIO_SAMPLE_RATE_96K=96000,
	AUDIO_SAMPLE_RATE_UNKNOWN=-1
} EAudioRate;

/**<
@brief use this enumerator to define the color space of the video signal on the SDI cable
*/
typedef enum _EConnectorSignalColorSpace
{
	RGB_ON_CONNECTOR=0x00400000, /**< Use this enumerator if the colorspace of video data on the SDI cable is RGB <br/>
										When using dual link capture/playback , user can choose the 
										color space of the data. <br>
										In single link SDI the color space of the signal is always YUB*/
	YUV_ON_CONNECTOR=0 /**<Use this enumerator if color space of video data on the SDI cable  is RGB.*/
}EConnectorSignalColorSpace;


/**<
@brief use this enumerator for controlling the dual link functionality.
*/
typedef enum _EDualLinkSignalFormatType
{
	Signal_FormatType_4224=0, /**< sets the card to work in  4:2:2:4 mode*/
	Signal_FormatType_4444=1,/**< sets the card to work in  4:4:4 10 bit dual link mode*/
	Signal_FormatType_444_10BitSDI=Signal_FormatType_4444,/**< sets the card to work in  10 bit 4:4:4 dual link mode*/
	Signal_FormatType_444_12BitSDI=0x4,/**< sets the card to work in  4:4:4 12 bit dual link mode*/
	Signal_FormatType_Independent_422 = 0x2,
	Signal_FormatType_Key_Key=0x8000/**< not used currently on epoch cards */
	
}EDualLinkSignalFormatType;


enum ECardOperatingMode
{
	CardOperatingMode_SingleLink=0x0,
	CardOperatingMode_Independent_422=CardOperatingMode_SingleLink,
	CardOperatingMode_DualLink=0x1,	
	CardOperatingMode_StereoScopic_422=0x3,	
	CardOperatingMode_Dependent_422=CardOperatingMode_StereoScopic_422,/**< not used currently on epoch cards */
};


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
/**< 
@brief this enumerator contains the status of the driver video/hanc fifo 
*/
typedef enum
{
	BLUE_FIFO_CLOSED=0, /**< Fifo has not been initialized*/
	BLUE_FIFO_STARTING=1,/**< Fifo is starting */
	BLUE_FIFO_RUNNING=2,/**< Fifo is running */
	BLUE_FIFO_STOPPING=3,/**< Fifo is in the process of stopping */
	BLUE_FIFO_PASSIVE=5,/**< Fifo is currently stopped or not active*/
}BlueVideoFifoStatus;
#endif

/**<
@brief use this enumerator to define the data range of the RGB video frame data.
*/
typedef enum _ERGBDataRange
{
	CGR_RANGE=0, /**<	In this mode RGB data expected by the user (capture) or provided by the user(playback) is 
						in the range of 0-255(8 bit) or 0-1023(10 bit0).<br/>
						driver uses this information to choose the appropriate YUV conversion matrices.*/
	SMPTE_RANGE=1  /**<	In this mode RGB data expected by the user (capture) or provided by the user(playback) is 
						in the range of 16-235(8 bit) or 64-940(10 bit0).<br/>
						driver uses this information to choose the appropriate YUV conversion matrices.*/
}ERGBDataRange;

typedef enum _EHD_XCONNECTOR_MODE
{
	SD_SDI=1,
	HD_SDI=2
}EHD_XCONNECTOR_MODE;

/**< @brief  this enumerator can be used to set the image orienatation of the frame.							
*/
typedef enum _EImageOrientation
{
	ImageOrientation_Normal=0,	/**< in this configuration , frame is top to bottom and left to right */
	ImageOrientation_VerticalFlip=1, /**< in this configuration frame is bottom to top and left to right*/
	ImageOrientation_Invalid=2,	
}EImageOrientation;

/**< @brief this enumerator defines the reference signal source that can be used with bluefish cards
*/
typedef enum _EBlueGenlockSource
{
	BlueGenlockBNC=0, /**< Genlock is used as reference signal source */
	BlueSDIBNC=0x10000, /**< SDI input B  is used as reference signal source */
	BlueSDI_B_BNC=BlueSDIBNC,
	BlueSDI_A_BNC=0x20000,/**< SDI input A  is used as reference signal source */
	BlueAnalog_BNC=0x40000, /**< Analog input is used as reference signal source */
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

	BLUE_VIDEOCHANNEL_E=4,
	BLUE_VIDEO_INPUT_CHANNEL_C=BLUE_VIDEOCHANNEL_E,

	BLUE_VIDEOCHANNEL_F=5,
	BLUE_VIDEO_INPUT_CHANNEL_D=BLUE_VIDEOCHANNEL_F,

	BLUE_VIDEOCHANNEL_G=6,
	BLUE_VIDEO_OUTPUT_CHANNEL_C=BLUE_VIDEOCHANNEL_G,

	BLUE_VIDEOCHANNEL_H=7,
	BLUE_VIDEO_OUTPUT_CHANNEL_D=BLUE_VIDEOCHANNEL_H,
	
	BLUE_OUTPUT_MEM_MODULE_A=BLUE_VIDEO_OUTPUT_CHANNEL_A,
	BLUE_OUTPUT_MEM_MODULE_B=BLUE_VIDEO_OUTPUT_CHANNEL_B,
	BLUE_INPUT_MEM_MODULE_A=BLUE_VIDEO_INPUT_CHANNEL_A,
	BLUE_INPUT_MEM_MODULE_B=BLUE_VIDEO_INPUT_CHANNEL_B,
	//BLUE_JETSTREAM_SCALER_MODULE_0=0x10,
	//BLUE_JETSTREAM_SCALER_MODULE_1=0x11,
	//BLUE_JETSTREAM_SCALER_MODULE_2=0x12,
	//BLUE_JETSTREAM_SCALER_MODULE_3=0x13,

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


/**<@brief this enumerator is not used need to be removed*/
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
	BLUE_CONNECTOR_SDI_OUTPUT_C,
	BLUE_CONNECTOR_SDI_OUTPUT_D,

	BLUE_CONNECTOR_AES = 300,
	BLUE_CONNECTOR_ANALOG_AUDIO_1,
	BLUE_CONNECTOR_ANALOG_AUDIO_2,

	BLUE_CONNECTOR_DVID_6,
	BLUE_CONNECTOR_SDI_INPUT_C= BLUE_CONNECTOR_DVID_6,
	BLUE_CONNECTOR_DVID_7,
	BLUE_CONNECTOR_SDI_INPUT_D= BLUE_CONNECTOR_DVID_7,

	//BLUE_CONNECTOR_RESOURCE_BLOCK=0x400,
	//BLUE_CONNECTOR_JETSTREAM_SCALER_0=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_JETSTREAM_SCALER_MODULE_0),
	//BLUE_CONNECTOR_JETSTREAM_SCALER_1=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_JETSTREAM_SCALER_MODULE_1),
	//BLUE_CONNECTOR_JETSTREAM_SCALER_2=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_JETSTREAM_SCALER_MODULE_2),
	//BLUE_CONNECTOR_JETSTREAM_SCALER_3=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_JETSTREAM_SCALER_MODULE_3),

	//BLUE_CONNECTOR_OUTPUT_MEM_MODULE_A=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_OUTPUT_MEM_MODULE_A),
	//BLUE_CONNECTOR_OUTPUT_MEM_MODULE_B=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_OUTPUT_MEM_MODULE_B),
	//BLUE_CONNECTOR_INPUT_MEM_MODULE_A=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_INPUT_MEM_MODULE_A),
	//BLUE_CONNECTOR_INPUT_MEM_MODULE_B=(BLUE_CONNECTOR_RESOURCE_BLOCK|BLUE_INPUT_MEM_MODULE_B),
	//
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

	BLUE_CONNECTOR_PROP_STEREO_MODE_SIDE_BY_SIDE,
	BLUE_CONNECTOR_PROP_STEREO_MODE_TOP_DOWN,
	BLUE_CONNECTOR_PROP_STEREO_MODE_LINE_BY_LINE,

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
/**
@desc use the values in this enumerator for controlling card property
*/
typedef enum _EBlueCardProperty
{
	VIDEO_DUAL_LINK_OUTPUT=0,/**<  Use this property to enable/diable cards dual link output property*/
	VIDEO_DUAL_LINK_INPUT=1,/**<  Use this property to enable/diable cards dual link input property*/
	VIDEO_DUAL_LINK_OUTPUT_SIGNAL_FORMAT_TYPE=2, /**<Use this property to select signal format type that should be used 
							when dual link output is enabled. Possible values this property can
							accept is defined in the enumerator EDualLinkSignalFormatType
												*/
	VIDEO_DUAL_LINK_INPUT_SIGNAL_FORMAT_TYPE=3,/**<	Use this property to select signal format type that should be used 
						when dual link input is enabled. Possible values this property can
						accept is defined in the enumerator EDualLinkSignalFormatType
						*/
	VIDEO_OUTPUT_SIGNAL_COLOR_SPACE=4,/**<	Use this property to select color space of the signal when dual link output is set to 
					use 4:4:4/4:4:4:4 signal format type. Possible values this property can
					accept is defined in the enumerator EConnectorSignalColorSpace
					*/
	VIDEO_INPUT_SIGNAL_COLOR_SPACE=5,/**<	Use this property to select color space of the signal when dual link input is set to 
					use 4:4:4/4:4:4:4 signal format type. Possible values this property can
					accept is defined in the enumerator EConnectorSignalColorSpace
					*/
	VIDEO_MEMORY_FORMAT=6, /**<Use this property to ser the pixel format that should be used by 
			video output channels.  Possible values this property can
			accept is defined in the enumerator EMemoryFormat
			*/	
	VIDEO_MODE=7,/**<Use this property to set the video mode that should be used by 
			video output channels.  Possible values this property can
			accept is defined in the enumerator EVideoMode
			*/	//
	VIDEO_UPDATE_TYPE=8,/**<Use this property to set the framestore update type that should be used by 
			video output channels.  Card can update video framestore at field/frame rate.
			Possible values this property can accept is defined in the enumerator EUpdateMethod
			*/	
	VIDEO_ENGINE=9,
	VIDEO_IMAGE_ORIENTATION=10,/**<	Use this property to set the image orientation of the video output framestore.
				This property must be set before frame is transferred to on card memory using 
				DMA transfer functions(system_buffer_write_async). It is recommended to use 
				vertical flipped image orientation only on RGB pixel formats.
				Possible values this property can accept is defined in the enumerator EImageOrientation
				*/
	VIDEO_USER_DEFINED_COLOR_MATRIX=11,
	VIDEO_PREDEFINED_COLOR_MATRIX=12,//EPreDefinedColorSpaceMatrix
	VIDEO_RGB_DATA_RANGE=13,		/**<	Use this property to set the data range of RGB pixel format, user can specify 
											whether the RGB data is in either SMPTE or CGR range. Based on this information 
											driver is decide which color matrix should be used.
											Possible values this property can accept is defined in the enumerator ERGBDataRange
											For SD cards this property will set the input and the output to the specified value.
											For Epoch/Create/SuperNova cards this property will only set the output to the specified value.
											For setting the input on Epoch/Create/SuperNova cards see EPOCH_VIDEO_INPUT_RGB_DATA_RANGE*/
	VIDEO_KEY_OVER_BLACK=14,/**< this property is deprecated and no longer supported on epoch/create range of cards.*/	
	VIDEO_KEY_OVER_INPUT_SIGNAL=15,
	VIDEO_SET_DOWN_CONVERTER_VIDEO_MODE=16,/**< this property is deprecated and no longer supported on epoch/create range of cards.
						EHD_XCONNECTOR_MODE
						*/
	VIDEO_LETTER_BOX=17,
	VIDEO_PILLOR_BOX_LEFT=18,
	VIDEO_PILLOR_BOX_RIGHT=19,
	VIDEO_PILLOR_BOX_TOP=20,
	VIDEO_PILLOR_BOX_BOTTOM=21,
	VIDEO_SAFE_PICTURE=22,
	VIDEO_SAFE_TITLE=23,
	VIDEO_INPUT_SIGNAL_VIDEO_MODE=24,/**< Use this property to retreive the video input signal information on the 
					default video input channel used by that  SDK object.
					*/
	VIDEO_COLOR_MATRIX_MODE=25,
	VIDEO_OUTPUT_MAIN_LUT=26,/**< this property is deprecated and no longer supported on epoch/create range of cards.*/
	VIDEO_OUTPUT_AUX_LUT=27,/**< this property is deprecated and no longer supported on epoch/create range of cards.*/
	VIDEO_LTC=28,		/**< this property is deprecated and no longer supported on epoch/create range of cards. To retreive/ outputting 
				LTC information you can use the HANC decoding and encoding functions.*/
	VIDEO_GPIO=29,	
	VIDEO_PLAYBACK_FIFO_STATUS=30, /**< This property can be used to retreive how many frames are bufferd in the video playback fifo.*/
	RS422_RX_BUFFER_LENGTH=31,
	RS422_RX_BUFFER_FLUSH=32,
	VIDEO_INPUT_UPDATE_TYPE=33,/**<	Use this property to set the framestore update type that should be used by 
				video input  channels.  Card can update video framestore at field/frame rate.
				Possible values this property can accept is defined in the enumerator EUpdateMethod
									*/	
	VIDEO_INPUT_MEMORY_FORMAT=34,/**<Use this property to set the pixel format that should be used by 
					video input channels when it is capturing a frame from video input source.  
					Possible values this property can accept is defined in the enumerator EMemoryFormat
									*/	
	VIDEO_GENLOCK_SIGNAL=35,/**< Use this property to retrieve video signal of the reference source that is used by the card.
				This can also be used to select the reference signal source that should be used. 	
				*/

	AUDIO_OUTPUT_PROP=36,	/**< this can be used to route PCM audio data onto respective audio output connectors. */
	AUDIO_CHANNEL_ROUTING=AUDIO_OUTPUT_PROP,
	AUDIO_INPUT_PROP=37,/**< Use this property to select audio input source that should be used when doing 
			an audio capture.
			Possible values this property can accept is defined in the enumerator Blue_Audio_Connector_Type.
			*/
	VIDEO_ENABLE_LETTERBOX=38,
	VIDEO_DUALLINK_OUTPUT_INVERT_KEY_COLOR=39,/**< this property is deprecated and no longer supported on epoch/create range of cards.*/
	VIDEO_DUALLINK_OUTPUT_DEFAULT_KEY_COLOR=40,/**< this property is deprecated and no longer supported on epoch/create range of cards.*/
	VIDEO_BLACKGENERATOR=41, /**< Use this property to control the black generator on the video output channel.
							 */
	VIDEO_INPUTFRAMESTORE_IMAGE_ORIENTATION=42,
	VIDEO_INPUT_SOURCE_SELECTION=43, /**<	The video input source that should be used by the SDK default video input channel 
					can be configured using this property.	
					Possible values this property can accept is defined in the enumerator EBlueConnectorIdentifier.
					*/
	DEFAULT_VIDEO_OUTPUT_CHANNEL=44,
	DEFAULT_VIDEO_INPUT_CHANNEL=45,
	VIDEO_REFERENCE_SIGNAL_TIMING=46,
	EMBEDEDDED_AUDIO_OUTPUT=47, /**< the embedded audio output property can be configured using this property.
					Possible values this property can accept is defined in the enumerator EBlueEmbAudioOutput.
					*/
	EMBEDDED_AUDIO_OUTPUT=EMBEDEDDED_AUDIO_OUTPUT,
	VIDEO_PLAYBACK_FIFO_FREE_STATUS=48, /**< this will return the number of free buffer in the fifo. 
					If the video engine  is framestore this will give you the number of buffers that the framestore mode 
					can you use with that video output channel.*/
	VIDEO_IMAGE_WIDTH=49,		/**< only for selective DMA of a smaller image onto video output raster; size in bytes (not pixels) */
	VIDEO_IMAGE_HEIGHT=50,		/**< only for selective DMA of a smaller image onto video output raster; number of lines */
	VIDEO_SCALER_MODE=51,
	AVAIL_AUDIO_INPUT_SAMPLE_COUNT=52,
	VIDEO_PLAYBACK_FIFO_ENGINE_STATUS=53,	/**< this will return the playback fifo status. The values returned by this property 
						are defined in the enumerator BlueVideoFifoStatus.
						*/	
	VIDEO_CAPTURE_FIFO_ENGINE_STATUS=54,	/**< this will return the capture  fifo status. 
						The values returned by this property are defined in the enumerator BlueVideoFifoStatus.
						*/
	VIDEO_2K_1556_PANSCAN=55,/**< this property is deprecated and no longer supported on epoch/create range of cards.*/
	VIDEO_OUTPUT_ENGINE=56,	/**< Use this property to set the video engine of the video output channels.
				Possible values this property can accept is defined in the enumerator EEngineMode 
				*/
	VIDEO_INPUT_ENGINE=57, /**< Use this property to set the video engine of the video input channels.
				Possible values this property can accept is defined in the enumerator EEngineMode 
				*/
	BYPASS_RELAY_A_ENABLE=58, /**< use this property to control the bypass relay on SDI A output.*/
	BYPASS_RELAY_B_ENABLE=59, /**< use this property to control the bypass relay on SDI B output.*/
	VIDEO_PREMULTIPLIER=60,
	VIDEO_PLAYBACK_START_TRIGGER_POINT=61, /**< Using this property you can instruct the driver to start the 
						video playback fifo on a particular video output field count.
						Normally video playback fifo is started on the next video interrupt after 
						the video_playback_start call.*/
	GENLOCK_TIMING=62,
	VIDEO_IMAGE_PITCH=63,
	VIDEO_IMAGE_OFFSET=64,
	VIDEO_INPUT_IMAGE_WIDTH=65,
	VIDEO_INPUT_IMAGE_HEIGHT=66,
	VIDEO_INPUT_IMAGE_PITCH=67,
	VIDEO_INPUT_IMAGE_OFFSET=68,
	TIMECODE_RP188=69,	/**< this property is deprecated and no longer supported on epoch/create range of cards.*/
	BOARD_TEMPERATURE=70,/**<This property can be used to retreive the Board temperature, core temperature and 
				RPM of the Fan on epoch/create range of cards.<br/>
				Use the macro's EPOCH_CORE_TEMP ,EPOCH_BOARD_TEMP and EPOCH_FAN_SPEED
				to retireive the respective values from the property.<br/>							
						*/	
	MR2_ROUTING=71,	/**< Use this property to control the MR2 functionlity on epoch range of cards.
			Use the following macro with this property.<br/>
			1) EPOCH_SET_ROUTING --> for setting the source, destination and link type of the routing connection,<br/>
			2) EPOCH_ROUTING_GET_SRC_DATA --> for getting the routing source.<br/>
							The possible source and destination elements supported by the routing matrix are defined in the 
							enumerator EEpochRoutingElements.<br/>
			*/
	SAVEAS_POWERUP_SETTINGS=72,
	VIDEO_CAPTURE_AVAIL_BUFFER_COUNT=73, /**< This property will return the number of captured frame avail in the fifo at present.
						If the video engine  is framestore this will give you the number of buffers that the framestore mode 
						can you use with that video input channel */
	EPOCH_APP_WATCHDOG_TIMER=74,/**< Use this property to control the application  watchdog timer functionality. 
					Possible values this property can accept is defined in the enumerator enum_blue_app_watchdog_timer_prop.
				*/	
	EPOCH_RESET_VIDEO_INPUT_FIELDCOUNT=75, /**< Use this property to reset the field count on both the 
						video channels of the card. You can pass the value that 
						should be used as starting fieldcount after the reset.
						This property can be used to keep track  sync between left and right signal 
						when you are capturing in stereoscopic mode.
					*/
	EPOCH_RS422_PORT_FLAGS=76,/**<	Use this property to set the master/slave property of the RS422 ports.
				Possible values this property can accept is defined in the enumerator enum_blue_rs422_port_flags.
				*/
	EPOCH_DVB_ASI_INPUT_TIMEOUT=77,	/**< Current DVB ASI input firmware does not support this property in hardware,
					this is a future addition.
					Use this property to set the timeout  of the DVB ASI input stream. 
					timeout is specified in  milliseconds.If hardware did not get the required no of 		
					packets( specified using EPOCH_DVB_ASI_INPUT_LATENCY_PACKET_COUNT)
					within the period specified in the timeout, hardware would generate a video input interrupt
					and it would be safe to read the dvb asi packet from the card.
					*/
	EPOCH_DVB_ASI_INPUT_PACKING_FORMAT=78, /**< Use this property to specify the packing method that should be used 
						when capturing DVB ASI packets.
						The possible packing methods are defined in the enumerator enum_blue_dvb_asi_packing_format.*/
	EPOCH_DVB_ASI_INPUT_LATENCY_PACKET_COUNT=79, /**< Use this property to set  how many asi packets should be captured by the card , before it 
							notifies the driver of available data using video input interrupt.<br/>
							*/
	VIDEO_PLAYBACK_FIFO_CURRENT_FRAME_UNIQUEID=80, /**< This property can be used to query the current unique id of 
							the frame that is being displayed currently by the video output channel. This 
							property is only usefull in the context of video fifo.<br/>
							You get a uniqueid when you present a frame using video_playback_present function.
							Alternative ways to get this information are <br/>
							1) using blue_wait_video_sync_async , the member current_display_frame_uniqueid contains the same information<br/>
							2) using wait_video_output_sync function on epoch cards, if 
							the flag UPD_FMT_FLAG_RETURN_CURRENT_UNIQUEID is appended with 
							either UPD_FMT_FRAME or UPD_FMT_FIELD , the return value of 
							the function wait_video_output_sync woukd contain the current display
							frames uniqueid.<br/>*/

	EPOCH_DVB_ASI_INPUT_GET_PACKET_SIZE = 81,/**< use this property to get the size of each asi transport stream packet
							(whether it is 188 or 204.*/
	EPOCH_DVB_ASI_INPUT_PACKET_COUNT = 82,/**< this property would give you the number of packets captured during the last 
						interrupt time frame. For ASI interrupt is generated if 
						hardware captured the requested number of packets or it hit the 
						timeout value
						*/
	EPOCH_DVB_ASI_INPUT_LIVE_PACKET_COUNT = 83,/**< this property would give you the number of packets that
						is being captured during the current interrupt time frame. 
						For ASI interrupt is generated when has hardware captured the 
						requested number of packets specified using  
						EPOCH_DVB_ASI_INPUT_LATENCY_PACKET_COUNT property.
						*/
	EPOCH_DVB_ASI_INPUT_AVAIL_PACKETS_IN_FIFO = 84,/**< This property would return the number of ASI packets 
							that has been captured into card memory , that
							can be retreived.
							This property is only valid when the video input 
							channel is being used in FIFO modes.
						*/
	EPOCH_ROUTING_SOURCE_VIDEO_MODE=VIDEO_SCALER_MODE,/**< Use this property to change the video mode that scaler should be set to.
							USe the macro SET_EPOCH_SCALER_MODE when using this property, as this macro 
							would allow you to select which one of the scaler blocks video mode should be updated.
							*/
	EPOCH_AVAIL_VIDEO_SCALER_COUNT=85,/**< This property would return available scaler processing block available on the card.*/
	EPOCH_ENUM_AVAIL_VIDEO_SCALERS_ID=86,/**< You can enumerate the available scaler processing block available on the card using this property.
					You pass in the index value as input parameter to get the scaler id that should be used.
					Applications are recommended to use this property to query the available scaler id's 
					rather than hardcoding a scaler id. As the scaler id's that you can use would vary based on
					whether you have VPS0 or VPS1 boards loaded on the base board.
					*/
	EPOCH_ALLOCATE_VIDEO_SCALER=87,	/**< This is just a helper property for applications who need to use more than one scaler
					and just wants to query the next available scaler from the driver pool, rather than hardcoding 
					each thread to use a particular scaler.
					Allocate a free scaler from the available scaler pool.
					User has got the option to specify whether they want to use the scaler for 
					use with a single link or dual link stream */
	EPOCH_RELEASE_VIDEO_SCALER=88, /**< Release the previously allocated scaler processing block back to the free pool.
					If the user passes in a value of 0, all the allocated scaler blocks in the driver are released.
					So effectively
					*/
	EPOCH_DMA_CARDMEMORY_PITCH=89,
	EPOCH_OUTPUT_CHANNEL_AV_OFFSET=90,
	EPOCH_SCALER_CHANNEL_MUX_MODE=91,
	EPOCH_INPUT_CHANNEL_AV_OFFSET=92,
	EPOCH_AUDIOOUTPUT_MANUAL_UCZV_GENERATION=93,/* ASI firmware only */
	EPOCH_SAMPLE_RATE_CONVERTER_BYPASS=94,
	EPOCH_GET_PRODUCT_ID=95,					/* returns the enum for the firmware type EEpochFirmwareProductID */
	EPOCH_GENLOCK_IS_LOCKED=96,
	EPOCH_DVB_ASI_OUTPUT_PACKET_COUNT=97,		/* ASI firmware only */
	EPOCH_DVB_ASI_OUTPUT_BIT_RATE=98,			/* ASI firmware only */
	EPOCH_DVB_ASI_DUPLICATE_OUTPUT_A=99,		/* ASI firmware only */
	EPOCH_DVB_ASI_DUPLICATE_OUTPUT_B=100,		/* ASI firmware only */
	EPOCH_SCALER_HORIZONTAL_FLIP=101,			/* see SideBySide_3D sample application */
	EPOCH_CONNECTOR_DIRECTION=102,				/* see application notes */
	EPOCH_AUDIOOUTPUT_VALIDITY_BITS=103,		/* ASI firmware only */
	EPOCH_SIZEOF_DRIVER_ALLOCATED_MEMORY=104,	/* video buffer allocated in Kernel space; accessible in userland via system_buffer_map() */
	INVALID_VIDEO_MODE_FLAG=105,				/* returns the enum for VID_FMT_INVALID that this SDK/Driver was compiled with;
												it changed between 5.9.x.x and 5.10.x.x driver branch and has to be handled differently for
												each driver if the application wants to use the VID_FMT_INVALID flag and support both driver branches */
	EPOCH_VIDEO_INPUT_VPID=106,					/* returns the VPID for the current video input signal */
	EPOCH_LOW_LATENCY_DMA=107,					/* not fully supported yet */
	EPOCH_VIDEO_INPUT_RGB_DATA_RANGE=108,

	VIDEO_CARDPROPERTY_INVALID=1000
}EBlueCardProperty;


typedef enum _EAnalogConnectorSignalType
{
	ANALOG_OUTPUTSIGNAL_CVBS_Y_C=1,
	ANALOG_OUTPUTSIGNAL_COMPONENT=2,
	ANALOG_OUTPUTSIGNAL_RGB=4
}EAnalogConnectorSignalType;

/**<
@brief Use this enumerator to set the analog video signal types and connectors.
*/
typedef enum _EAnalogInputConnectorType 
{
/* Composite input */
	ANALOG_VIDEO_INPUT_CVBS_AIN1=0x00, /**<only available on Mini COAX */
	ANALOG_VIDEO_INPUT_CVBS_AIN2=0x01, /**<available on both Mini COAX and Mini DIN*/
	ANALOG_VIDEO_INPUT_CVBS_AIN3=0x02, /**<available on both Mini COAX and Mini DIN*/
	ANALOG_VIDEO_INPUT_CVBS_AIN4=0x03, /**<only available on Mini COAX */
	ANALOG_VIDEO_INPUT_CVBS_AIN5=0x04, /**<only available on Mini COAX */
	ANALOG_VIDEO_INPUT_CVBS_AIN6=0x05, /**<available on both Mini COAX and Mini DIN */

/*svideo input*/
//Y_C is a synonym for svideo
	ANALOG_VIDEO_INPUT_Y_C_AIN1_AIN4=0x06, /**<only available on Mini COAX*/
	ANALOG_VIDEO_INPUT_Y_C_AIN2_AIN5=0x07, /**<only available on Mini COAX*/
	ANALOG_VIDEO_INPUT_Y_C_AIN3_AIN6=0x08, /**<available on both Mini COAX and Mini DIN*/

/*YUV input*/
	ANALOG_VIDEO_INPUT_YUV_AIN1_AIN4_AIN5=0x09, /**<only available on Mini COAX*/
	ANALOG_VIDEO_INPUT_YUV_AIN2_AIN3_AIN6=0x0a, /**<available on both Mini COAX and Mini DIN*/
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
/**< brief Used to determine how video interrupts are handled*/
typedef enum _EEngineMode
{
	VIDEO_ENGINE_FRAMESTORE=0,	/**< framestore engine. In this mode user is responsible for 
									 schduling a capture or playback after waiting for the 
									 respective video sync;s*/
	VIDEO_ENGINE_PLAYBACK=1,	/**< Playback engine. In this mode there is a driver FIFO, which 
									is reponisble for scheudling a frame for  playback.
									User just adds video frames into the fifo.*/
	VIDEO_ENGINE_CAPTURE=2,		/**< Capture engine  In this mode there is a driver FIFO, which 
									is reponisble for scheudling a frame for capture.
									User just retreives video frames from the FIFO.*/
	VIDEO_ENGINE_PAGEFLIP=3,	/**< not supported any more */
	VIDEO_ENGINE_DUPLEX=4,		/**< Full Duplex video. This is a FIFO mode. Use this mode if you want 
								to capture and playback at the same time.*/
	VIDEO_ENGINE_INVALID
} EEngineMode;

/**< use this enumerator for controlling emb audio output properties using the 
	 property EMBEDDED_AUDIO_OUTPUT.	
*/
typedef enum _EBlueEmbAudioOutput
{
	blue_emb_audio_enable=0x1,	// Switches off/on  the whole HANC output from connecotrs associated with the channel
	blue_auto_aes_to_emb_audio_encoder=0x2, //control whether the auto aes to emb thread should be running or not.
	blue_emb_audio_group1_enable=0x4, /**< enables group1(ch 0-  3) emb audio */
	blue_emb_audio_group2_enable=0x8, /**< enables group2(ch 4-  7) emb audio */
	blue_emb_audio_group3_enable=0x10, /**< enables group3(ch 8-  11) emb audio */
	blue_emb_audio_group4_enable=0x20, /**< enables group4(ch 12-  16) emb audio */
	blue_enable_hanc_timestamp_pkt = 0x40
}EBlueEmbAudioOutput;


/**< Not used any more */
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
	COL_GREEN_Y=2,
	COL_KEY=3
}MatrixColType;




/**< Bits defining supported features that can be used with VideoFeature_struct*/
#define VIDEO_CAPS_INPUT_SDI					(0x00000001)    /**<  Capable of input of SDI Video */
#define VIDEO_CAPS_OUTPUT_SDI					(0x00000002)    /**<  Capable of output of SDI Video */
#define VIDEO_CAPS_INPUT_COMP					(0x00000004)    /**<  Capable of capturing Composite Video input */
#define VIDEO_CAPS_OUTPUT_COMP					(0x00000008)    /**<  Capable of capturing Composite Video output */

#define VIDEO_CAPS_INPUT_YUV					(0x00000010)    /**<  Capable of capturing Component Video input */
#define VIDEO_CAPS_OUTPUT_YUV					(0x00000020)    /**<  Capable of capturing Component Video output */
#define VIDEO_CAPS_INPUT_SVIDEO					(0x00000040)    /**<  Capable of capturing SVideo input */
#define VIDEO_CAPS_OUTPUT_SVIDEO				(0x00000080)    /**<  Capable of capturing SVideo output */

#define VIDEO_CAPS_GENLOCK						(0x00000100)    /**<  Able to adjust Vert & Horiz timing */
#define VIDEO_CAPS_VERTICAL_FLIP				(0x00000200)    /**<  Able to flip rasterisation */
#define VIDEO_CAPS_KEY_OUTPUT					(0x00000400)    /**<  Video keying output capable */
#define VIDEO_CAPS_4444_OUTPUT					(0x00000800)    /**<  Capable of outputting 4444 (dual link) */

#define VIDEO_CAPS_DUALLINK_INPUT				(0x00001000)	/**<  Dual Link input   */
#define VIDEO_CAPS_INTERNAL_KEYER				(0x00002000)	/**<  Has got an internal Keyer */
#define VIDEO_CAPS_RGB_COLORSPACE_SDI_CONN		(0x00004000)	/**<  Support RGB colorspace  in on an  SDI connector  */
#define VIDEO_CAPS_HAS_PILLOR_BOX				(0x00008000)	/**<  Has got support for pillor box */

#define VIDEO_CAPS_OUTPUT_RGB 					(0x00010000)    /**<  Has Analog RGB output connector  */
#define VIDEO_CAPS_SCALED_RGB 					(0x00020000)    /**<  Can scale RGB colour space */
#define AUDIO_CAPS_PLAYBACK 					(0x00040000)	/**<  Has got audio output */
#define AUDIO_CAPS_CAPTURE 						(0x00080000)

#define VIDEO_CAPS_DOWNCONVERTER				(0x00100000)
#define VIDEO_CAPS_DUALOUTPUT_422_IND_STREAM	(0x00200000)	/**<  Specifies whether the card supports Dual Indepenedent 422 output streams */
#define VIDEO_CAPS_DUALINPUT_422_IND_STREAM		(0x00400000)	/**<  Specifies whether the card supports Dual Indepenedent 422 input streams */

#define VIDEO_CAPS_VBI_OUTPUT					(0x00800000)	/**<  Specifies whether the card supports VBI output */
#define VIDEO_CAPS_VBI_INPUT					(0x04000000)	/**<  Specifies whether the card supports VBI input */	

#define VIDEO_CAPS_HANC_OUTPUT					(0x02000000)
#define VIDEO_CAPS_HANC_INPUT					(0x04000000)

#define VIDEO_CAPS_FOUND_VPS0					VIDEO_CAPS_DOWNCONVERTER	/**< specifies whether the VPS0 scaler board was found on the card */
#define VIDEO_CAPS_FOUND_VPS1					(0x10000000)				/**< specifies whether the VPS1 scaler board was found on the card */
#define VIDEO_CAPS_FOUND_VPIO					(0x20000000)				/**< specifies whether the VPIO(DVI daughter board)board was found on the card */

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


#define Blue_DMABuffer(CardBufferType,BufferId,DataType)		( (((ULONG)DataType&0xF)<<(ULONG)BLUE_DMA_DATA_TYPE_OFFSET)| \
																( CardBufferType<<(ULONG)BLUE_CARD_BUFFER_TYPE_OFFSET) | \
																( ((ULONG)BufferId&0xFFF)) |0)

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


#define BlueBuffer(CardBufferType,BufferId)			(((CardBufferType)<<BLUE_CARD_BUFFER_TYPE_OFFSET)|((BufferId&0xFFF))|0)
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
	BLUE_DATA_FRAME_STEREO_LEFT=BLUE_DATA_FRAME,
	BLUE_DATA_FRAME_STEREO_RIGHT=8,
	BLUE_DMADATA_INVALID=9
}EDMADataType;

typedef struct _AUXILLARY_VIDEO_INFO
{
	BLUE_UINT32  video_channel_id;
	BLUE_UINT32  lVideoMode;
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
	BLUE_VIDEO_AUX_VIDFMT_CHANGE=8,

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
	BLUE_UINT32 current_display_frame_id; // would give you the current frame id which is being displayed
	BLUE_UINT32 current_display_frame_uniqueid; // would give you the unique id associated with current frame id which is being displayed
												// this is only valid when using fifo modes.
	BLUE_UINT8  pad[24];
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
struct VideoFeature_struct 
{
	BLUE_UINT32  Type;                   // Bluefish card type
	BLUE_UINT32  CardSubType;		
	BLUE_UINT32  Bus;                    // Which PIC bus (bridge) it is on
	BLUE_UINT32  Slot;                   // Which slot card is plugged into
	BLUE_UINT32  Feature;                //  Look at  the _EBlueFishCardFeatures  definition to know what each bit mean
	BLUE_UINT32  FirmwareVersion;
};

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
	BLUE_UINT64 ltcTimeCode;			//LTC timecode
	unsigned long videochannel;			//the channel this frame was captured from
	long BufferId;						//this buffer contains the captured frame
	unsigned long Count;				//total captured frames
	unsigned long DroppedFrameCount;	//dropped frame count
	unsigned long nFrameTimeStamp;		//field count the frame was captured at
	unsigned long nVideoSignalType;		//video mode of this frame
	unsigned int  nASIPktCount;			//only for DVB-ASI; how many ASI packets are in this frame
	unsigned int  nASIPktSize;			//only for DVB-ASI; how many bytes per packet
	unsigned int  nAudioValidityBits;	//part of the channels status block for audio
	unsigned char pad[20];				//not used
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

enum SerialPort_struct_flags
{
	SerialPort_Read=1,
	SerialPort_Write=2,
	SerialPort_TX_Queue_Status=4,		
	SerialPort_RX_Queue_Status=8,
	SerialPort_RX_FlushBuffer=16,
	SerialPort_RX_IntWait_Return_On_Data=32,
	
};

struct SerialPort_struct
{
	unsigned char 	Buffer[64];
	unsigned int	nBufLength;
	unsigned int	nSerialPortId;
	unsigned int	bFlag; // SerialPort_struct_flags 
	unsigned short	sTimeOut; 
};


struct blue_video_scaler_ceofficent
{
	BLUE_UINT32 ioctl_read_only_flag;
	BLUE_UINT32 nScalerId;
	BLUE_UINT32 nScalerFilterType;
	BLUE_UINT32 nScalerCoefficentWeight[15];
};

enum blue_video_scaler_param_flags
{
	scaler_flags_set_destrect_as_framesize = 0x1,
};

struct blue_video_scaler_param_struct
{
	BLUE_UINT32 ioctl_read_only_flag;
	BLUE_UINT32 nScalerId;
	BLUE_UINT32 nSrcVideoHeight;
	BLUE_UINT32 nSrcVideoWidth;
	BLUE_UINT32 nSrcVideoYPos;
	BLUE_UINT32 nSrcVideoXPos;
	BLUE_UINT32 nDestVideoHeight;
	BLUE_UINT32 nDestVideoWidth;
	BLUE_UINT32 nDestVideoYPos;
	BLUE_UINT32 nDestVideoXPos;
	BLUE_UINT32 nHScaleFactor;
	BLUE_UINT32 nVScaleFactor;
	BLUE_UINT32 nScalerOutputVideoMode;
	BLUE_UINT32 nScalerParamFlags;
	BLUE_UINT32 pad[128];
};
#ifndef EXCLUDE_USERLAND_STRUCT
struct blue_color_matrix_struct{
	BLUE_UINT32  VideoChannel;
	BLUE_UINT32 MatrixColumn; //MatrixColType enumerator defines this 
	double	Coeff_B;
	double	Coeff_R;
	double	Coeff_G;
	double	Coeff_K;
	double	const_value;
};
#endif
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
	ORAC_FPGA_CONFIG_CMD_READ_DATA=7,
	ORAC_FPGA_CONFIG_INIT=8,
	ORAC_FPGA_CONFIG_EXIT=9
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

#define	AES_SRC_BYPASS_CHANNEL_1_2 0x1
#define	AES_SRC_BYPASS_CHANNEL_3_4 0x2
#define	AES_SRC_BYPASS_CHANNEL_5_6 0x4
#define	AES_SRC_BYPASS_CHANNEL_7_8 0x8

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

	EPOCH_DEST_AES_ANALOG_AUDIO_OUTPUT,

	EPOCH_SRC_AV_SIGNAL_GEN,
	EPOCH_SRC_DEST_VPIO_SCALER_0,
	EPOCH_SRC_DEST_VPIO_SCALER_1,

	EPOCH_DEST_VARIVUE_HDMI,

	EPOCH_DEST_INPUT_MEM_INTERFACE_CHC,
	EPOCH_DEST_INPUT_MEM_INTERFACE_CHD,

	EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHC,
	EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHD,

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

/** 
	@desc	use these macro for doing the MR2 routing  on epoch range of cards.
			MR2 routing can be controlled using the property MR_ROUTING.
*/
#define EPOCH_SET_ROUTING(routing_src,routing_dest,data_link_type) ((routing_src & 0xFF) | ((routing_dest & 0xFF)<<8) | ((data_link_type&0xFFFF)<<16))
#define EPOCH_ROUTING_GET_SRC_DATA(value)		(value & 0xFF)
#define EPOCH_ROUTING_GET_DEST_DATA(value)		((value>>8) & 0xFF)
#define EPOCH_ROUTING_GET_LINK_TYPE_DATA(value)	((value>>16) & 0xFFFF)

#define GPIO_TX_PORT_A	(1)
#define GPIO_TX_PORT_B	(2)

#define EPOCH_GPIO_TX(port,value)	(port<<16|value) // if want to set each of the GPO 
													// ports individually you should use this macro.
													// without the macro it would set both the GPO
													// ports on the card

/**
	@desc	use these macros for controlling epoch application watch dog settings.
			The card property EPOCH_APP_WATCHDOG_TIMER can be used to control 
			the watchdog timer functionality.
*/
enum enum_blue_app_watchdog_timer_prop
{
	enum_blue_app_watchdog_timer_start_stop=(1<<31),			// can be used to enable/disable timer 
	enum_blue_app_watchdog_timer_keepalive=(1<<30),				// can be used to reset the timer value
	enum_blue_app_watchdog_timer_get_present_time=(1<<29),		// can query to get the value of the timer
	enum_blue_app_watchdog_get_timer_activated_status=(1<<28),	// can query to get whether the timer has been activated
	enum_blue_app_watchdog_get_timer_start_stop_status=(1<<27),	// can query whether the timer has been set.
	enum_blue_app_watchdog_enable_gpo_on_active=(1<<26), // using this enumerator you can tell the system that when 
														// application watchdog timer has expired whether a GPO output should be triggered or not.
														// you can use also use this  enumerator  to select
														// which GPO output should be triggered with this. to use GPO port A pass a value of 
														// GPIO_TX_PORT_A when this enumerator is used.
	enum_blue_hardware_watchdog_enable_gpo=(1<<25) // can be used to enable/disable GPO trigger when hardware watchdog timer has been 
													// triggered
};

#define EPOCH_WATCHDOG_TIMER_SET_MACRO(prop,value) (prop|(value &0xFFFF))
#define EPOCH_WATCHDOG_TIMER_QUERY_MACRO(prop) (prop)
#define EPOCH_WATCHDOG_TIMER_GET_VALUE_MACRO(value) (value&0xFFFF)

enum enum_blue_rs422_port_flags
{
	enum_blue_rs422_port_set_as_slave =(1<<0) // If this flag is set the RS422 port would be set to slave mode.
											  // by default port is setup to work in master mode , where it would be acting 
											  // as master in the transactions.											  
};
#define EPOCH_RS422_PORT_FLAG_SET_MACRO(portid,value)	((portid&0x3)|(value<<3))
#define EPOCH_RS422_PORT_FLAG_GET_FLAG_MACRO(value)		((value>>3)&0xFFFF)
#define EPOCH_RS422_PORT_FLAG_GET_PORTID_MACRO(value)	(value&0x3)


enum enum_blue_dvb_asi_packing_format
{
	enum_blue_dvb_asi_packed_format=1,/**< In this packing method the asi packets are stored as 188 or 204 bytes*/
	enum_blue_dvb_asi_packed_format_with_timestamp=2,/**< In this packing method the asi packets are stored as (8+188) or (8+204) bytes
															The timestamp is stored at the begininig of the packet , using 8 bytes*/
	enum_blue_dvb_asi_256byte_container_format=3,
	enum_blue_dvb_asi_256byte_container_format_with_timestamp=4
};


#define RS422_SERIALPORT_FLAG(timeout,port,RxFlushBuffer) (((unsigned long)(timeout)<<16)|(port & 0x3) | (RxFlushBuffer<<15))
// use this macro with Wait_For_SerialPort_InputData,
// if you you want the  function to return 
// immediatelty when it gets byte in the serial RX port.
#define RS422_SERIALPORT_FLAG2(timeout,port,RxFlushBuffer,RXIntWaitReturnOnAvailData) (((unsigned long)(timeout)<<16)|(port & 0x3) | (RxFlushBuffer<<15)|(RXIntWaitReturnOnAvailData<<14))

typedef enum _blue_blackgenerator_status
{
	ENUM_BLACKGENERATOR_OFF =	0,	//producing normal video output
	ENUM_BLACKGENERATOR_ON =	1,	//producing black video output
	ENUM_BLACKGENERATOR_SDI_SYNC_OFF =	2	//no valid SDI signal is coming out of our SDI output connector; only available in Epoch ASI firmware
}blue_blackgenerator_status;
