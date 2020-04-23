/*///////////////////////////////////////////////////////////////////////////
// File:        BlueDriver_p.h
// Author:      Tim Bragulla
//
// Description: Legacy definitions for usage with legacy Bluefish APIs
//              (BlueVelvet and BlueVelvetC)
//
// (C) Copyright 2017 by Bluefish Technologies Pty Ltd. All Rights Reserved.
///////////////////////////////////////////////////////////////////////////*/


#ifndef HG_BLUE_DRIVER_P_LEGACY_HG
#define HG_BLUE_DRIVER_P_LEGACY_HG

#include "BlueTypes.h"

/*///////////////////////////////////////////////////////////////////////////
// L E G A C Y   D E C L A R A T I O N S - BEGIN
///////////////////////////////////////////////////////////////////////////*/

#define BLUE_LITTLE_ENDIAN	0
#define BLUE_BIG_ENDIAN		1

#define BLUE_CARD_BUFFER_TYPE_OFFSET		(12)
#define BLUE_DMA_DATA_TYPE_OFFSET			(16)
#define GetDMACardBufferId(value)			( value & 0x00000FFF)
#define GetCardBufferType(value)			((value & 0x0000F000) >> BLUE_CARD_BUFFER_TYPE_OFFSET)
#define GetDMADataType(value)				((value & 0x000F0000) >> BLUE_DMA_DATA_TYPE_OFFSET)

/* FLAGS FOR DMA FUNCTION CALLS */
#define Blue_DMABuffer(CardBufferType, BufferId, DataType)		( (((ULONG)DataType&0xF)<<(ULONG)BLUE_DMA_DATA_TYPE_OFFSET)| \
																( CardBufferType<<(ULONG)BLUE_CARD_BUFFER_TYPE_OFFSET) | \
																( ((ULONG)BufferId&0xFFF)) | 0)

#define BlueImage_VBI_DMABuffer(BufferId, DataType)				( (((ULONG)DataType&0xF)<<(ULONG)BLUE_DMA_DATA_TYPE_OFFSET)| \
																( BLUE_CARDBUFFER_IMAGE_VBI<<(ULONG)BLUE_CARD_BUFFER_TYPE_OFFSET) | \
																( ((ULONG)BufferId&0xFFF)) | 0)

#define BlueImage_DMABuffer(BufferId, DataType)					( (((ULONG)DataType&0xF)<<(ULONG)BLUE_DMA_DATA_TYPE_OFFSET)| \
																( BLUE_CARDBUFFER_IMAGE<<(ULONG)BLUE_CARD_BUFFER_TYPE_OFFSET) | \
																( ((ULONG)BufferId&0xFFF)) | 0)

#define BlueImage_VBI_HANC_DMABuffer(BufferId, DataType)		( (((ULONG)DataType&0xF)<<(ULONG)BLUE_DMA_DATA_TYPE_OFFSET)| \
																( BLUE_CARDBUFFER_IMAGE_VBI_HANC<<(ULONG)BLUE_CARD_BUFFER_TYPE_OFFSET) | \
																( ((ULONG)BufferId&0xFFF)) | 0)

#define BlueImage_HANC_DMABuffer(BufferId, DataType)			( (((ULONG)DataType&0xF)<<(ULONG)BLUE_DMA_DATA_TYPE_OFFSET)| \
																( BLUE_CARDBUFFER_IMAGE_HANC<<(ULONG)BLUE_CARD_BUFFER_TYPE_OFFSET) | \
																( ((ULONG)BufferId&0xFFF)) | 0)

/* FLAGS FOR CAPTURE AND PLAYBACK FUNCTION CALLS */
#define BlueBuffer(CardBufferType,BufferId)			(((CardBufferType)<<BLUE_CARD_BUFFER_TYPE_OFFSET)|((BufferId&0xFFF))|0)
#define BlueBuffer_Image_VBI(BufferId)				(((BLUE_CARDBUFFER_IMAGE_VBI)<<BLUE_CARD_BUFFER_TYPE_OFFSET)|((BufferId&0xFFF))|0)
#define BlueBuffer_Image(BufferId)					(((BLUE_CARDBUFFER_IMAGE)<<BLUE_CARD_BUFFER_TYPE_OFFSET)|((BufferId&0xFFF))|0)
#define BlueBuffer_Image_VBI_HANC(BufferId)			(((BLUE_CARDBUFFER_IMAGE_VBI_HANC)<<BLUE_CARD_BUFFER_TYPE_OFFSET)|((BufferId&0xFFF))|0)
#define BlueBuffer_Image_HANC(BufferId)				(((BLUE_CARDBUFFER_IMAGE_HANC)<<BLUE_CARD_BUFFER_TYPE_OFFSET)|((BufferId&0xFFF))|0)
#define BlueBuffer_HANC(BufferId)					(((BLUE_CARDBUFFER_HANC)<<BLUE_CARD_BUFFER_TYPE_OFFSET)|((BufferId&0xFFF))|0)


/* Buffer identifiers */
#define	BUFFER_ID_AUDIO_IN		(0)
#define	BUFFER_ID_AUDIO_OUT		(1)
#define	BUFFER_ID_VIDEO0		(2)
#define	BUFFER_ID_VIDEO1		(3)
#define	BUFFER_ID_VIDEO2		(4)
#define	BUFFER_ID_VIDEO3		(5)

/* AUDIO_OUTPUT_PROP/AUDIO_CHANNEL_ROUTING */
#define GET_ANALOG_AUDIO_LEFT_ROUTINGCHANNEL(value)				(value&0xFF)
#define GET_ANALOG_AUDIO_RIGHT_ROUTINGCHANNEL(value)			((value&0xFF00)>>8)
#define SET_ANALOG_AUDIO_ROUTINGCHANNEL(left,right)				(((right & 0xFF)<<8)|(left & 0xFF))
#define SET_AUDIO_OUTPUT_ROUTINGCHANNEL(output_type,src_channel_id,_output_channel_id) ((1<<31)|((output_type&3)<<29)|((_output_channel_id &0x3F)<<23)|((src_channel_id & 0x3F)<<16))
#define GET_AUDIO_OUTPUT_SRC_CHANNEL_ROUTING(value)				((value>>16) & 0x3F)
#define GET_AUDIO_OUTPUT_CHANNEL_ROUTING(value)					((value>>23) & 0x3F)
#define GET_AUDIO_OUTPUT_TYPE_ROUTING(value)					((value & 0x60000000)>>29)

/* AES_OUTPUT_ROUTING */
#define SET_AES_OUTPUT_ROUTING(OutputVideoChannel, AudioSrcChannel, AudioDstChannel)	(((OutputVideoChannel & 0xFF) << 16) | ((AudioDstChannel & 0xFF) << 8) | (AudioSrcChannel & 0xFF))
#define GET_AES_OUTPUT_ROUTING_STREAM(value)											((value >> 16) & 0xFF)
#define GET_AES_OUTPUT_ROUTING_DST_CHANNEL(value)										((value >> 8) & 0xFF)
#define GET_AES_OUTPUT_ROUTING_SRC_CHANNEL(value)										(value & 0xFF)

/* MUTE_AES_OUTPUT_CHANNEL */
#define SET_MUTE_AES_OUTPUT_CHANNEL(AudioDstChannel, Mute)	(((Mute & 0x1) << 31) | AudioDstChannel & 0xFF)

#define AUDIO_INPUT_SOURCE_SELECT_FLAG							(1<<16)	
#define AUDIO_INPUT_SOURCE_SELECT(SynchCount,AudioInputSource)	(AUDIO_INPUT_SOURCE_SELECT_FLAG|(SynchCount)|(AudioInputSource<<17))

#define EPOCH_WATCHDOG_TIMER_SET_MACRO(prop, value) (prop|(value &0xFFFF))
#define EPOCH_WATCHDOG_TIMER_QUERY_MACRO(prop)		(prop)
#define EPOCH_WATCHDOG_TIMER_GET_VALUE_MACRO(value)	(value&0xFFFF)

#define EPOCH_RS422_PORT_FLAG_SET_MACRO(portid,value)	((portid&0x3)|(value<<3))
#define EPOCH_RS422_PORT_FLAG_GET_FLAG_MACRO(value)		((value>>3)&0xFFFF)
#define EPOCH_RS422_PORT_FLAG_GET_PORTID_MACRO(value)	(value&0x3)

#define RS422_SERIALPORT_FLAG(timeout, port, RxFlushBuffer) (((unsigned long)(timeout)<<16)|(port & 0x3) | (RxFlushBuffer<<15))
#define RS422_SERIALPORT_FLAG2(timeout, port, RxFlushBuffer, RXIntWaitReturnOnAvailData) (((unsigned long)(timeout)<<16)|(port & 0x3) | (RxFlushBuffer<<15)|(RXIntWaitReturnOnAvailData<<14))

/* Epoch scaler not supported */
#define SET_EPOCH_SCALER_MODE(scaler_id,video_mode) ((scaler_id << 16) | video_mode)
#define GET_EPOCH_SCALER_MODE(value)                (value & 0xFFFF)
#define GET_EPOCH_SCALER_ID(value)                  ((value & 0xFFFF0000) >> 16)

/* use these macros for retreiving the temp and fan speed on epoch range of cards. */
#define EPOCH_CORE_TEMP(value)					(value & 0xFF)
#define EPOCH_BOARD_TEMP(value)					((value>>16) & 0xFF)
#define EPOCH_FAN_SPEED(value)					((value>>24) & 0xFF)

/*	Use these macro for doing the MR2 routing  on epoch range of cards. MR2 routing can be controlled using the property MR_ROUTING. */
#define EPOCH_SET_ROUTING(routing_src,routing_dest,data_link_type) ((routing_src & 0xFF) | ((routing_dest & 0xFF)<<8) | ((data_link_type&0xFFFF)<<16))
#define EPOCH_ROUTING_GET_SRC_DATA(value)		(value & 0xFF)
#define EPOCH_ROUTING_GET_DEST_DATA(value)		((value>>8) & 0xFF)
#define EPOCH_ROUTING_GET_LINK_TYPE_DATA(value)	((value>>16) & 0xFFFF)

/* MACROS for card property BYPASS_RELAY_ENABLE */
#define BLUE_ENABLE_BYPASS_RELAY(RelayNumber)       ((RelayNumber << 16) | 1)
#define BLUE_DISABLE_BYPASS_RELAY(RelayNumber)      ((RelayNumber << 16))
#define BLUE_GET_BYPASS_RELAY_SETTING(RelayNumber)  (RelayNumber << 16)
#define GPIO_TX_PORT_A	(1)
#define GPIO_TX_PORT_B	(2)

#define EPOCH_GPIO_TX(port,value)	(port<<16|value)    /* if want to set each of the GPO ports individually you should use this macro.
                                                           without the macro it would set both the GPO ports on the card */

#define VPEnableFieldCountTrigger		((BLUE_U64)1<<63)
#define VPTriggerGetFieldCount(value)	((BLUE_U64)value & 0xFFFFFFFF)

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

#define ANALOG_CHANNEL_0                        MONO_CHANNEL_9
#define ANALOG_CHANNEL_1                        MONO_CHANNEL_10
#define ANALOG_AUDIO_PAIR                       (ANALOG_CHANNEL_0 | ANALOG_CHANNEL_1)

#define STEREO_PAIR_1                           (MONO_CHANNEL_1 | MONO_CHANNEL_2)
#define STEREO_PAIR_2                           (MONO_CHANNEL_3 | MONO_CHANNEL_4)
#define STEREO_PAIR_3                           (MONO_CHANNEL_5 | MONO_CHANNEL_6)
#define STEREO_PAIR_4                           (MONO_CHANNEL_7 | MONO_CHANNEL_8)

#define IGNORE_SYNC_WAIT_TIMEOUT_VALUE          (0xFFFFFFFF)

#define AUDIO_INPUT_SOURCE_EMB      0
#define AUDIO_INPUT_SOURCE_AES      1

/*///////////////////////////////////////////////////////////////////////////////////
//H E L P E R   M A C R O S
///////////////////////////////////////////////////////////////////////////////////*/

/* the following macros are used with card property OVERRIDE_OUTPUT_VPID_DEFAULT */
#define OUTPUT_VPID_SET_ENABLED(value)								((value) |= 0x8000000000000000ULL )
#define OUTPUT_VPID_SET_DISABLED(value)								((value) &= ~(0x8000000000000000ULL))
#define OUTPUT_VPID_SET_BYTES(value, byte1, byte2, byte3, byte4)	(value = ((value & 0xFFFFFFFF00000000ULL) | ((((BLUE_U64)(byte4 & 0xFF)) << 24) | ((byte3 & 0xFF) << 16) | ((byte2 & 0xFF) << 8) | (byte1 & 0xFF))))
#define OUTPUT_VPID_SET_SDI_CONNECTOR(value, outputconnector)		(value = ((value & 0xFFFF0000FFFFFFFFULL) | ((outputconnector & 0xFFFFULL) << 32)))
#define OUTPUT_VPID_GET_ENABLED(value)								((value) & 0x8000000000000000ULL )
#define OUTPUT_VPID_GET_SDI_CONNECTOR(value)						((value >> 32) & 0xFFFFULL)
#define OUTPUT_VPID_GET_VPID_BYTE1(value)							(value & 0xFFLL)
#define OUTPUT_VPID_GET_VPID_BYTE2(value)							((value >> 8) & 0xFFULL)
#define OUTPUT_VPID_GET_VPID_BYTE3(value)							((value >> 16) & 0xFFULL)
#define OUTPUT_VPID_GET_VPID_BYTE4(value)							((value >> 24) & 0xFFULL)

/* the following macros are used with card property INTERLOCK_REFERENCE */
#define INTERLOCK_REFERENCE_GET_OUTPUT_ENABLED(value)		((value)		& 0x01)
#define INTERLOCK_REFERENCE_GET_INPUT_DETECTED(value)		((value >> 1)	& 0x01)
#define INTERLOCK_REFERENCE_GET_SLAVE_POSITION(value)		((value >> 2)	& 0x1F)

/* the following macros are used with card property CARD_FEATURE_STREAM_INFO */
#define CARD_FEATURE_GET_SDI_OUTPUT_STREAM_COUNT(value)		((value)		& 0xF)
#define CARD_FEATURE_GET_SDI_INPUT_STREAM_COUNT(value)		((value >> 4)	& 0xF)
#define CARD_FEATURE_GET_ASI_OUTPUT_STREAM_COUNT(value)		((value >> 8)	& 0xF)
#define CARD_FEATURE_GET_ASI_INPUT_STREAM_COUNT(value)		((value >> 12)	& 0xF)
#define CARD_FEATURE_GET_3G_SUPPORT(value)					((value >> 16)	& 0xF)

/* the following macros are used with card property CARD_FEATURE_CONNECTOR_INFO */
#define CARD_FEATURE_GET_SDI_OUTPUT_CONNECTOR_COUNT(value)    ((value)		& 0xF)
#define CARD_FEATURE_GET_SDI_INPUT_CONNECTOR_COUNT(value)     ((value >> 4)	& 0xF)
#define CARD_FEATURE_GET_AES_CONNECTOR_SUPPORT(value)         ((value >> 8)	& 0x1)
#define CARD_FEATURE_GET_RS422_CONNECTOR_SUPPORT(value)       ((value >> 9)	& 0x1)
#define CARD_FEATURE_GET_LTC_CONNECTOR_SUPPORT(value)         ((value >> 10)	& 0x1)
#define CARD_FEATURE_GET_GPIO_CONNECTOR_SUPPORT(value)        ((value >> 11)	& 0x1)
#define CARD_FEATURE_GET_HDMI_CONNECTOR_SUPPORT(value)        ((value >> 12)	& 0x1)
#define CARD_FEATURE_GET_HDMI_OUTPUT_CONNECTOR_SUPPORT(value) ((value >> 12)	& 0x1)
#define CARD_FEATURE_GET_HDMI_INPUT_CONNECTOR_SUPPORT(value)  ((value >> 13)	& 0x1)

/* the following macros are used with card property VIDEO_ONBOARD_KEYER */
#define VIDEO_ONBOARD_KEYER_GET_STATUS_ENABLED(value)					((value) & 0x1)
#define VIDEO_ONBOARD_KEYER_GET_STATUS_OVER_BLACK(value)				((value) & 0x2)
#define VIDEO_ONBOARD_KEYER_GET_STATUS_USE_INPUT_ANCILLARY(value)		((value) & 0x4)
#define VIDEO_ONBOARD_KEYER_GET_STATUS_DATA_IS_PREMULTIPLIED(value)		((value) & 0x8)
#define VIDEO_ONBOARD_KEYER_SET_STATUS_ENABLED(value)					(value |= 0x1)
#define VIDEO_ONBOARD_KEYER_SET_STATUS_DISABLED(value)					(value &= ~(0x1))
#define VIDEO_ONBOARD_KEYER_SET_STATUS_ENABLE_OVER_BLACK(value)			(value |= 0x2)
#define VIDEO_ONBOARD_KEYER_SET_STATUS_DISABLE_OVER_BLACK(value)		(value &= ~(0x2))
#define VIDEO_ONBOARD_KEYER_SET_STATUS_USE_INPUT_ANCILLARY(value)		(value |= 0x4)      /* only use this setting when keying over valid input (input must also match output video mode), includes HANC and VANC */
#define VIDEO_ONBOARD_KEYER_SET_STATUS_USE_OUTPUT_ANCILLARY(value)		(value &= ~(0x4))
#define VIDEO_ONBOARD_KEYER_SET_STATUS_DATA_IS_PREMULTIPLIED(value)		(value |= 0x8)
#define VIDEO_ONBOARD_KEYER_SET_STATUS_DATA_IS_NOT_PREMULTIPLIED(value)	(value &= ~(0x8))

/* the following macros are used with card property EPOCH_HANC_INPUT_FLAGS */
#define HANC_FLAGS_IS_ARRI_RECORD_FLAG_SET(value)	((value) & 0x1)

/* the following macros are used with card property EPOCH_RAW_VIDEO_INPUT_TYPE */
#define RAW_VIDEO_INPUT_TYPE_IS_10BIT		(0x01)
#define RAW_VIDEO_INPUT_TYPE_IS_12BIT		(0x02)
#define RAW_VIDEO_INPUT_TYPE_IS_WEISSCAM	(0x10)
#define RAW_VIDEO_INPUT_TYPE_IS_ARRI		(0x20)

/* the following macros are used with card property EPOCH_PCIE_CONFIG_INFO */
#define PCIE_CONFIG_INFO_GET_MAX_PAYLOAD_SIZE(value) ((value) & 0xFFFF)
#define PCIE_CONFIG_INFO_GET_MAX_READREQUEST_SIZE(value) ((value >> 16) & 0xFFFF)

/* the following macros are used with card property MISC_CONNECTOR_OUT */
#define MISC_CONNECTOR_OUT_SET_CONNECTOR_ID(value, ID)               (value = (ID & 0x0000FFFF) << 16)
#define MISC_CONNECTOR_OUT_SET_SIGNAL_TYPE_LTC(value)                (value = (value & 0xFFFF0000) | 0x0002)
#define MISC_CONNECTOR_OUT_SET_SIGNAL_TYPE_INTERLOCK(value)          (value = (value & 0xFFFF0000) | 0x0004)
#define MISC_CONNECTOR_OUT_SET_SIGNAL_TYPE_SPG(value)                (value = (value & 0xFFFF0000) | 0x0008)
#define MISC_CONNECTOR_OUT_IS_SIGNAL_TYPE_LTC(value)                 (value & 0x0002)
#define MISC_CONNECTOR_OUT_IS_SIGNAL_TYPE_INTERLOCK(value)           (value & 0x0004)
#define MISC_CONNECTOR_OUT_IS_SIGNAL_TYPE_SPG(value)                 (value & 0x0008)

/* the following macros are used with card property MISC_CONNECTOR_IN */
#define MISC_CONNECTOR_IN_SET_CONNECTOR_ID(value, ID)       (value = (ID & 0xFFFF) << 16)
#define MISC_CONNECTOR_IN_IS_SIGNAL_DETECTED(value)         (value & 0x0001)
#define MISC_CONNECTOR_IN_IS_SIGNAL_TYPE_LTC(value)         (value & 0x0002)
#define MISC_CONNECTOR_IN_IS_SIGNAL_TYPE_INTERLOCK(value)   (value & 0x0004)


typedef enum _EBlueCardProperty
{
	VIDEO_DUAL_LINK_OUTPUT =						0,	/**< Use this property to enable/diable cards dual link output property */
	VIDEO_DUAL_LINK_INPUT =							1,	/**< Use this property to enable/diable cards dual link input property */
	VIDEO_DUAL_LINK_OUTPUT_SIGNAL_FORMAT_TYPE =		2,	/**< Use this property to select signal format type that should be used
															when dual link output is enabled. Possible values this property can
															accept is defined in the enumerator EDualLinkSignalFormatType */
	VIDEO_DUAL_LINK_INPUT_SIGNAL_FORMAT_TYPE =		3,	/**< Use this property to select signal format type that should be used
															when dual link input is enabled. Possible values this property can
															accept is defined in the enumerator EDualLinkSignalFormatType */
	VIDEO_OUTPUT_SIGNAL_COLOR_SPACE =				4,	/**< Use this property to select color space of the signal when dual link output is set to
															use 4:4:4/4:4:4:4 signal format type. Possible values this property can
															accept is defined in the enumerator EConnectorSignalColorSpace */
	VIDEO_INPUT_SIGNAL_COLOR_SPACE =				5,	/**< Use this property to select color space of the signal when dual link input is set to
														   use 4:4:4/4:4:4:4 signal format type. Possible values this property can
														   accept is defined in the enumerator EConnectorSignalColorSpace */
	VIDEO_MEMORY_FORMAT =							6, /**< Use this property to ser the pixel format that should be used by
															 video output channels.  Possible values this property can
															 accept is defined in the enumerator EMemoryFormat*/
	VIDEO_MODE =									7,	/**< Use this property to set the video mode that should be used by
															video output channels.  Possible values this property can
															accept is defined in the enumerator EVideoMode*/
	VIDEO_UPDATE_TYPE =								8,	/**< Use this property to set the framestore update type that should be used by
															  video output channels.  Card can update video framestore at field/frame rate.
															  Possible values this property can accept is defined in the enumerator EUpdateMethod */
	VIDEO_ENGINE =									9,
	VIDEO_IMAGE_ORIENTATION =						10,	/**< Use this property to set the image orientation of the video output framestore.
															 This property must be set before frame is transferred to on card memory using
															 DMA transfer functions(system_buffer_write_async). It is recommended to use
															 vertical flipped image orientation only on RGB pixel formats.
															 Possible values this property can accept is defined in the enumerator EImageOrientation */
	VIDEO_USER_DEFINED_COLOR_MATRIX =				11,
	VIDEO_PREDEFINED_COLOR_MATRIX =					12,	/* EPreDefinedColorSpaceMatrix */
	VIDEO_RGB_DATA_RANGE =							13, /**< Use this property to set the data range of RGB pixel format, user can specify
															whether the RGB data is in either SMPTE or CGR range. Based on this information
															driver is decide which color matrix should be used.
															Possible values this property can accept is defined in the enumerator ERGBDataRange
															For SD cards this property will set the input and the output to the specified value.
															For Epoch/Create/SuperNova cards this property will only set the output to the specified value.
															For setting the input on Epoch/Create/SuperNova cards see EPOCH_VIDEO_INPUT_RGB_DATA_RANGE */
	VIDEO_KEY_OVER_BLACK =							14, /**< DEPRECATED */
	VIDEO_KEY_OVER_INPUT_SIGNAL =					15,	/**< DEPRECATED */
	VIDEO_SET_DOWN_CONVERTER_VIDEO_MODE =			16,	/**< DEPRECATED */
	VIDEO_LETTER_BOX =								17,	/**< DEPRECATED */
	VIDEO_PILLOR_BOX_LEFT =							18,	/**< DEPRECATED */
	VIDEO_PILLOR_BOX_RIGHT =						19,	/**< DEPRECATED */
	VIDEO_PILLOR_BOX_TOP =							20,	/**< DEPRECATED */
	VIDEO_PILLOR_BOX_BOTTOM =						21,	/**< DEPRECATED */
	VIDEO_SAFE_PICTURE =							22,	/**< DEPRECATED */
	VIDEO_SAFE_TITLE =								23,	/**< DEPRECATED */
	VIDEO_INPUT_SIGNAL_VIDEO_MODE =					24,	/**< Use this property to retreive the video input signal information on the
															default video input channel used by that SDK object.
															When calling SetCardProperty with a valid video mode on this property, the SDK
															will will use this video mode "Hint" if the card buffers are set up despite there being a valid
															input signal; the card buffers will be set up when calling one of these card properties:
															VIDEO_INPUT_MEMORY_FORMAT
															VIDEO_INPUT_UPDATE_TYPE
															VIDEO_INPUT_ENGINE
															Note: QueryCardProperty(VIDEO_INPUT_SIGNAL_VIDEO_MODE) will still return the actual video input signal */
	VIDEO_COLOR_MATRIX_MODE =						25,
	VIDEO_OUTPUT_MAIN_LUT =							26,	/**< DEPRECATED */
	VIDEO_OUTPUT_AUX_LUT =							27,	/**< DEPRECATED */
	VIDEO_LTC =										28,	/**< DEPRECATED; To retreive/ outputting LTC information you can use the HANC decoding and encoding functions. */
	VIDEO_GPIO =									29,
	VIDEO_PLAYBACK_FIFO_STATUS =					30, /**< This property can be used to retreive how many frames are bufferd in the video playback fifo. */
	RS422_RX_BUFFER_LENGTH =						31,
	RS422_RX_BUFFER_FLUSH =							32,
	VIDEO_INPUT_UPDATE_TYPE =						33,	/**< Use this property to set the framestore update type that should be used by
															 video input  channels.  Card can update video framestore at field/frame rate.
															 Possible values this property can accept is defined in the enumerator EUpdateMethod */
	VIDEO_INPUT_MEMORY_FORMAT =						34,	/**< Use this property to set the pixel format that should be used by
															   video input channels when it is capturing a frame from video input source.
															   Possible values this property can accept is defined in the enumerator EMemoryFormat*/
	VIDEO_GENLOCK_SIGNAL =							35,	/**< Use this property to retrieve video signal of the reference source that is used by the card.
															This can also be used to select the reference signal source that should be used.
															See application note AN004_Genlock.pdf for more information */
	AUDIO_OUTPUT_PROP =								36,	/**< this can be used to route PCM audio data onto respective audio output connectors. */
	AUDIO_CHANNEL_ROUTING =							AUDIO_OUTPUT_PROP,
	AUDIO_INPUT_PROP =								37,	/**< Use this property to select audio input source that should be used when doing an audio capture.
															Possible values this property can accept is defined in the enumerator Blue_Audio_Connector_Type. */
	VIDEO_ENABLE_LETTERBOX =						38,	/**< DEPRECATED */
	VIDEO_DUALLINK_OUTPUT_INVERT_KEY_COLOR =		39,	/**< this property is deprecated and no longer supported on epoch/create range of cards. */
	VIDEO_DUALLINK_OUTPUT_DEFAULT_KEY_COLOR =		40,	/**< this property is deprecated and no longer supported on epoch/create range of cards. */
	VIDEO_BLACKGENERATOR =							41,	/**< Use this property to control the black generator on the video output channel. */
	VIDEO_INPUTFRAMESTORE_IMAGE_ORIENTATION =		42,
	VIDEO_INPUT_SOURCE_SELECTION =					43, /**< The video input source that should be used by the SDK default video input channel
															can be configured using this property.
															Possible values this property can accept is defined in the enumerator EBlueConnectorIdentifier. */
	DEFAULT_VIDEO_OUTPUT_CHANNEL =					44,
	DEFAULT_VIDEO_INPUT_CHANNEL =					45,
	VIDEO_REFERENCE_SIGNAL_TIMING =					46,
	EMBEDEDDED_AUDIO_OUTPUT =						47,	/**< the embedded audio output property can be configured using this property.
																Possible values this property can accept is defined in the enumerator EBlueEmbAudioOutput. */
	EMBEDDED_AUDIO_OUTPUT =							EMBEDEDDED_AUDIO_OUTPUT,
	VIDEO_PLAYBACK_FIFO_FREE_STATUS =				48, /**< this will return the number of free buffer in the fifo.
															  If the video engine  is framestore this will give you the number of buffers that the framestore mode
															  can you use with that video output channel. */
	VIDEO_IMAGE_WIDTH =								49,	/**< selective output DMA: see application note AN008_SelectiveDMA.pdf for more details */
	VIDEO_IMAGE_HEIGHT =							50,	/**< selective output DMA: see application note AN008_SelectiveDMA.pdf for more details */
	VIDEO_SELECTIVE_OUTPUT_DMA_DST_PITCH =			VIDEO_IMAGE_WIDTH,	/* pitch (bytes per line) of destination buffer (card memory) */
	VIDEO_SELECTIVE_OUTPUT_DMA_SRC_LINES =			VIDEO_IMAGE_HEIGHT,	/* number of video lines to extract from source image (system memory)*/
	VIDEO_SCALER_MODE =								51,
	AVAIL_AUDIO_INPUT_SAMPLE_COUNT =				52,	/* DEPRECATED */
	VIDEO_PLAYBACK_FIFO_ENGINE_STATUS =				53,	/**< this will return the playback fifo status. The values returned by this property
																are defined in the enumerator BlueVideoFifoStatus. */
	VIDEO_CAPTURE_FIFO_ENGINE_STATUS =				54,	/**< this will return the capture  fifo status.
															The values returned by this property are defined in the enumerator BlueVideoFifoStatus. */
	VIDEO_2K_1556_PANSCAN =							55,	/**< DEPRECATED */
	VIDEO_OUTPUT_ENGINE =							56,	/**< Use this property to set the video engine of the video output channels.
															Possible values this property can accept is defined in the enumerator EEngineMode */
	VIDEO_INPUT_ENGINE =							57, /**< Use this property to set the video engine of the video input channels.
															Possible values this property can accept is defined in the enumerator EEngineMode */
	BYPASS_RELAY_A_ENABLE =							58, /**< use this property to control the bypass relay on SDI A output. */
	BYPASS_RELAY_B_ENABLE =							59,	/**< use this property to control the bypass relay on SDI B output. */
	VIDEO_PREMULTIPLIER =							60,
	VIDEO_PLAYBACK_START_TRIGGER_POINT =			61, /**< Using this property you can instruct the driver to start the
															 video playback fifo on a particular video output field count.
															 Normally video playback fifo is started on the next video interrupt after
															 the video_playback_start call. */
	GENLOCK_TIMING =								62,
	VIDEO_IMAGE_PITCH =								63,	/**< selective output DMA: see application note AN008_SelectiveDMA.pdf for more details */
	VIDEO_IMAGE_OFFSET =							64,	/**< currently not used; selective output DMA: see application note AN008_SelectiveDMA.pdf for more details */
	VIDEO_SELECTIVE_OUTPUT_DMA_SRC_PITCH =			VIDEO_IMAGE_PITCH,	/* pitch (bytes per line) of source buffer (system memory) */
	VIDEO_INPUT_IMAGE_WIDTH =						65,	/**< selective input DMA: see application note AN008_SelectiveDMA.pdf for more details */
	VIDEO_INPUT_IMAGE_HEIGHT =						66,	/**< selective input DMA: see application note AN008_SelectiveDMA.pdf for more details */
	VIDEO_INPUT_IMAGE_PITCH =						67,	/**< selective input DMA: see application note AN008_SelectiveDMA.pdf for more details */
	VIDEO_INPUT_IMAGE_OFFSET =						68,	/**< currently not used; selective input DMA: see application note AN008_SelectiveDMA.pdf for more details */
	VIDEO_SELECTIVE_INPUT_DMA_SRC_PITCH =			VIDEO_INPUT_IMAGE_WIDTH,	/* pitch (bytes per line) of source buffer (card memory) */
	VIDEO_SELECTIVE_INPUT_DMA_SRC_LINES =			VIDEO_INPUT_IMAGE_HEIGHT,	/* number of video lines to extract from source image (card memory) */
	VIDEO_SELECTIVE_INPUT_DMA_DST_PITCH =			VIDEO_INPUT_IMAGE_PITCH,	/* pitch (bytes per line) of destination buffer (system memory) */
	TIMECODE_RP188 =								69,	/**< this property is deprecated and no longer supported on epoch/create range of cards.*/
	BOARD_TEMPERATURE =								70, /**<This property can be used to retreive the Board temperature, core temperature and
															   RPM of the Fan on epoch/create range of cards.<br/>
															   Use the macro's EPOCH_CORE_TEMP ,EPOCH_BOARD_TEMP and EPOCH_FAN_SPEED
															   to retireive the respective values from the property. */
	MR2_ROUTING =									71,	/**< Use this property to control the MR2 functionlity on epoch range of cards.
															Use the following macro with this property.<br/>
															1) EPOCH_SET_ROUTING --> for setting the source, destination and link type of the routing connection,
															2) EPOCH_ROUTING_GET_SRC_DATA --> for getting the routing source.
															The possible source and destination elements supported by the routing matrix are defined in the
															enumerator EEpochRoutingElements. */
	SAVEAS_POWERUP_SETTINGS =						72,	/**< DEPRECATED */
	VIDEO_CAPTURE_AVAIL_BUFFER_COUNT =				73, /**< This property will return the number of captured frame avail in the fifo at present.
															   If the video engine  is framestore this will give you the number of buffers that the framestore mode
															   can you use with that video input channel */
	EPOCH_APP_WATCHDOG_TIMER =						74,/**< Use this property to control the application  watchdog timer functionality.
															Possible values this property can accept is defined in the enumerator enum_blue_app_watchdog_timer_prop. */
	EPOCH_RESET_VIDEO_INPUT_FIELDCOUNT =			75, /**< Use this property to reset the field count on both the
															 video channels of the card. You can pass the value that
															 should be used as starting fieldcount after the reset.
															 This property can be used to keep track  sync between left and right signal
															 when you are capturing in stereoscopic mode. */
	EPOCH_RS422_PORT_FLAGS =						76,	/**< Use this property to set the master/slave property of the RS422 ports.
															Possible values this property can accept is defined in the enumerator enum_blue_rs422_port_flags. */
	EPOCH_DVB_ASI_INPUT_TIMEOUT =					77,	/**< Current DVB ASI input firmware does not support this property in hardware,
															this is a future addition.
															Use this property to set the timeout  of the DVB ASI input stream.
															timeout is specified in  milliseconds.If hardware did not get the required no of
															packets( specified using EPOCH_DVB_ASI_INPUT_LATENCY_PACKET_COUNT)
															within the period specified in the timeout, hardware would generate a video input interrupt
															and it would be safe to read the dvb asi packet from the card. */
	EPOCH_DVB_ASI_INPUT_PACKING_FORMAT =			78, /**< Use this property to specify the packing method that should be used
															 when capturing DVB ASI packets.
															 The possible packing methods are defined in the enumerator enum_blue_dvb_asi_packing_format. */
	EPOCH_DVB_ASI_INPUT_LATENCY_PACKET_COUNT =		79, /**< Use this property to set  how many asi packets should be captured by the card , before it
																notifies the driver of available data using video input interrupt. */
	VIDEO_PLAYBACK_FIFO_CURRENT_FRAME_UNIQUEID =	80, /**< This property can be used to query the current unique id of
															 the frame that is being displayed currently by the video output channel. This
															 property is only usefull in the context of video fifo.<br/>
															 You get a uniqueid when you present a frame using video_playback_present function.
															 Alternative ways to get this information are <br/>
															 1) using blue_wait_video_sync_async , the member current_display_frame_uniqueid contains the same information<br/>
															 2) using wait_video_output_sync function on epoch cards, if
															 the flag UPD_FMT_FLAG_RETURN_CURRENT_UNIQUEID is appended with
															 either UPD_FMT_FRAME or UPD_FMT_FIELD , the return value of
															 the function wait_video_output_sync woukd contain the current display frames uniqueid. */

	EPOCH_DVB_ASI_INPUT_GET_PACKET_SIZE =			81,	/**< use this property to get the size of each asi transport stream packet (whether it is 188 or 204.*/
	EPOCH_DVB_ASI_INPUT_PACKET_COUNT =				82,	/**< this property would give you the number of packets captured during the last
															  interrupt time frame. For ASI interrupt is generated if
															  hardware captured the requested number of packets or it hit the timeout value */
	EPOCH_DVB_ASI_INPUT_LIVE_PACKET_COUNT =			83,	/**< this property would give you the number of packets that
																is being captured during the current interrupt time frame.
																For ASI interrupt is generated when has hardware captured the
																requested number of packets specified using
																EPOCH_DVB_ASI_INPUT_LATENCY_PACKET_COUNT property. */
	EPOCH_DVB_ASI_INPUT_AVAIL_PACKETS_IN_FIFO =		84,	/**< This property would return the number of ASI packets that has been captured into card memory, that
															   can be retreived. This property is only valid when the video input
															   channel is being used in FIFO modes. */
	EPOCH_ROUTING_SOURCE_VIDEO_MODE =				VIDEO_SCALER_MODE,	/**< Use this property to change the video mode that scaler should be set to.
																		USe the macro SET_EPOCH_SCALER_MODE when using this property, as this macro
																		would allow you to select which one of the scaler blocks video mode should be updated. */
	EPOCH_AVAIL_VIDEO_SCALER_COUNT =				85,	/**< DEPRECATED */
	EPOCH_ENUM_AVAIL_VIDEO_SCALERS_ID =				86,	/**< DEPRECATED */
	EPOCH_ALLOCATE_VIDEO_SCALER =					87,	/**< DEPRECATED */
	EPOCH_RELEASE_VIDEO_SCALER =					88,	/**< DEPRECATED */
	EPOCH_DMA_CARDMEMORY_PITCH =					89,	/**< DEPRECATED */
	EPOCH_OUTPUT_CHANNEL_AV_OFFSET =				90,	/**< DEPRECATED */
	EPOCH_SCALER_CHANNEL_MUX_MODE =					91,	/**< DEPRECATED */
	EPOCH_INPUT_CHANNEL_AV_OFFSET =					92,	/**< DEPRECATED */
	EPOCH_AUDIOOUTPUT_MANUAL_UCZV_GENERATION =		93,	/* ASI firmware only */
	EPOCH_SAMPLE_RATE_CONVERTER_BYPASS =			94,		/** bypasses the sample rate converter for AES audio; only turn on for Dolby-E support
															*	pass in a flag to signal which audio stereo pair should be bypassed:
															*	bit 0:	AES channels 0 and 1
															*	bit 1:	AES channels 2 and 3
															*	bit 2:	AES channels 4 and 5
															*	bit 3:	AES channels 6 and 7
															*	For example: bypass the sample rate converter for channels 0 to 3: flag = 0x3;	*/
	EPOCH_GET_PRODUCT_ID =							95,	/* returns the enum for the firmware type EEpochFirmwareProductID */
	EPOCH_GENLOCK_IS_LOCKED =						96,
	EPOCH_DVB_ASI_OUTPUT_PACKET_COUNT =				97,		/* ASI firmware only */
	EPOCH_DVB_ASI_OUTPUT_BIT_RATE =					98,		/* ASI firmware only */
	EPOCH_DVB_ASI_DUPLICATE_OUTPUT_A =				99,		/* ASI firmware only */
	EPOCH_DVB_ASI_DUPLICATE_OUTPUT_B =				100,	/* ASI firmware only */
	EPOCH_SCALER_HORIZONTAL_FLIP =					101,	/* see SideBySide_3D sample application */
	EPOCH_CONNECTOR_DIRECTION =						102,	/* depricated / not supported */
	EPOCH_AUDIOOUTPUT_VALIDITY_BITS =				103,	/* ASI firmware only */
	EPOCH_SIZEOF_DRIVER_ALLOCATED_MEMORY =			104,	/**< DEPRECATED */
	INVALID_VIDEO_MODE_FLAG =						105,	/* returns the enum for VID_FMT_INVALID that this SDK/Driver was compiled with;
																it changed between 5.9.x.x and 5.10.x.x driver branch and has to be handled differently for
																each driver if the application wants to use the VID_FMT_INVALID flag and support both driver branches */
	EPOCH_VIDEO_INPUT_VPID =						106,	/* returns the VPID for the current video input signal. Input value is of type EBlueConnectorIdentifier */
	EPOCH_LOW_LATENCY_DMA =							107,	/**< DEPRECATED; use new feature EPOCH_SUBFIELD_INPUT_INTERRUPTS instead */
	EPOCH_VIDEO_INPUT_RGB_DATA_RANGE =				108,
	EPOCH_DVB_ASI_OUTPUT_PACKET_SIZE =				109,	/* firmware supports either 188 or 204 bytes per ASI packet; set to either
																enum_blue_dvb_asi_packet_size_188_bytes or
																enum_blue_dvb_asi_packet_size_204_bytes */
	EPOCH_SUBFIELD_INPUT_INTERRUPTS =				110,	/* similar to the EPOCH_LOW_LATENCY_DMA card feature, but this doesn't influence the DMA;
																it simply adds interrupts between the frame/field interrupts that trigger when a corresponding
																video chunk has been captured required minimum driver: 5.10.1.8*/
	EPOCH_AUDIOOUTPUT_METADATA_SETTINGS =			111,	/* Use the EAudioMetaDataSettings enumerator to change the audio output metadata settings */
	EPOCH_HD_SDI_TRANSPORT =						112,	/* output only: available modes are defined in the enum EHdSdiTransport; for inputs see EPOCH_HD_SDI_TRANSPORT_INPUT */
	CARD_FEATURE_STREAM_INFO =						113,	/* only supported from driver 5.10.2.x; info on how many in/out SDI/ASI streams are supported */
	CARD_FEATURE_CONNECTOR_INFO =					114,	/* only supported from driver 5.10.2.x; info on which connectors are supported: SDI in/out, AES, RS422, LTC, GPIO */
	EPOCH_HANC_INPUT_FLAGS =						115,	/* this property can be queried to test flags being set in the HANC space (e.g. HANC_FLAGS_IS_ARRI_RECORD_FLAG_SET) */
	EPOCH_INPUT_VITC =								116,	/* this property retrieves the current input VITC timecode; set .vt = VT_UI8 as this is a 64bit value; */
	EPOCH_RAW_VIDEO_INPUT_TYPE =					117,	/* specifies if the raw/bayer input is ARRI 10/12 bit or Weisscam; set to 0 to revert back to normal SDI mode */
	EPOCH_PCIE_CONFIG_INFO =						118,	/* only supported from driver 5.10.2.x; provides info on PCIE maximum payload size and maximum read request siize */
	EPOCH_4K_QUADLINK_CHANNEL =						119,	/* use this property to set the 4K quadrant number for the current channel in 4K output mode; quadrant numbers are 1 - 4 */
	EXTERNAL_LTC_SOURCE_SELECTION =					120,	/* use the enum EBlueExternalLtcSource to set the input source for the external LTC */
	EPOCH_HD_SDI_TRANSPORT_INPUT =					121,	/* can only be queried; return values are defined in the enum EHdSdiTransport. Input value is of type EBlueConnectorIdentifier */
	CARD_CONNECTED_VIA_TB =							122,	/* MAC only: use this to check if the Card is connected via ThunderBolt */
	INTERLOCK_REFERENCE =							123,	/* this feature is only supported on Epoch Neutron cards; check application note AN004_Genlock.pdf for more information */
	VIDEO_ONBOARD_KEYER =							124,	/* this property is currently only supported by Epoch Neutron cards; use the VIDEO_ONBOARD_KEYER_GET_STATUS macros for this property*/
	EPOCH_OUTPUT_VITC_MANUAL_CONTROL =				125,	/* Epoch Neutron only: this property enables the feature to allow output of a custom VITC timecode on a field by field basis (low frame rates only); for high frame rates the conventional way (using the HANC buffer) must be used */
	EPOCH_OUTPUT_VITC =								126,	/* Epoch Neutron only: this property sets the custom VITC timecode (64 bit value) on a field by field basis (for low frame rates only); set .vt = VT_UI8 as this is a 64bit value; */
	EPOCH_INPUT_VITC_SOURCE =						127,	/* this property selects the source for the card property EPOCH_INPUT_VITC for SD video modes; in SD video modes the VITC source can be either
																from VBI space or from RP188 packets; the default (value = 0) is set to RP188; setting this to 1 will select VBI space as the source for EPOCH_INPUT_VITC; set .vt = VT_UI4 */
	TWO_SAMPLE_INTERLEAVE_OUTPUT =					128,	/* enables two sample interleave mode for 4K video modes using two output channels; options are: 0 = turn feature off, 1 = turn feature on */
	TWO_SAMPLE_INTERLEAVE_INPUT =					129,	/* enables two sample interleave mode for 4K video modes using two input channels; options are: 0 = turn feature off, 1 = turn feature on  */
	BTC_TIMER =										130,	/* BTC: Coordinated Bluefish Time; this timer has microsecond granularity and is started/reset when the driver starts; set .vt = VT_UI8 as this is a 64bit value; */
	BFLOCK_SIGNAL_ENABLE =							131,	/* S+ cards can generate a proprietary lock signal on the S+ connector (connector 0); options are 0 = turn off signal (connector 0 will be copy of SDI A output); 1 = turn on lock signal output; set .vt = VT_UI4 */
	AES_OUTPUT_ROUTING =							132,	/* set the stream source and source channels for the AES output; .vt = VT_UI4 */
	MUTE_AES_OUTPUT_CHANNEL =						133,	/* mute any of the AES output channels (0..7); to enable/disable mute use the SET_MUTE_AES_OUTPUT_CHANNEL macro; to query an AES output channels mute status
																set VT.ulVal to the AES output channel number (0..7) then call QueryCardProperty(); the return value will be 1 = muted or 0 = enabled; set .vt to VT_UI4	*/
	FORCE_SD_VBI_OUTPUT_BUFFER_TO_V210 =			134,	/* this card property forces the VBI buffer to V210 memory format in SD video modes (default for HD video modes) so that it can handle 10 bit VANC packets.
																set 1 = force to V210 or 0 = follow video memory fomat (default); set .vt to VT_UI4; when changing this property the video output mode and video output engine need to be set again manually! */
	EMBEDDED_AUDIO_INPUT_INFO =						135,	/*	this card property returns info on how which embedded audio input channels are available (channel mask for channels 1 - 16 in lower 16 bits).
																it also returns the data payload for each channel (1 - 16) in the upper 16 bits (0 = embedded audio, 1 = other (e.g. Dolby Digital)) */
	OVERRIDE_OUTPUT_VPID_DEFAULT =					136,	/*	this card property should only be used for debugging purposes if the default VPID needs to be changed; it will override the output VPID that is set up by default depending on the video mode, pixel format and signal format type.
																this property takes a 64 bit value (set .vt to VT_UI8) and is defined as follows (there are helper MACROS defined in the MACROS section at the end of this header file):
																7...0: Byte 1 of VPID
																15...8: Byte 2 of VPID
																23..16: Byte 3 of VPID
																31..24: Byte 4 of VPID
																47..32: SDI output connector (EBlueConnectorIdentifier)
																62..48: reserved (set to 0)
																63: enable/disable VPID output (0 = disabled, 1 = enabled) */
    FORCE_SD_VBI_INPUT_BUFFER_TO_V210 =             137,	/* this card property forces the VBI input buffer to V210 memory format in SD video modes (default for HD video modes) so that it can handle 10 bit VANC packets.
                                                               set 1 = force to V210 or 0 = follow video memory fomat (default); set .vt to VT_UI4; when changing this property the video input engine needs to be set again manually! */
    BYPASS_RELAY_ENABLE =                           138,    /* Enable/Disable the bypass relays; use MACROs BLUE_SET_ENABLE_BYPASS_RELAY(), BLUE_SET_DISABLE_BYPASS_RELAY() and BLUE_GET_BYPASS_RELAY_SETTING() to initialise the value (.vt = VT_UI4) */
    VIDEO_MODE_EXT_OUTPUT =                         139,    /* Query or set the video mode for the output channel using the _EVideoModeExt enums. */
    VIDEO_MODE_EXT_INPUT =                          140,    /* Query the current video mode on the input channel using the _EVideoModeExt enums. In case of no valid video mode present the input FIFOs can be set up for an expected video mode by setting this card property*/
    IS_VIDEO_MODE_EXT_SUPPORTED_OUTPUT =            141,
    IS_VIDEO_MODE_EXT_SUPPORTED_INPUT =             142,
    MISC_CONNECTOR_OUT =                            143,    /* KRONOS and above: 32 bit value, query and set. Use MACROS MISC_CONNECTOR_OUT_***; supported connectors to set/query: BLUE_CONNECTOR_REF_OUT, BLUE_CONNECTOR_INTERLOCK_OUT */
    MISC_CONNECTOR_IN =                             144,    /* KRONOS and above: 32 bit value, query only. Use MACROS MISC_CONNECTOR_IN_***; supported connectors to query: BLUE_CONNECTOR_REF_IN, BLUE_CONNECTOR_INTERLOCK_IN */
 	SUB_IMAGE_MAPPING =                             145,    /* Valid on input and output, but only has an effect on 4k/UHD and larger buffers.   Use ESubImageMapping enum to specify value; default value is IMAGE_MAPPING_TWO_SAMPLE_INTERLEAVE. set to IMAGE_MAPPING_SQUARE_DIVISION to specify that the 4K/UHD buffers handled by this channel are in SD (Square Division) mode */


	VIDEO_CARDPROPERTY_INVALID =					1000
}EBlueCardProperty;

typedef enum _ESubImageMapping
{
    IMAGE_MAPPING_INVALID =                 0,
    IMAGE_MAPPING_TWO_SAMPLE_INTERLEAVE =   1,
    IMAGE_MAPPING_SQUARE_DIVISION =         2,
}ESubImageMapping;

typedef enum _EHdSdiTransport
{
    HD_SDI_TRANSPORT_INVALID =      0,      /* Invalid input signal */
	HD_SDI_TRANSPORT_1_5G =         0x1,	/* HD as 1.5G */
	HD_SDI_TRANSPORT_3G_LEVEL_A =   0x2,	/* 3G Level A */
	HD_SDI_TRANSPORT_3G_LEVEL_B =   0x3,	/* 3G Level B */
}EHdSdiTransport;

typedef enum _EAudioMetaDataSettings
{
	AUDIO_METADATA_KEEP_ALIVE = 0x1		/*	When setting this bit for the EPOCH_AUDIOOUTPUT_METADATA_SETTINGS card property the audio meta data (like RP188 timecode
										will still be played out even after stopping audio playback; this is a static settings and only needs to be set once;
										it is channel based and can be changed for all output channels independently */
}EAudioMetaDataSettings;

/* This enumerator is still supported, but deprecated */
/* Please use _EVideoModeExt instead and card properties VIDEO_MODE_EXT_OUTPUT and VIDEO_MODE_EXT_INPUT */
typedef enum _EVideoMode
{
	VID_FMT_PAL =				0,
	VID_FMT_NTSC =				1,
	VID_FMT_576I_5000 =			VID_FMT_PAL,
	VID_FMT_486I_5994 =			VID_FMT_NTSC,
	VID_FMT_720P_5994 =			2,
	VID_FMT_720P_6000 =			3,
	VID_FMT_1080PSF_2397 =		4,
	VID_FMT_1080PSF_2400 =		5,
	VID_FMT_1080P_2397 =		6,
	VID_FMT_1080P_2400 =		7,
	VID_FMT_1080I_5000 =		8,
	VID_FMT_1080I_5994 =		9,
	VID_FMT_1080I_6000 =		10,
	VID_FMT_1080P_2500 =		11,
	VID_FMT_1080P_2997 =		12,
	VID_FMT_1080P_3000 =		13,
	VID_FMT_HSDL_1498 =			14,
	VID_FMT_HSDL_1500 =			15,
	VID_FMT_720P_5000 =			16,
	VID_FMT_720P_2398 =			17,
	VID_FMT_720P_2400 =			18,
	VID_FMT_2048_1080PSF_2397 =	19,
	VID_FMT_2048_1080PSF_2400 =	20,
	VID_FMT_2048_1080P_2397 =	21,
	VID_FMT_2048_1080P_2400 =	22,
	VID_FMT_1080PSF_2500 =		23,
	VID_FMT_1080PSF_2997 =		24,
	VID_FMT_1080PSF_3000 =		25,
	VID_FMT_1080P_5000 =		26,
	VID_FMT_1080P_5994 =		27,
	VID_FMT_1080P_6000 =		28,
	VID_FMT_720P_2500 =			29,
	VID_FMT_720P_2997 =			30,
	VID_FMT_720P_3000 =			31,
	VID_FMT_DVB_ASI =			32,
	VID_FMT_2048_1080PSF_2500 =	33,
	VID_FMT_2048_1080PSF_2997 =	34,
	VID_FMT_2048_1080PSF_3000 =	35,
	VID_FMT_2048_1080P_2500 =	36,
	VID_FMT_2048_1080P_2997 =	37,
	VID_FMT_2048_1080P_3000 =	38,
	VID_FMT_2048_1080P_5000 =	39,
	VID_FMT_2048_1080P_5994 =	40,
	VID_FMT_2048_1080P_6000 =	41,
	VID_FMT_1080P_4800 =		42,
	VID_FMT_2048_1080P_4800 =	43,
    VID_FMT_1080P_4795 =        44,
    VID_FMT_2048_1080P_4795 =   45,

	VID_FMT_INVALID =			46
} EVideoMode;

typedef enum _EVideoModeExt
{
    VID_FMT_EXT_INVALID =           1024,
    VID_FMT_EXT_CUSTOM =            1025,
    VID_FMT_EXT_DVB_ASI =           1026,
    VID_FMT_EXT_576I_5000 =         1027,
    VID_FMT_EXT_486I_5994 =         1028,
    VID_FMT_EXT_720P_2398 =         1029,
    VID_FMT_EXT_720P_2400 =         1030,
    VID_FMT_EXT_720P_2500 =         1031,
    VID_FMT_EXT_720P_2997 =         1032,
    VID_FMT_EXT_720P_3000 =         1033,
    VID_FMT_EXT_720P_4795 =         1034,
    VID_FMT_EXT_720P_4800 =         1035,
    VID_FMT_EXT_720P_5000 =         1036,
    VID_FMT_EXT_720P_5994 =         1037,
    VID_FMT_EXT_720P_6000 =         1038,
    VID_FMT_EXT_1080I_5000 =        1039,
    VID_FMT_EXT_1080I_5994 =        1040,
    VID_FMT_EXT_1080I_6000 =        1041,
    VID_FMT_EXT_1080PSF_2398 =      1042,
    VID_FMT_EXT_1080PSF_2400 =      1043,
    VID_FMT_EXT_1080PSF_2500 =      1044,
    VID_FMT_EXT_1080PSF_2997 =      1045,
    VID_FMT_EXT_1080PSF_3000 =      1046,
    VID_FMT_EXT_1080P_2398 =        1047,
    VID_FMT_EXT_1080P_2400 =        1048,
    VID_FMT_EXT_1080P_2500 =        1049,
    VID_FMT_EXT_1080P_2997 =        1050,
    VID_FMT_EXT_1080P_3000 =        1051,
    VID_FMT_EXT_1080P_4795 =        1052,
    VID_FMT_EXT_1080P_4800 =        1053,
    VID_FMT_EXT_1080P_5000 =        1054,
    VID_FMT_EXT_1080P_5994 =        1055,
    VID_FMT_EXT_1080P_6000 =        1056,
    VID_FMT_EXT_2K_1080PSF_2398 =   1057,
    VID_FMT_EXT_2K_1080PSF_2400 =   1058,
    VID_FMT_EXT_2K_1080PSF_2500 =   1059,
    VID_FMT_EXT_2K_1080PSF_2997 =   1060,
    VID_FMT_EXT_2K_1080PSF_3000 =   1061,
    VID_FMT_EXT_2K_1080P_2398 =     1062,
    VID_FMT_EXT_2K_1080P_2400 =     1063,
    VID_FMT_EXT_2K_1080P_2500 =     1064,
    VID_FMT_EXT_2K_1080P_2997 =     1065,
    VID_FMT_EXT_2K_1080P_3000 =     1066,
    VID_FMT_EXT_2K_1080P_4795 =     1067,
    VID_FMT_EXT_2K_1080P_4800 =     1068,
    VID_FMT_EXT_2K_1080P_5000 =     1069,
    VID_FMT_EXT_2K_1080P_5994 =     1070,
    VID_FMT_EXT_2K_1080P_6000 =     1071,
    VID_FMT_EXT_2K_1556I_1499 =     1072,
    VID_FMT_EXT_2K_1556I_1500 =     1073,
    VID_FMT_EXT_2160P_2398 =        1074,
    VID_FMT_EXT_2160P_2400 =        1075,
    VID_FMT_EXT_2160P_2500 =        1076,
    VID_FMT_EXT_2160P_2997 =        1077,
    VID_FMT_EXT_2160P_3000 =        1078,
    VID_FMT_EXT_2160P_4795 =        1079,
    VID_FMT_EXT_2160P_4800 =        1080,
    VID_FMT_EXT_2160P_5000 =        1081,
    VID_FMT_EXT_2160P_5994 =        1082,
    VID_FMT_EXT_2160P_6000 =        1083,
    VID_FMT_EXT_4K_2160P_2398 =     1084,
    VID_FMT_EXT_4K_2160P_2400 =     1085,
    VID_FMT_EXT_4K_2160P_2500 =     1086,
    VID_FMT_EXT_4K_2160P_2997 =     1087,
    VID_FMT_EXT_4K_2160P_3000 =     1088,
    VID_FMT_EXT_4K_2160P_4795 =     1089,
    VID_FMT_EXT_4K_2160P_4800 =     1090,
    VID_FMT_EXT_4K_2160P_5000 =     1091,
    VID_FMT_EXT_4K_2160P_5994 =     1092,
    VID_FMT_EXT_4K_2160P_6000 =     1093,
    VID_FMT_EXT_4320P_2398 =        1094,
    VID_FMT_EXT_4320P_2400 =        1095,
    VID_FMT_EXT_4320P_2500 =        1096,
    VID_FMT_EXT_4320P_2997 =        1097,
    VID_FMT_EXT_4320P_3000 =        1098,
    VID_FMT_EXT_4320P_4795 =        1099,
    VID_FMT_EXT_4320P_4800 =        1100,
    VID_FMT_EXT_4320P_5000 =        1101,
    VID_FMT_EXT_4320P_5994 =        1102,
    VID_FMT_EXT_4320P_6000 =        1103,
    VID_FMT_EXT_8K_4320P_2398 =     1104,
    VID_FMT_EXT_8K_4320P_2400 =     1105,
    VID_FMT_EXT_8K_4320P_2500 =     1106,
    VID_FMT_EXT_8K_4320P_2997 =     1107,
    VID_FMT_EXT_8K_4320P_3000 =     1108,
    VID_FMT_EXT_8K_4320P_4795 =     1109,
    VID_FMT_EXT_8K_4320P_4800 =     1110,
    VID_FMT_EXT_8K_4320P_5000 =     1111,
    VID_FMT_EXT_8K_4320P_5994 =     1112,
    VID_FMT_EXT_8K_4320P_6000 =     1113,
    VID_FMT_EXT_LAST_ENTRY_V1 =     VID_FMT_EXT_8K_4320P_6000,

    /* aliases */
    VID_FMT_EXT_PAL =               VID_FMT_EXT_576I_5000,
    VID_FMT_EXT_NTSC =              VID_FMT_EXT_486I_5994,
    VID_FMT_EXT_HSDL_1499 =         VID_FMT_EXT_2K_1556I_1499,
    VID_FMT_EXT_HSDL_1500 =         VID_FMT_EXT_2K_1556I_1500,

}EVideoModeExt;

typedef enum _EBFLockSignalType
{
	BFLOCK_SIGNAL_UNKNOWN =	0x1000,
	BFLOCK_SIGNAL_2398 =	0x1001,
	BFLOCK_SIGNAL_2400 =	0x1002,
	BFLOCK_SIGNAL_2500 =	0x1003,
	BFLOCK_SIGNAL_2997 =	0x1004,
	BFLOCK_SIGNAL_3000 =	0x1005,
	BFLOCK_SIGNAL_4795 =	0x1006,
	BFLOCK_SIGNAL_4800 =	0x1007,
	BFLOCK_SIGNAL_5000 =	0x1008,
	BFLOCK_SIGNAL_5994 =	0x1009,
	BFLOCK_SIGNAL_6000 =	0x100A,
}EBFLockSignalType;

typedef enum _EMemoryFormat
{
	MEM_FMT_ARGB =					0,
	MEM_FMT_BV10 =					1,	/* not supported */
	MEM_FMT_BV8 =					2,
	MEM_FMT_YUVS =					MEM_FMT_BV8,
	MEM_FMT_V210 =					3,
	MEM_FMT_RGBA =					4,
	MEM_FMT_CINEON_LITTLE_ENDIAN =	5,
	MEM_FMT_ARGB_PC =				6,
	MEM_FMT_BGRA =					MEM_FMT_ARGB_PC,
	MEM_FMT_CINEON =				7,
	MEM_FMT_2VUY =					8,
	MEM_FMT_BGR =					9,
	MEM_FMT_BGR_16_16_16 =			10,
	MEM_FMT_BGR_48 =				MEM_FMT_BGR_16_16_16,
	MEM_FMT_BGRA_16_16_16_16 =		11,
	MEM_FMT_BGRA_64 =				MEM_FMT_BGRA_16_16_16_16,
	MEM_FMT_VUYA_4444 =				12,
	MEM_FMT_V216 =					13,
	MEM_FMT_Y210 =					14,
	MEM_FMT_Y216 =					15,
	MEM_FMT_RGB =					16,
	MEM_FMT_YUV_ALPHA =				17,
	MEM_FMT_RGB_16_16_16 =			18,
	MEM_FMT_RGB_48 =				MEM_FMT_RGB_16_16_16,
	MEM_FMT_RGBA_16_16_16_16 =		19,
	MEM_FMT_RGBA_64 =				MEM_FMT_RGBA_16_16_16_16,
	MEM_FMT_YCA8 =					20,
	MEM_FMT_CYA8 =					21,
	MEM_FMT_YUV_ALPHA_10 =			22,
	MEM_FMT_YCA10 =					23,
	MEM_FMT_CYA10 =					24,
	MEM_FMT_YAC10 =					25,
	MEM_FMT_CAY10 =					26,

	MEM_FMT_INVALID =				27
} EMemoryFormat;

typedef enum _EUpdateMethod
{
	UPD_FMT_FIELD =							0,
	UPD_FMT_FRAME =							1,
	UPD_FMT_FRAME_DISPLAY_FIELD1 =			2,
	UPD_FMT_FRAME_DISPLAY_FIELD2 =			3,
	UPD_FMT_INVALID =						4,
	UPD_FMT_FLAG_RETURN_CURRENT_UNIQUEID =	0x80000000,	/*	if this flag is used on epoch cards, function would
															return the unique id of the current frame as the return value. */
} EUpdateMethod;

typedef enum _EResoFormat
{
	RES_FMT_NORMAL = 0,
	RES_FMT_HALF,
	RES_FMT_INVALID
} EResoFormat;

typedef enum _EEngineMode
{
	VIDEO_ENGINE_FRAMESTORE =	0,	/* Low latency mode for capture and playback; buffer cycling must be done by the user */
	VIDEO_ENGINE_PLAYBACK =		1,	/* deprecated; use VIDEO_ENGINE_DUPLEX instead */
	VIDEO_ENGINE_CAPTURE =		2,	/* FIFO mode for capturing Video/VANC/HANC and have driver do automatic playthrough on the output.
										output channel cannot be used for anything else */
	VIDEO_ENGINE_PAGEFLIP =		3,	/* deprecated/not supported */
	VIDEO_ENGINE_DUPLEX =		4,	/* FIFO mode for either capture or playback; capture and playback side can be used independently at the same time*/
	VIDEO_ENGINE_INVALID =		5
} EEngineMode;

typedef enum
{
	BLUE_FIFO_CLOSED =			0,	/**< Fifo has not been initialized*/
	BLUE_FIFO_STARTING =		1,	/**< Fifo is starting */
	BLUE_FIFO_RUNNING =			2,	/**< Fifo is running */
	BLUE_FIFO_STOPPING =		3,	/**< Fifo is in the process of stopping */
	BLUE_FIFO_PASSIVE =			5,	/**< Fifo is currently stopped or not active*/
	BLUE_FIFO_STATUS_INVALID =	10
}BlueVideoFifoStatus;

typedef enum
{
	BLUE_FIFO_NULL_ATTRIBUTE =		0x0,
	BLUE_FIFO_ECHOPORT_ENABLED =	0x1,
	BLUE_FIFO_STEPMODE =			0x2,
	BLUE_FIFO_LOOPMODE =			0x4
}BlueVideoFifo_Attributes;

typedef enum _ECardType
{
    CRD_BLUEDEEP_LT =               0,   /*  not supported */
    CRD_BLUEDEEP_SD =               1,   /*  not supported */
    CRD_BLUEDEEP_AV =               2,   /*  not supported */
    CRD_BLUEDEEP_IO =               3,   /*  not supported */
    CRD_BLUEWILD_AV =               4,   /*  not supported */
    CRD_IRIDIUM_HD =                5,   /*  not supported */
    CRD_BLUEWILD_RT =               6,   /*  not supported */
    CRD_BLUEWILD_HD =               7,   /*  not supported */
    CRD_REDDEVIL =                  8,   /*  not supported */
    CRD_BLUEDEEP_HD =               9,   /*  not supported, but value used for CRD_BLUE_EPOCH_2K */
    CRD_BLUEDEEP_HDS =              10,  /*  not supported */
    CRD_BLUE_ENVY =                 11,  /*  not supported */
    CRD_BLUE_PRIDE =                12,  /*  not supported */
    CRD_BLUE_GREED =                13,  /*  not supported */
    CRD_BLUE_INGEST =               14,  /*  not supported */
    CRD_BLUE_SD_DUALLINK =          15,  /*  not supported */
    CRD_BLUE_CATALYST =             16,  /*  not supported */
    CRD_BLUE_SD_DUALLINK_PRO =      17,  /*  not supported */
    CRD_BLUE_SD_INGEST_PRO =        18,  /*  not supported */
    CRD_BLUE_SD_DEEPBLUE_LITE_PRO = 19,  /*  not supported */
    CRD_BLUE_SD_SINGLELINK_PRO =    20,  /*  not supported */
    CRD_BLUE_SD_IRIDIUM_AV_PRO =    21,  /*  not supported */
    CRD_BLUE_SD_FIDELITY =          22,  /*  not supported */
    CRD_BLUE_SD_FOCUS =             23,  /*  not supported */
    CRD_BLUE_SD_PRIME =             24,  /*  not supported */

    CRD_BLUE_EPOCH_2K =             9,
	CRD_BLUE_EPOCH_2K_HORIZON =		CRD_BLUE_EPOCH_2K,
	CRD_BLUE_EPOCH_2K_CORE =		25,
	CRD_BLUE_EPOCH_2K_ULTRA =		26,
	CRD_BLUE_EPOCH_HORIZON =		27,
	CRD_BLUE_EPOCH_CORE =			28,
	CRD_BLUE_EPOCH_ULTRA =			29,
	CRD_BLUE_CREATE_HD =			30,
	CRD_BLUE_CREATE_2K =			31,
	CRD_BLUE_CREATE_2K_ULTRA =		32,
	CRD_BLUE_CREATE_3D =			CRD_BLUE_CREATE_2K,
	CRD_BLUE_CREATE_3D_ULTRA =		CRD_BLUE_CREATE_2K_ULTRA,
	CRD_BLUE_SUPER_NOVA =			33,
	CRD_BLUE_SUPER_NOVA_S_PLUS =	34,
	CRD_BLUE_SUPER_NOVA_MICRO =		35,
	CRD_BLUE_NEUTRON =				CRD_BLUE_SUPER_NOVA_MICRO,
	CRD_BLUE_EPOCH_CG =				36,
	CRD_BLUE_KRONOS_ELEKTRON =		37,
	CRD_BLUE_KRONOS_OPTIKOS =		38,
    CRD_BLUE_KRONOS_K8 =            39,

	CRD_INVALID =					500
} ECardType;

typedef enum _EHDCardSubType
{
    CRD_HD_FURY =       1,   /* not supported */
    CRD_HD_VENGENCE =   2,   /* not supported */
    CRD_HD_IRIDIUM_XP = 3,   /* not supported */
    CRD_HD_IRIDIUM =    4,   /* not supported */
    CRD_HD_LUST =       5,   /* not supported */
    CRD_HD_INVALID           /* not supported */
}EHDCardSubType;

enum EEpochFirmwareProductID
{
	ORAC_FILMPOST_FIRMWARE_PRODUCTID =									(0x01),	 /* Epoch (2K) Horizon/Core/Ultra, Create/Create3D/Create3D Ultra */
	ORAC_BROADCAST_FIRMWARE_PRODUCTID =									(0x02),	 /* Epoch (2K) Horizon/Core/Ultra, Create/Create3D/Create3D Ultra */
	ORAC_ASI_FIRMWARE_PRODUCTID =										(0x03),	 /* Epoch (2K) Horizon/Core/Ultra */
	ORAC_4SDIINPUT_FIRMWARE_PRODUCTID =									(0x04),	 /* Epoch Supernova/Supernova S+ */
	ORAC_4SDIOUTPUT_FIRMWARE_PRODUCTID =								(0x05),	 /* Epoch Supernova/Supernova S+ */
	ORAC_2SDIINPUT_2SDIOUTPUT_FIRMWARE_PRODUCTID =						(0x06),	 /* Epoch Supernova/Supernova S+ */
	ORAC_1SDIINPUT_3SDIOUTPUT_FIRMWARE_PRODUCTID =						(0x08),	 /* Epoch Supernova/Supernova S+, deprecated */
	ORAC_INPUT_1SDI_1CHANNEL_OUTPUT_4SDI_3CHANNEL_FIRMWARE_PRODUCTID =	(0x09),	 /* Epoch Supernova/Supernova S+ */
	ORAC_INPUT_2SDI_2CHANNEL_OUTPUT_3SDI_2CHANNEL_FIRMWARE_PRODUCTID =	(0x0A),	 /* Epoch Supernova/Supernova S+, deprecated */
	ORAC_INPUT_3SDI_3CHANNEL_OUTPUT_1SDI_1CHANNEL_FIRMWARE_PRODUCTID =	(0x0B),	 /* Epoch Supernova/Supernova S+ */
	ORAC_BNC_ASI_FIRMWARE_PRODUCTID =									(0x0C),	 /* Epoch Supernova/Supernova S+ */
	ORAC_NEUTRON_2_IN_0_OUT_FIRMWARE_PRODUCTID =						(0x0D),	 /* Epoch Neutron */
	ORAC_NEUTRON_0_IN_2_OUT_FIRMWARE_PRODUCTID =						(0x0E),	 /* Epoch Neutron */
	ORAC_NEUTRON_1_IN_1_OUT_FIRMWARE_PRODUCTID =						(0x0F),	 /* Epoch Neutron */
	ORAC_NEUTRON_2_IN_0_OUT_SCALER_FIRMWARE_PRODUCTID =					(0x10),	 /* Epoch Neutron */
	ORAC_NEUTRON_0_IN_2_OUT_SCALER_FIRMWARE_PRODUCTID =					(0x11),	 /* Epoch Neutron */
	ORAC_NEUTRON_1_IN_1_OUT_SCALER_FIRMWARE_PRODUCTID =					(0x12),	 /* Epoch Neutron */
	ORAC_NEUTRON_ASI_FIRMWARE_PRODUCTID =								(0x13),	 /* Epoch Neutron */
	ORAC_INPUT_1SDI_1CHANNEL_OUTPUT_3SDI_3CHANNEL_FIRMWARE_PRODUCTID =	(0x14),	 /* Epoch Supernova/Supernova S+ */
	ORAC_NEUTRON_1_IN_2_OUT_FIRMWARE_PRODUCTID =						(0x15),	 /* Epoch Neutron */
	ORAC_NEUTRON_3_IN_0_OUT_FIRMWARE_PRODUCTID =						(0x16),	 /* Epoch Neutron */
	ORAC_NEUTRON_0_IN_3_OUT_FIRMWARE_PRODUCTID =						(0x17),	 /* Epoch Neutron */
	ORAC_INPUT_1SDI_1CHANNEL_OUTPUT_3SDI_2CHANNEL_FIRMWARE_PRODUCTID =  (0x18),	 /* Epoch Supernova/Supernova S+ */
    ORAC_NEUTRON_2_IN_1_OUT_FIRMWARE_PRODUCTID =                        (0x19),	 /* Epoch Neutron */
    ZEUS_4_IN_4_OUT_FIRMWARE_PRODUCTID =                                (0x1A),  /* Kronos Elektron/Optikos */
    ZEUS_8_IN_FIRMWARE_PRODUCTID =                                      (0x1B),  /* Kronos Elektron/Optikos */
    ZEUS_8_OUT_FIRMWARE_PRODUCTID =                                     (0x1C),  /* Kronos Elektron/Optikos */
    ZEUS_RECOVERY_FIRMWARE_PRODUCTID =                                  (0x1D),  /* Kronos Elektron/Optikos */
};

typedef enum _EBlueLUTType
{
	BLUE_MAIN_LUT_B_Pb =	0,
	BLUE_MAIN_LUT_G_Y =		1,
	BLUE_MAIN_LUT_R_Pr =	2,
	BLUE_AUX_LUT_B_Pb =		3,
	BLUE_AUX_LUT_G_Y =		4,
	BLUE_AUX_LUT_R_Pr =		5,
}EBlueLUTType;

typedef enum _EBlueConnectorIdentifier
{
	BLUE_CONNECTOR_INVALID =		-1,

	BLUE_CONNECTOR_BNC_A =			0,
	BLUE_CONNECTOR_BNC_B =			1,
	BLUE_CONNECTOR_BNC_C =			2,
	BLUE_CONNECTOR_BNC_D =			3,
	BLUE_CONNECTOR_BNC_E =			4,
	BLUE_CONNECTOR_BNC_F =			5,
	BLUE_CONNECTOR_GENLOCK =		6,

    BLUE_CONNECTOR_REF_IN =         BLUE_CONNECTOR_GENLOCK,
    BLUE_CONNECTOR_REF_OUT =        7,
    BLUE_CONNECTOR_INTERLOCK_IN =   8,
    BLUE_CONNECTOR_INTERLOCK_OUT =  9,

	BLUE_CONNECTOR_ANALOG_VIDEO_1 =	100,
	BLUE_CONNECTOR_ANALOG_VIDEO_2 =	101,
	BLUE_CONNECTOR_ANALOG_VIDEO_3 =	102,
	BLUE_CONNECTOR_ANALOG_VIDEO_4 =	103,
	BLUE_CONNECTOR_ANALOG_VIDEO_5 =	104,
	BLUE_CONNECTOR_ANALOG_VIDEO_6 =	105,

	BLUE_CONNECTOR_DVID_1 =			200,
	BLUE_CONNECTOR_DVID_2 =			201,
	BLUE_CONNECTOR_DVID_3 =			202,
	BLUE_CONNECTOR_DVID_4 =			203,
	BLUE_CONNECTOR_DVID_5 =			204,
    BLUE_CONNECTOR_SDI_OUTPUT_A =   BLUE_CONNECTOR_DVID_1,
    BLUE_CONNECTOR_SDI_OUTPUT_B =   BLUE_CONNECTOR_DVID_2,
    BLUE_CONNECTOR_SDI_OUTPUT_C	=	205,
	BLUE_CONNECTOR_SDI_OUTPUT_D =	206,
    BLUE_CONNECTOR_SDI_OUTPUT_E	=	207,
	BLUE_CONNECTOR_SDI_OUTPUT_F =	208,
    BLUE_CONNECTOR_SDI_OUTPUT_G	=	209,
	BLUE_CONNECTOR_SDI_OUTPUT_H =	210,

    BLUE_CONNECTOR_SDI_OUTPUT_1 =   BLUE_CONNECTOR_SDI_OUTPUT_A,
    BLUE_CONNECTOR_SDI_OUTPUT_2 =   BLUE_CONNECTOR_SDI_OUTPUT_B,
    BLUE_CONNECTOR_SDI_OUTPUT_3 =   BLUE_CONNECTOR_SDI_OUTPUT_C,
    BLUE_CONNECTOR_SDI_OUTPUT_4 =   BLUE_CONNECTOR_SDI_OUTPUT_D,
    BLUE_CONNECTOR_SDI_OUTPUT_5 =   BLUE_CONNECTOR_SDI_OUTPUT_E,
    BLUE_CONNECTOR_SDI_OUTPUT_6 =   BLUE_CONNECTOR_SDI_OUTPUT_F,
    BLUE_CONNECTOR_SDI_OUTPUT_7 =   BLUE_CONNECTOR_SDI_OUTPUT_G,
    BLUE_CONNECTOR_SDI_OUTPUT_8 =   BLUE_CONNECTOR_SDI_OUTPUT_H,

	BLUE_CONNECTOR_AES =			300,
	BLUE_CONNECTOR_ANALOG_AUDIO_1 =	301,
	BLUE_CONNECTOR_ANALOG_AUDIO_2 =	302,
    BLUE_CONNECTOR_DVID_6 =			303,
	BLUE_CONNECTOR_DVID_7 =			304,

    BLUE_CONNECTOR_SDI_INPUT_A =    BLUE_CONNECTOR_DVID_3,
    BLUE_CONNECTOR_SDI_INPUT_B =    BLUE_CONNECTOR_DVID_4,
    BLUE_CONNECTOR_SDI_INPUT_C =    BLUE_CONNECTOR_DVID_6,
    BLUE_CONNECTOR_SDI_INPUT_D =    BLUE_CONNECTOR_DVID_7,
    BLUE_CONNECTOR_SDI_INPUT_E =    305,
    BLUE_CONNECTOR_SDI_INPUT_F =    306,
    BLUE_CONNECTOR_SDI_INPUT_G =    307,
    BLUE_CONNECTOR_SDI_INPUT_H =    308,

    BLUE_CONNECTOR_SDI_INPUT_1 =    BLUE_CONNECTOR_SDI_INPUT_A,
    BLUE_CONNECTOR_SDI_INPUT_2 =    BLUE_CONNECTOR_SDI_INPUT_B,
    BLUE_CONNECTOR_SDI_INPUT_3 =    BLUE_CONNECTOR_SDI_INPUT_C,
    BLUE_CONNECTOR_SDI_INPUT_4 =    BLUE_CONNECTOR_SDI_INPUT_D,
    BLUE_CONNECTOR_SDI_INPUT_5 =    BLUE_CONNECTOR_SDI_INPUT_E,
    BLUE_CONNECTOR_SDI_INPUT_6 =    BLUE_CONNECTOR_SDI_INPUT_F,
    BLUE_CONNECTOR_SDI_INPUT_7 =    BLUE_CONNECTOR_SDI_INPUT_G,
    BLUE_CONNECTOR_SDI_INPUT_8 =    BLUE_CONNECTOR_SDI_INPUT_H,

}EBlueConnectorIdentifier;

typedef enum _EBlueConnectorSignalDirection
{
	BLUE_CONNECTOR_SIGNAL_INVALID = -1,
	BLUE_CONNECTOR_SIGNAL_INPUT = 0,
	BLUE_CONNECTOR_SIGNAL_OUTPUT = 1,
}EBlueConnectorSignalDirection;

typedef enum _EBlueConnectorProperty
{
	BLUE_INVALID_CONNECTOR_PROPERTY =				-1,

	/* signal property */
	BLUE_CONNECTOR_PROP_INPUT_SIGNAL =				0,
	BLUE_CONNECTOR_PROP_OUTPUT_SIGNAL =				1,

    /* Video output */
	BLUE_CONNECTOR_PROP_SDI =						0,
	BLUE_CONNECTOR_PROP_YUV_Y =						1,
	BLUE_CONNECTOR_PROP_YUV_U =						2,
	BLUE_CONNECTOR_PROP_YUV_V =						3,
	BLUE_CONNECTOR_PROP_RGB_R =						4,
	BLUE_CONNECTOR_PROP_RGB_G =						5,
	BLUE_CONNECTOR_PROP_RGB_B =						6,
	BLUE_CONNECTOR_PROP_CVBS =						7,
	BLUE_CONNECTOR_PROP_SVIDEO_Y =					8,
	BLUE_CONNECTOR_PROP_SVIDEO_C =					9,

    /* Audio output */
	BLUE_CONNECTOR_PROP_AUDIO_AES =					0x2000,
	BLUE_CONNECTOR_PROP_AUDIO_EMBEDDED =			0x2001,
	BLUE_CONNECTOR_PROP_AUDIO_ANALOG =				0x2002,


	BLUE_CONNECTOR_PROP_SINGLE_LINK =				0x3000,
	BLUE_CONNECTOR_PROP_DUALLINK_LINK_1 =			0x3001,
	BLUE_CONNECTOR_PROP_DUALLINK_LINK_2 =			0x3002,
	BLUE_CONNECTOR_PROP_DUALLINK_LINK =				0x3003,

	BLUE_CONNECTOR_PROP_STEREO_MODE_SIDE_BY_SIDE =	0x3004,
	BLUE_CONNECTOR_PROP_STEREO_MODE_TOP_DOWN =		0x3005,
	BLUE_CONNECTOR_PROP_STEREO_MODE_LINE_BY_LINE =	0x3006,

}EBlueConnectorProperty;

typedef enum
{
	BLUE_AUDIO_AES =		0,					/** 8 channels of AES */
	BLUE_AUDIO_ANALOG =		1,					/** 2 channels of analog audio */
	BLUE_AUDIO_SDIA =		2,					/** deprecated, do not use */
	BLUE_AUDIO_EMBEDDED =	BLUE_AUDIO_SDIA,	/** use BLUE_AUDIO_EMBEDDED for any embedded audio stream; the stream is associated with the SDK object (BlueVelvet4/BlueVelvetC) */
	BLUE_AUDIO_SDIB =		3,					/** deprecated, do not use */
	BLUE_AUDIO_AES_PAIR0 =	4,					/** deprecated, do not use */
	BLUE_AUDIO_AES_PAIR1 =	5,					/** deprecated, do not use */
	BLUE_AUDIO_AES_PAIR2 =	6,					/** deprecated, do not use */
	BLUE_AUDIO_AES_PAIR3 =	7,					/** deprecated, do not use */
	BLUE_AUDIO_SDIC =		8,					/** deprecated, do not use */
	BLUE_AUDIO_SDID =		9,					/** deprecated, do not use */
    BLUE_AUDIO_SDIE =		10,					/** deprecated, do not use */
    BLUE_AUDIO_SDIF =		11,					/** deprecated, do not use */
    BLUE_AUDIO_SDIG =		12,					/** deprecated, do not use */
    BLUE_AUDIO_SDIH =		13,					/** deprecated, do not use */
	BLUE_AUDIO_INVALID =	100
} Blue_Audio_Connector_Type;

typedef enum _EAudioRate
{
	AUDIO_SAMPLE_RATE_48K =		48000,
	AUDIO_SAMPLE_RATE_96K =		96000,
	AUDIO_SAMPLE_RATE_UNKNOWN =	-1
} EAudioRate;

typedef enum _EDMADataType
{
    BLUE_DATA_FRAME =               0,
    BLUE_DATA_IMAGE =               BLUE_DATA_FRAME,
    BLUE_DATA_FIELD1 =              1,
    BLUE_DATA_FIELD2 =              2,              /** deprecated, do not use */
    BLUE_DATA_VBI =                 3,
    BLUE_DATA_HANC =                4,
    BLUE_DATA_AUDIO_IN =            5,              /** deprecated, do not use */
    BLUE_DATA_AUDIO_OUT =           6,              /** deprecated, do not use */
    BLUE_DATA_FRAME_RDOM =          7,              /** deprecated, do not use */
    BLUE_DATA_FRAME_STEREO_LEFT =   BLUE_DATA_FRAME,
    BLUE_DATA_FRAME_STEREO_RIGHT =  8,
    BLUE_DMADATA_INVALID =          9,

    // preferred aliases and new types
    BLUE_DMA_DATA_TYPE_IMAGE =       BLUE_DATA_FRAME,
    BLUE_DMA_DATA_TYPE_IMAGE_FRAME = BLUE_DMA_DATA_TYPE_IMAGE,
    BLUE_DMA_DATA_TYPE_IMAGE_FIELD = BLUE_DATA_FIELD1,
    BLUE_DMA_DATA_TYPE_VANC =        BLUE_DATA_VBI,
    BLUE_DMA_DATA_TYPE_VANC_A =      BLUE_DMA_DATA_TYPE_VANC,
    BLUE_DMA_DATA_TYPE_VANC_B =      10,
    BLUE_DMA_DATA_TYPE_HANC_AUDIO =  BLUE_DATA_HANC,
    BLUE_DMA_DATA_TYPE_HANC_RAW_A =  11,
    BLUE_DMA_DATA_TYPE_HANC_RAW_B =  12,
}EDMADataType;

typedef enum _EDMADirection
{
	DMA_WRITE =		0,
	DMA_READ =		1,
	DMA_INVALID =	2
}EDMADirection;

typedef enum _EBlueVideoAuxInfoType
{
	BLUE_VIDEO_AUX_MEMFMT_CHANGE = 1,
	BLUE_VIDEO_AUX_UPDATE_LTC = 2,
	BLUE_VIDEO_AUX_UPDATE_GPIO = 4,
	BLUE_VIDEO_AUX_VIDFMT_CHANGE = 8,
}EBlueVideoAuxInfoType;

typedef enum _MatrixColType
{
	COL_BLUE_PB = 0,
	COL_RED_PR = 1,
	COL_GREEN_Y = 2,
	COL_KEY = 3
}MatrixColType;

/*	This enumerator can be used to set the image orienatation of the frame. */
typedef enum _EImageOrientation
{
	ImageOrientation_Normal = 0,	/* in this configuration frame is top to bottom and left to right */
	ImageOrientation_VerticalFlip = 1,	/* in this configuration frame is bottom to top and left to right */
	ImageOrientation_Invalid = 2,
}EImageOrientation;

/* This enumerator defines the reference signal source that can be used with bluefish cards */
typedef enum _EBlueGenlockSource
{
	BlueGenlockBNC =    0x00000000,     /** Genlock is used as reference signal source */
	BlueSDIBNC =        0x00010000,	    /** SDI input B is used as reference signal source */
	BlueSDI_B_BNC =     BlueSDIBNC,
	BlueSDI_A_BNC =     0x00020000,	    /** SDI input A is used as reference signal source */
	BlueAnalog_BNC =    0x00040000,	    /** deprecated, not supported */
	BlueSoftware =      0x00080000,
	BlueFreeRunning =   BlueSoftware,   /** free running, but phase can be software controlled */
	BlueGenlockAux =    0x00100000,     /** auxiliary genlock connector on Epoch Neutron cards */
	BlueInterlock =     0x00200000,     /** interlock connector on Epoch Neutron and Kronos cards */

    /* aliases and new defintions */
    BlueRefSrc_FreeRunning = BlueFreeRunning,
    BlueRefSrc_RefIn =       BlueGenlockBNC,    /* genlock signal on ref in BNC connector      (Epoch/Kronos) */
    BlueRefSrc_RefInAux =    BlueGenlockAux,    /* genlock signal on ref in AUX BNC connector  (Epoch Neutron only) */
    BlueRefSrc_InterlockIn = BlueInterlock,     /* interlock signal on internal MMCX connector (Epoch/Kronos) */
    BlueRefSrc_SdiInput1 =   BlueSDI_A_BNC,
    BlueRefSrc_SdiInput2 =   BlueSDI_B_BNC,
    BlueRefSrc_SdiInput3 =   0x01000000,
    BlueRefSrc_SdiInput4 =   0x01100000,
    BlueRefSrc_SdiInput5 =   0x01200000,
    BlueRefSrc_SdiInput6 =   0x01300000,
    BlueRefSrc_SdiInput7 =   0x01400000,
    BlueRefSrc_SdiInput8 =   0x01500000,
}EBlueGenlockSource;

typedef enum _EBlueVideoChannel
{
	BLUE_VIDEOCHANNEL_A = 0,
	BLUE_VIDEOCHANNEL_B = 1,
	BLUE_VIDEOCHANNEL_C = 2,
	BLUE_VIDEOCHANNEL_D = 3,
	BLUE_VIDEOCHANNEL_E = 4,
	BLUE_VIDEOCHANNEL_F = 5,
	BLUE_VIDEOCHANNEL_G = 6,
	BLUE_VIDEOCHANNEL_H = 7,
	BLUE_VIDEOCHANNEL_I = 8,
	BLUE_VIDEOCHANNEL_J = 9,
	BLUE_VIDEOCHANNEL_K = 10,
	BLUE_VIDEOCHANNEL_L = 11,
	BLUE_VIDEOCHANNEL_M = 12,
	BLUE_VIDEOCHANNEL_N = 13,
	BLUE_VIDEOCHANNEL_O = 14,
	BLUE_VIDEOCHANNEL_P = 15,
	BLUE_VIDEO_OUTPUT_CHANNEL_A = BLUE_VIDEOCHANNEL_A,
	BLUE_VIDEO_OUTPUT_CHANNEL_B = BLUE_VIDEOCHANNEL_B,
	BLUE_VIDEO_INPUT_CHANNEL_A = BLUE_VIDEOCHANNEL_C,
	BLUE_VIDEO_INPUT_CHANNEL_B = BLUE_VIDEOCHANNEL_D,
	BLUE_VIDEO_INPUT_CHANNEL_C = BLUE_VIDEOCHANNEL_E,
	BLUE_VIDEO_INPUT_CHANNEL_D = BLUE_VIDEOCHANNEL_F,
	BLUE_VIDEO_OUTPUT_CHANNEL_C = BLUE_VIDEOCHANNEL_G,
	BLUE_VIDEO_OUTPUT_CHANNEL_D = BLUE_VIDEOCHANNEL_H,
	BLUE_VIDEO_OUTPUT_CHANNEL_E = BLUE_VIDEOCHANNEL_I,
	BLUE_VIDEO_OUTPUT_CHANNEL_F = BLUE_VIDEOCHANNEL_J,
	BLUE_VIDEO_OUTPUT_CHANNEL_G = BLUE_VIDEOCHANNEL_K,
	BLUE_VIDEO_OUTPUT_CHANNEL_H = BLUE_VIDEOCHANNEL_L,
	BLUE_VIDEO_INPUT_CHANNEL_E = BLUE_VIDEOCHANNEL_M,
	BLUE_VIDEO_INPUT_CHANNEL_F = BLUE_VIDEOCHANNEL_N,
	BLUE_VIDEO_INPUT_CHANNEL_G = BLUE_VIDEOCHANNEL_O,
	BLUE_VIDEO_INPUT_CHANNEL_H = BLUE_VIDEOCHANNEL_P,

    BLUE_VIDEO_INPUT_CHANNEL_1 = BLUE_VIDEO_INPUT_CHANNEL_A,
    BLUE_VIDEO_INPUT_CHANNEL_2 = BLUE_VIDEO_INPUT_CHANNEL_B,
    BLUE_VIDEO_INPUT_CHANNEL_3 = BLUE_VIDEO_INPUT_CHANNEL_C,
    BLUE_VIDEO_INPUT_CHANNEL_4 = BLUE_VIDEO_INPUT_CHANNEL_D,
    BLUE_VIDEO_INPUT_CHANNEL_5 = BLUE_VIDEO_INPUT_CHANNEL_E,
    BLUE_VIDEO_INPUT_CHANNEL_6 = BLUE_VIDEO_INPUT_CHANNEL_F,
    BLUE_VIDEO_INPUT_CHANNEL_7 = BLUE_VIDEO_INPUT_CHANNEL_G,
    BLUE_VIDEO_INPUT_CHANNEL_8 = BLUE_VIDEO_INPUT_CHANNEL_H,
    BLUE_VIDEO_OUTPUT_CHANNEL_1 = BLUE_VIDEO_OUTPUT_CHANNEL_A,
    BLUE_VIDEO_OUTPUT_CHANNEL_2 = BLUE_VIDEO_OUTPUT_CHANNEL_B,
    BLUE_VIDEO_OUTPUT_CHANNEL_3 = BLUE_VIDEO_OUTPUT_CHANNEL_C,
    BLUE_VIDEO_OUTPUT_CHANNEL_4 = BLUE_VIDEO_OUTPUT_CHANNEL_D,
    BLUE_VIDEO_OUTPUT_CHANNEL_5 = BLUE_VIDEO_OUTPUT_CHANNEL_E,
    BLUE_VIDEO_OUTPUT_CHANNEL_6 = BLUE_VIDEO_OUTPUT_CHANNEL_F,
    BLUE_VIDEO_OUTPUT_CHANNEL_7 = BLUE_VIDEO_OUTPUT_CHANNEL_G,
    BLUE_VIDEO_OUTPUT_CHANNEL_8 = BLUE_VIDEO_OUTPUT_CHANNEL_H,

	BLUE_VIDEOCHANNEL_INVALID = 30
}EBlueVideoChannel;

typedef enum _EEpochRoutingElements
{
	EPOCH_SRC_DEST_SCALER_0 =               0x1,
	EPOCH_SRC_DEST_SCALER_1 =               0x2,
	EPOCH_SRC_DEST_SCALER_2 =               0x3,
	EPOCH_SRC_DEST_SCALER_3 =               0x4,

	EPOCH_SRC_SDI_INPUT_A =                 0x5,
	EPOCH_SRC_SDI_INPUT_B =                 0x6,
	EPOCH_SRC_SDI_INPUT_C =                 0x7,
	EPOCH_SRC_SDI_INPUT_D =                 0x8,

	EPOCH_DEST_SDI_OUTPUT_A =               0x9,
	EPOCH_DEST_SDI_OUTPUT_B =               0xA,
	EPOCH_DEST_SDI_OUTPUT_C =               0xB,
	EPOCH_DEST_SDI_OUTPUT_D =               0xC,

	EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHA =    0xD,
	EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHB =    0xE,

	EPOCH_DEST_INPUT_MEM_INTERFACE_CHA =    0xF,
	EPOCH_DEST_INPUT_MEM_INTERFACE_CHB =    0x10,

	EPOCH_DEST_AES_ANALOG_AUDIO_OUTPUT =    0x11,

	EPOCH_SRC_AV_SIGNAL_GEN =               0x12,
	EPOCH_SRC_DEST_VPIO_SCALER_0 =          0x13,
	EPOCH_SRC_DEST_VPIO_SCALER_1 =          0x14,

	EPOCH_DEST_VARIVUE_HDMI =               0x15,

	EPOCH_DEST_INPUT_MEM_INTERFACE_CHC =    0x16,
	EPOCH_DEST_INPUT_MEM_INTERFACE_CHD =    0x17,

	EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHC =    0x18,
	EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHD =    0x19,

	EPOCH_SRC_SDI_INPUT_A_3GB_LINK_B =      0x1A,
	EPOCH_SRC_SDI_INPUT_B_3GB_LINK_B =      0x1B,
	EPOCH_SRC_SDI_INPUT_C_3GB_LINK_B =      0x1C,
	EPOCH_SRC_SDI_INPUT_D_3GB_LINK_B =      0x1D,

	EPOCH_DEST_SDI_OUTPUT_A_3GB_LINK_B =    0x1E,
	EPOCH_DEST_SDI_OUTPUT_B_3GB_LINK_B =    0x1F,
	EPOCH_DEST_SDI_OUTPUT_C_3GB_LINK_B =    0x20,
	EPOCH_DEST_SDI_OUTPUT_D_3GB_LINK_B =    0x21,

	EPOCH_DEST_HDMI_OUTPUT =                0x22,
	EPOCH_DEST_HDMI_OUTPUT_LINK_A =         EPOCH_DEST_HDMI_OUTPUT,
	EPOCH_DEST_HDMI_OUTPUT_LINK_B =         0x23,

    EPOCH_SRC_SDI_INPUT_E =                 0x24,
    EPOCH_SRC_SDI_INPUT_F =                 0x25,
    EPOCH_SRC_SDI_INPUT_G =                 0x26,
    EPOCH_SRC_SDI_INPUT_H =                 0x27,
    
    EPOCH_SRC_SDI_INPUT_E_3GB_LINK_B =      0x28,
	EPOCH_SRC_SDI_INPUT_F_3GB_LINK_B =      0x29,
	EPOCH_SRC_SDI_INPUT_G_3GB_LINK_B =      0x2A,
	EPOCH_SRC_SDI_INPUT_H_3GB_LINK_B =      0x2B,

    EPOCH_DEST_SDI_OUTPUT_E =               0x2C,
	EPOCH_DEST_SDI_OUTPUT_F =               0x2D,
	EPOCH_DEST_SDI_OUTPUT_G =               0x2E,
	EPOCH_DEST_SDI_OUTPUT_H =               0x2F,

    EPOCH_DEST_SDI_OUTPUT_E_3GB_LINK_B =    0x30,
	EPOCH_DEST_SDI_OUTPUT_F_3GB_LINK_B =    0x31,
	EPOCH_DEST_SDI_OUTPUT_G_3GB_LINK_B =    0x32,
	EPOCH_DEST_SDI_OUTPUT_H_3GB_LINK_B =    0x33,

    EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHE =    0x34,
	EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHF =    0x35,
    EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHG =    0x36,
	EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHH =    0x37,

    EPOCH_DEST_INPUT_MEM_INTERFACE_CHE =    0x38,
	EPOCH_DEST_INPUT_MEM_INTERFACE_CHF =    0x39,
    EPOCH_DEST_INPUT_MEM_INTERFACE_CHG =    0x3C,
    EPOCH_DEST_INPUT_MEM_INTERFACE_CHH =    0x3D,

    /* aliases */
    EPOCH_SRC_SDI_INPUT_1 =                 EPOCH_SRC_SDI_INPUT_A,
    EPOCH_SRC_SDI_INPUT_2 =                 EPOCH_SRC_SDI_INPUT_B,
    EPOCH_SRC_SDI_INPUT_3 =                 EPOCH_SRC_SDI_INPUT_C,
    EPOCH_SRC_SDI_INPUT_4 =                 EPOCH_SRC_SDI_INPUT_D,
    EPOCH_SRC_SDI_INPUT_5 =                 EPOCH_SRC_SDI_INPUT_E,
    EPOCH_SRC_SDI_INPUT_6 =                 EPOCH_SRC_SDI_INPUT_F,
    EPOCH_SRC_SDI_INPUT_7 =                 EPOCH_SRC_SDI_INPUT_G,
    EPOCH_SRC_SDI_INPUT_8 =                 EPOCH_SRC_SDI_INPUT_H,
    EPOCH_SRC_SDI_INPUT_A_3GB_LINK_A =      EPOCH_SRC_SDI_INPUT_A,
	EPOCH_SRC_SDI_INPUT_B_3GB_LINK_A =      EPOCH_SRC_SDI_INPUT_B,
	EPOCH_SRC_SDI_INPUT_C_3GB_LINK_A =      EPOCH_SRC_SDI_INPUT_C,
	EPOCH_SRC_SDI_INPUT_D_3GB_LINK_A =      EPOCH_SRC_SDI_INPUT_D,
    EPOCH_SRC_SDI_INPUT_E_3GB_LINK_A =      EPOCH_SRC_SDI_INPUT_E,
	EPOCH_SRC_SDI_INPUT_F_3GB_LINK_A =      EPOCH_SRC_SDI_INPUT_F,
	EPOCH_SRC_SDI_INPUT_G_3GB_LINK_A =      EPOCH_SRC_SDI_INPUT_G,
	EPOCH_SRC_SDI_INPUT_H_3GB_LINK_A =      EPOCH_SRC_SDI_INPUT_H,
    EPOCH_SRC_SDI_INPUT_1_3GB_LINK_A =      EPOCH_SRC_SDI_INPUT_A,
	EPOCH_SRC_SDI_INPUT_2_3GB_LINK_A =      EPOCH_SRC_SDI_INPUT_B,
	EPOCH_SRC_SDI_INPUT_3_3GB_LINK_A =      EPOCH_SRC_SDI_INPUT_C,
	EPOCH_SRC_SDI_INPUT_4_3GB_LINK_A =      EPOCH_SRC_SDI_INPUT_D,
    EPOCH_SRC_SDI_INPUT_5_3GB_LINK_A =      EPOCH_SRC_SDI_INPUT_E,
	EPOCH_SRC_SDI_INPUT_6_3GB_LINK_A =      EPOCH_SRC_SDI_INPUT_F,
	EPOCH_SRC_SDI_INPUT_7_3GB_LINK_A =      EPOCH_SRC_SDI_INPUT_G,
	EPOCH_SRC_SDI_INPUT_8_3GB_LINK_A =      EPOCH_SRC_SDI_INPUT_H,
    EPOCH_SRC_SDI_INPUT_1_3GB_LINK_B =      EPOCH_SRC_SDI_INPUT_A_3GB_LINK_B,
    EPOCH_SRC_SDI_INPUT_2_3GB_LINK_B =      EPOCH_SRC_SDI_INPUT_B_3GB_LINK_B,
    EPOCH_SRC_SDI_INPUT_3_3GB_LINK_B =      EPOCH_SRC_SDI_INPUT_C_3GB_LINK_B,
    EPOCH_SRC_SDI_INPUT_4_3GB_LINK_B =      EPOCH_SRC_SDI_INPUT_D_3GB_LINK_B,
    EPOCH_SRC_SDI_INPUT_5_3GB_LINK_B =      EPOCH_SRC_SDI_INPUT_E_3GB_LINK_B,
    EPOCH_SRC_SDI_INPUT_6_3GB_LINK_B =      EPOCH_SRC_SDI_INPUT_F_3GB_LINK_B,
    EPOCH_SRC_SDI_INPUT_7_3GB_LINK_B =      EPOCH_SRC_SDI_INPUT_G_3GB_LINK_B,
    EPOCH_SRC_SDI_INPUT_8_3GB_LINK_B =      EPOCH_SRC_SDI_INPUT_H_3GB_LINK_B,
    
    EPOCH_DEST_SDI_OUTPUT_1 =               EPOCH_DEST_SDI_OUTPUT_A,
    EPOCH_DEST_SDI_OUTPUT_2 =               EPOCH_DEST_SDI_OUTPUT_B,
    EPOCH_DEST_SDI_OUTPUT_3 =               EPOCH_DEST_SDI_OUTPUT_C,
    EPOCH_DEST_SDI_OUTPUT_4 =               EPOCH_DEST_SDI_OUTPUT_D,
    EPOCH_DEST_SDI_OUTPUT_5 =               EPOCH_DEST_SDI_OUTPUT_E,
    EPOCH_DEST_SDI_OUTPUT_6 =               EPOCH_DEST_SDI_OUTPUT_F,
    EPOCH_DEST_SDI_OUTPUT_7 =               EPOCH_DEST_SDI_OUTPUT_G,
    EPOCH_DEST_SDI_OUTPUT_8 =               EPOCH_DEST_SDI_OUTPUT_H,
    EPOCH_DEST_SDI_OUTPUT_A_3GB_LINK_A =    EPOCH_DEST_SDI_OUTPUT_A,
	EPOCH_DEST_SDI_OUTPUT_B_3GB_LINK_A =    EPOCH_DEST_SDI_OUTPUT_B,
	EPOCH_DEST_SDI_OUTPUT_C_3GB_LINK_A =    EPOCH_DEST_SDI_OUTPUT_C,
	EPOCH_DEST_SDI_OUTPUT_D_3GB_LINK_A =    EPOCH_DEST_SDI_OUTPUT_D,
    EPOCH_DEST_SDI_OUTPUT_E_3GB_LINK_A =    EPOCH_DEST_SDI_OUTPUT_E,
	EPOCH_DEST_SDI_OUTPUT_F_3GB_LINK_A =    EPOCH_DEST_SDI_OUTPUT_F,
	EPOCH_DEST_SDI_OUTPUT_G_3GB_LINK_A =    EPOCH_DEST_SDI_OUTPUT_G,
	EPOCH_DEST_SDI_OUTPUT_H_3GB_LINK_A =    EPOCH_DEST_SDI_OUTPUT_H,
    EPOCH_DEST_SDI_OUTPUT_1_3GB_LINK_A =    EPOCH_DEST_SDI_OUTPUT_A,
	EPOCH_DEST_SDI_OUTPUT_2_3GB_LINK_A =    EPOCH_DEST_SDI_OUTPUT_B,
	EPOCH_DEST_SDI_OUTPUT_3_3GB_LINK_A =    EPOCH_DEST_SDI_OUTPUT_C,
	EPOCH_DEST_SDI_OUTPUT_4_3GB_LINK_A =    EPOCH_DEST_SDI_OUTPUT_D,
    EPOCH_DEST_SDI_OUTPUT_5_3GB_LINK_A =    EPOCH_DEST_SDI_OUTPUT_E,
	EPOCH_DEST_SDI_OUTPUT_6_3GB_LINK_A =    EPOCH_DEST_SDI_OUTPUT_F,
	EPOCH_DEST_SDI_OUTPUT_7_3GB_LINK_A =    EPOCH_DEST_SDI_OUTPUT_G,
	EPOCH_DEST_SDI_OUTPUT_8_3GB_LINK_A =    EPOCH_DEST_SDI_OUTPUT_H,
    EPOCH_DEST_SDI_OUTPUT_1_3GB_LINK_B =    EPOCH_DEST_SDI_OUTPUT_A_3GB_LINK_B,
	EPOCH_DEST_SDI_OUTPUT_2_3GB_LINK_B =    EPOCH_DEST_SDI_OUTPUT_B_3GB_LINK_B,
	EPOCH_DEST_SDI_OUTPUT_3_3GB_LINK_B =    EPOCH_DEST_SDI_OUTPUT_C_3GB_LINK_B,
	EPOCH_DEST_SDI_OUTPUT_4_3GB_LINK_B =    EPOCH_DEST_SDI_OUTPUT_D_3GB_LINK_B,
    EPOCH_DEST_SDI_OUTPUT_5_3GB_LINK_B =    EPOCH_DEST_SDI_OUTPUT_E_3GB_LINK_B,
	EPOCH_DEST_SDI_OUTPUT_6_3GB_LINK_B =    EPOCH_DEST_SDI_OUTPUT_F_3GB_LINK_B,
	EPOCH_DEST_SDI_OUTPUT_7_3GB_LINK_B =    EPOCH_DEST_SDI_OUTPUT_G_3GB_LINK_B,
	EPOCH_DEST_SDI_OUTPUT_8_3GB_LINK_B =    EPOCH_DEST_SDI_OUTPUT_H_3GB_LINK_B,
    
    EPOCH_SRC_OUTPUT_MEM_INTERFACE_CH1 =    EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHA,
    EPOCH_SRC_OUTPUT_MEM_INTERFACE_CH2 =    EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHB,
    EPOCH_SRC_OUTPUT_MEM_INTERFACE_CH3 =    EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHC,
    EPOCH_SRC_OUTPUT_MEM_INTERFACE_CH4 =    EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHD,
    EPOCH_SRC_OUTPUT_MEM_INTERFACE_CH5 =    EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHE,
    EPOCH_SRC_OUTPUT_MEM_INTERFACE_CH6 =    EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHF,
    EPOCH_SRC_OUTPUT_MEM_INTERFACE_CH7 =    EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHG,
    EPOCH_SRC_OUTPUT_MEM_INTERFACE_CH8 =    EPOCH_SRC_OUTPUT_MEM_INTERFACE_CHH,

    EPOCH_DEST_INPUT_MEM_INTERFACE_CH1 =    EPOCH_DEST_INPUT_MEM_INTERFACE_CHA,
    EPOCH_DEST_INPUT_MEM_INTERFACE_CH2 =    EPOCH_DEST_INPUT_MEM_INTERFACE_CHB,
    EPOCH_DEST_INPUT_MEM_INTERFACE_CH3 =    EPOCH_DEST_INPUT_MEM_INTERFACE_CHC,
    EPOCH_DEST_INPUT_MEM_INTERFACE_CH4 =    EPOCH_DEST_INPUT_MEM_INTERFACE_CHD,
    EPOCH_DEST_INPUT_MEM_INTERFACE_CH5 =    EPOCH_DEST_INPUT_MEM_INTERFACE_CHE,
    EPOCH_DEST_INPUT_MEM_INTERFACE_CH6 =    EPOCH_DEST_INPUT_MEM_INTERFACE_CHF,
    EPOCH_DEST_INPUT_MEM_INTERFACE_CH7 =    EPOCH_DEST_INPUT_MEM_INTERFACE_CHG,
    EPOCH_DEST_INPUT_MEM_INTERFACE_CH8 =    EPOCH_DEST_INPUT_MEM_INTERFACE_CHH,

}EEpochRoutingElements;

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
	MONO_CHANNEL_9	= 0x00000100,   /* to be used by analog audio output channels */
	MONO_CHANNEL_10	= 0x00000200,   /* to be used by analog audio output channels */
	MONO_CHANNEL_11	= 0x00000400,   /* actual channel 9  */
	MONO_CHANNEL_12	= 0x00000800,   /* actual channel 10 */
	MONO_CHANNEL_13	= 0x00001000,   /* actual channel 11 */
	MONO_CHANNEL_14	= 0x00002000,   /* actual channel 12 */
	MONO_CHANNEL_15	= 0x00004000,   /* actual channel 13 */
	MONO_CHANNEL_16	= 0x00008000,   /* actual channel 14 */
	MONO_CHANNEL_17	= 0x00010000,   /* actual channel 15 */
	MONO_CHANNEL_18	= 0x00020000    /* actual channel 16 */
}BlueAudioChannelDesc;

/* Use this enumerator to define the color space of the video signal on the SDI cable */
typedef enum _EConnectorSignalColorSpace
{
	RGB_ON_CONNECTOR = 0x00400000,	/*	Use this enumerator if the colorspace of video data on the SDI cable is RGB
									When using dual link capture/playback, user can choose the color space of the data.
									In single link SDI the color space of the signal is always YUV */
	YUV_ON_CONNECTOR = 0			/*	Use this enumerator if color space of video data on the SDI cable is RGB. */
}EConnectorSignalColorSpace;

/* Use this enumerator to define the data range of the RGB video frame data. */
typedef enum _ERGBDataRange
{
	CGR_RANGE = 0,	/*	In this mode RGB data expected by the user (capture) or provided by the user(playback) is
					in the range of 0-255(8 bit) or 0-1023(10 bit0).
					driver uses this information to choose the appropriate YUV conversion matrices. */
	SMPTE_RANGE = 1  /*	In this mode RGB data expected by the user (capture) or provided by the user(playback) is
					 in the range of 16-235(8 bit) or 64-940(10 bit0).
					 driver uses this information to choose the appropriate YUV conversion matrices. */
}ERGBDataRange;

typedef enum _EPreDefinedColorSpaceMatrix
{
	UNITY_MATRIX = 0,
	MATRIX_709_CGR = 1,
	MATRIX_RGB_TO_YUV_709_CGR = MATRIX_709_CGR,
	MATRIX_709 = 2,
	MATRIX_RGB_TO_YUV_709 = MATRIX_709,
	RGB_FULL_RGB_SMPTE = 3,
	MATRIX_601_CGR = 4,
	MATRIX_RGB_TO_YUV_601_CGR = MATRIX_601_CGR,
	MATRIX_601 = 5,
	MATRIX_RGB_TO_YUV_601 = MATRIX_601,
	MATRIX_VUYA = 6,
	UNITY_MATRIX_INPUT = 7,
	MATRIX_YUV_TO_RGB_709_CGR = 8,
	MATRIX_YUV_TO_RGB_709 = 9,
	RGB_SMPTE_RGB_FULL = 10,
	MATRIX_YUV_TO_RGB_601_CGR = 11,
	MATRIX_YUV_TO_RGB_601 = 12,
	MATRIX_USER_DEFINED = 13,
} EPreDefinedColorSpaceMatrix;

/* Use this enumerator for controlling the dual link functionality. */
typedef enum _EDualLinkSignalFormatType
{
	Signal_FormatType_4224 = 0,		/* sets the card to work in 4:2:2:4 mode */
	Signal_FormatType_4444 = 1,		/* sets the card to work in 4:4:4 10 bit dual link mode */
	Signal_FormatType_444_10BitSDI = Signal_FormatType_4444,
	Signal_FormatType_444_12BitSDI = 0x4,	/* sets the card to work in 4:4:4 12 bit dual link mode */
	Signal_FormatType_Independent_422 = 0x2,
	Signal_FormatType_Key_Key = 0x8000	/* not used currently on epoch cards */

}EDualLinkSignalFormatType;

enum ECardOperatingMode
{
	CardOperatingMode_SingleLink = 0x0,
	CardOperatingMode_Independent_422 = CardOperatingMode_SingleLink,
	CardOperatingMode_DualLink = 0x1,
	CardOperatingMode_StereoScopic_422 = 0x3,
	CardOperatingMode_Dependent_422 = CardOperatingMode_StereoScopic_422,	/* not used currently on epoch cards */
	CardOperatingMode_DualLink_Dual3G = 0x4,
};

typedef enum _blue_output_hanc_ioctl_enum
{
	blue_get_output_hanc_buffer = 0,
	blue_put_output_hanc_buffer = 1,
	blue_get_valid_silent_hanc_data_status = 3,
	blue_set_valid_silent_hanc_data_status = 4,
	blue_start_output_fifo = 5,
	blue_stop_output_fifo = 6,
	blue_init_output_fifo = 7,
	blue_get_queues_info = 8,
	blue_get_output_fifo_info = blue_get_queues_info,
	blue_get_output_fifo_status = 9,
	blue_start_output_fifo_no_auto_turn_off = 10	/* this is used when we don't really use the FIFO, but handle audio playback ourselves in DirectShow;
													   need to make sure that our HANC output FIFO doesn't turn off audio as there are never any HANC frames to be played */
}blue_output_hanc_ioctl_enum;

typedef enum _blue_input_hanc_ioctl_enum
{
	blue_get_input_hanc_buffer = 0,
	blue_start_input_fifo = 3,
	blue_stop_input_fifo = 4,
	blue_init_input_fifo = 5,
	blue_playthru_input_fifo = 6,
	blue_release_input_hanc_buffer = 7,
	blue_map_input_hanc_buffer = 8,
	blue_unmap_input_hanc_buffer = 9,
	blue_get_info_input_hanc_fifo = 10,
	blue_get_input_rp188 = 11,
	blue_get_input_fifo_status = 12,
}blue_input_hanc_ioctl_enum;

#define HANC_PLAYBACK_INIT				(0x00000001)
#define HANC_PLAYBACK_START				(0x00000002)
#define HANC_PLAYBACK_STOP				(0x00000004)

#define HANC_CAPTURE_INIT				(0x00000010)
#define HANC_CAPTURE_START				(0x00000020)
#define HANC_CAPTURE_STOP				(0x00000040)
#define HANC_CAPTURE_PLAYTHRU			(0x00000080)

typedef enum _EDMACardBufferType
{
	BLUE_CARDBUFFER_IMAGE = 0,
	BLUE_CARDBUFFER_IMAGE_VBI_HANC = 1,
	BLUE_CARDBUFFER_IMAGE_VBI = 2,
	BLUE_CARDBUFFER_AUDIO_OUT = 3,
	BLUE_CARDBUFFER_AUDIO_IN = 4,
	BLUE_CARDBUFFER_HANC = 5,
	BLUE_CARDBUFFER_IMAGE_HANC = 6,
}EDMACardBufferType;

enum enum_blue_dvb_asi_packing_format
{
	enum_blue_dvb_asi_packed_format = 1,	/* In this packing method the asi packets are stored as 188 or 204 bytes */
	enum_blue_dvb_asi_packed_format_with_timestamp = 2,	/* In this packing method the asi packets are stored as (8+188) or (8+204) bytes
														The timestamp is stored at the begininig of the packet, using 8 bytes */
	enum_blue_dvb_asi_256byte_container_format = 3,
	enum_blue_dvb_asi_256byte_container_format_with_timestamp = 4
};

enum enum_blue_dvb_asi_packet_size
{
	enum_blue_dvb_asi_packet_size_188_bytes = 1,
	enum_blue_dvb_asi_packet_size_204_bytes = 2
};

typedef enum _blue_blackgenerator_status
{
	ENUM_BLACKGENERATOR_OFF = 0,            /* producing normal video output */
	ENUM_BLACKGENERATOR_ON = 1,             /* producing black video output  */
	ENUM_BLACKGENERATOR_SDI_SYNC_OFF = 2    /* no valid SDI signal is coming out of our SDI output connector; only available in Epoch ASI firmware */
}blue_blackgenerator_status;

typedef enum _EBlueExternalLtcSource
{
    EXT_LTC_SRC_BREAKOUT_HEADER = 0,    /* default; header on the PCB board/Shield (Epoch only) */
    EXT_LTC_SRC_GENLOCK_BNC =     1,    /* Genlock input BNC connector (Epoch and Kronos) */
    EXT_LTC_SRC_INTERLOCK =       2,    /* Interlock input MMCX connector (Kronos only) */
    EXT_LTC_SRC_STEM_PORT =       3,    /* STEM port (Kronos only) */
}EBlueExternalLtcSource;

/**< use this enumerator for controlling emb audio output properties using the property EMBEDDED_AUDIO_OUTPUT. */
typedef enum _EBlueEmbAudioOutput
{
	blue_emb_audio_enable =              0x01,  /**< Switches off/on the whole HANC output from connecotrs associated with the channel */
	blue_auto_aes_to_emb_audio_encoder = 0x02,  /**< control whether the auto aes to emb thread should be running or not */
	blue_emb_audio_group1_enable =       0x04,  /**< enables group1(ch  0 -  3) emb audio */
	blue_emb_audio_group2_enable =       0x08,  /**< enables group2(ch  4 -  7) emb audio */
	blue_emb_audio_group3_enable =       0x10,  /**< enables group3(ch  8 - 11) emb audio */
	blue_emb_audio_group4_enable =       0x20,  /**< enables group4(ch 12 - 16) emb audio */
	blue_enable_hanc_timestamp_pkt =     0x40
}EBlueEmbAudioOutput;

enum SerialPort_struct_flags
{
	SerialPort_Read = 1,
	SerialPort_Write = 2,
	SerialPort_TX_Queue_Status = 4,
	SerialPort_RX_Queue_Status = 8,
	SerialPort_RX_FlushBuffer = 16,
	SerialPort_RX_IntWait_Return_On_Data = 32,
};

/*	Use these macros for controlling epoch application watch dog settings. The card property EPOCH_APP_WATCHDOG_TIMER can be used to control
the watchdog timer functionality. */
enum enum_blue_app_watchdog_timer_prop
{
	enum_blue_app_watchdog_timer_start_stop = (1 << 31),            /* can be used to enable/disable timer */
	enum_blue_app_watchdog_timer_keepalive = (1 << 30),             /* can be used to reset the timer value */
	enum_blue_app_watchdog_timer_get_present_time = (1 << 29),      /* can query to get the value of the timer */
	enum_blue_app_watchdog_get_timer_activated_status = (1 << 28),  /* can query to get whether the timer has been activated */
	enum_blue_app_watchdog_get_timer_start_stop_status = (1 << 27), /* can query whether the timer has been set. */
	enum_blue_app_watchdog_enable_gpo_on_active = (1 << 26),        /* using this enumerator you can tell the system that when */
                                                                    /* application watchdog timer has expired whether a GPO output should be triggered or not. */
                                                                    /* you can use also use this  enumerator  to select */
                                                                    /* which GPO output should be triggered with this. to use GPO port A pass a value of */
                                                                    /* GPIO_TX_PORT_A when this enumerator is used. */
    enum_blue_hardware_watchdog_enable_gpo = (1 << 25)              /* can be used to enable/disable GPO trigger when hardware watchdog timer has been triggered */
};

enum enum_blue_rs422_port_flags
{
	enum_blue_rs422_port_set_as_slave = (1 << 0)    /*  If this flag is set the RS422 port would be set to slave mode.
                                                        by default port is setup to work in master mode , where it would be acting 
                                                        as master in the transactions. */
};

typedef enum
{
	AUDIO_CHANNEL_LOOPING_OFF		= 0x00000000,   /**< deprecated not used any more */
	AUDIO_CHANNEL_LOOPING			= 0x00000001,   /**< deprecated not used any more */
	AUDIO_CHANNEL_LITTLEENDIAN		= 0x00000000,   /**< if the audio data is little endian this flag must be set*/
	AUDIO_CHANNEL_BIGENDIAN			= 0x00000002,   /**< if the audio data is big  endian this flag must be set*/
	AUDIO_CHANNEL_OFFSET_IN_BYTES	= 0x00000004,   /**< deprecated not used any more */
	AUDIO_CHANNEL_16BIT				= 0x00000008,   /**< if the audio channel bit depth is 16 bits this flag must be set*/
	AUDIO_CHANNEL_BLIP_PENDING		= 0x00000010,   /**< deprecated not used any more */
	AUDIO_CHANNEL_BLIP_COMPLETE		= 0x00000020,   /**< deprecated not used any more */
	AUDIO_CHANNEL_SELECT_CHANNEL	= 0x00000040,   /**< deprecated not used any more */
	AUDIO_CHANNEL_24BIT				= 0x00000080    /**< if the audio channel bit depth is 24 bits this flag must be set*/
} EAudioFlags;





/*///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////// S T R U C T S ////////////////////////////////////////////////////////////////////////////////////////////////////////////*/
#pragma pack(push, video_frame, 1)

struct blue_videoframe_info
{
	BLUE_U64      ltcTimeCode;
	unsigned long videochannel;
	unsigned long BufferId;
	unsigned long Count;
	unsigned long DroppedFrameCount;
};

struct blue_videoframe_info_ex
{
	BLUE_U64		ltcTimeCode;        /* LTC timecode, not used */
	unsigned long	videochannel;       /* the channel this frame was captured from */
	long			BufferId;           /* this buffer contains the captured frame */
	unsigned long	Count;              /* total captured frames */
    unsigned long	DroppedFrameCount;  /* dropped frame count */
	unsigned long	nFrameTimeStamp;    /* field count the frame was captured at */
	unsigned long	nVideoSignalType;   /* video mode of this frame _EVideoMode */
	unsigned int	nASIPktCount;       /* only for DVB-ASI; how many ASI packets are in this frame */
	unsigned int	nASIPktSize;        /* only for DVB-ASI; how many bytes per packet */
	unsigned int	nAudioValidityBits; /* part of the channels status block for audio */
	BLUE_U64		btcTimeStamp;       /* Coordinated Bluefish Time timestamp */
	unsigned char	ucVideoModeLinkA;   /* only used in 1.5G dual link mode */
	unsigned char	ucVideoModeLinkB;   /* only used in 1.5G dual link mode */
    unsigned long	VideoModeExt;       /* video mode of this frame _EVideoModeExt */
	unsigned char	pad[6];             /* not used */
};

typedef struct _AUXILLARY_VIDEO_INFO
{
	BLUE_U32 video_channel_id;
	BLUE_U32 lVideoMode;
	BLUE_U32 lUniqueId;
	BLUE_U32 lInfoType;
	BLUE_U32 lMemFmt;
	BLUE_U32 lGpio;
	BLUE_U64 lLTC;
}Auxillary_Video_Info;

typedef struct
{
	BLUE_S32 inputConnector;    /* ANALOG_VIDEO_INPUT_CONNECTOR, EAnalogInputConnectorType */
	BLUE_S32 inputPed;          /* ANALOG_VIDEO_INPUT_PED, */
	BLUE_S32 inputBrightness;   /* ANALOG_VIDEO_INPUT_BRIGHTNESS, */
	BLUE_S32 inputHue;          /* ANALOG_VIDEO_INPUT_HUE, */
	BLUE_S32 inputLumaGain;     /* ANALOG_VIDEO_INPUT_LUMA_GAIN, */
	BLUE_S32 inputChromaGain;   /* ANALOG_VIDEO_INPUT_CHROMA_GAIN, */
	BLUE_S32 inputAutoGain;     /* ANALOG_VIDEO_INPUT_AUTO_GAIN, */
	BLUE_S32 outputPed;         /* ANALOG_VIDEO_OUTPUT_PED, */
	BLUE_S32 outputBrightness;  /* ANALOG_VIDEO_OUTPUT_BRIGHTNESS, */
	BLUE_S32 outputHue;         /* ANALOG_VIDEO_OUTPUT_HUE, */
	BLUE_S32 outputYGain;       /* ANALOG_VIDEO_OUTPUT_Y_GAIN, */
	BLUE_S32 outputUGain;       /* ANALOG_VIDEO_OUTPUT_U_GAIN, */
	BLUE_S32 outputVGain;       /* ANALOG_VIDEO_OUTPUT_V_GAIN, */
	BLUE_S32 outputSharpness;   /* ANALOG_VIDEO_OUTPUT_SHARPNESS, */
	BLUE_S32 outputAutoGain;    /* ANALOG_VIDEO_OUTPUT_AUTO_GAIN, */
	BLUE_S32 outputSignalTypes; /* EAnalogConnectorSignalType */
}AnalogCardState;

struct SerialPort_struct
{
	unsigned char 	Buffer[64];
	unsigned int	nBufLength;
	unsigned int	nSerialPortId;
	unsigned int	bFlag; /* SerialPort_struct_flags */
	unsigned short	sTimeOut;
};

struct blue_color_matrix_struct
{
	BLUE_U32 VideoChannel;
	BLUE_U32 MatrixColumn;  /* MatrixColType enumerator defines this */
	double Coeff_B;
	double Coeff_R;
	double Coeff_G;
	double Coeff_K;
	double const_value;
};

typedef struct _blue_video_sync_struct
{
	BLUE_U32 sync_wait_type;                    /* field or frame (UPD_FMT_FIELD or UPD_FMT_FRAME) */
	BLUE_U32 video_channel;                     /* which video channel interrupt should the interrupt wait for, e.g. BLUE_VIDEO_INPUT_CHANNEL_A, BLUE_VIDEO_OUTPUT_CHANNEL_A, etc. */
	BLUE_U32 timeout_video_msc;                 /* field count when to return or IGNORE_SYNC_WAIT_TIMEOUT_VALUE to return at next field/frame sync */
	BLUE_U32 video_msc;                         /* current video msc (field count) */
	BLUE_U32 current_display_frame_id;          /* current buffer id which is being displayed */
	BLUE_U32 current_display_frame_uniqueid;    /* unique id associated with current buffer id which is being displayedl; this is only valid when using fifo modes. */
	BLUE_U16 subfield_interrupt;                /* subfield interrupt number; 0 == main frame sync */
	BLUE_U16 subfield_lines;                    /* number of lines of video captured at this subfield interrupt */
	BLUE_U64 btcTimeStamp;                      /* Coordinated Bluefish Time timestamp of field/frame which is currently being displayed */
	BLUE_U8  pad[12];
}blue_video_sync_struct;

typedef struct sync_options
{
    BLUE_U32    SyncWaitType;                   /* required: field or frame (UPD_FMT_FIELD or UPD_FMT_FRAME) */
    BLUE_U32    VideoChannel;                   /* required: */
    BLUE_U32    FieldCountTimeout;              /* field count when to return or IGNORE_SYNC_WAIT_TIMEOUT_VALUE to return at next field/frame sync ( not yet supported on linux) */
    BLUE_U32    FieldCount;                     /* Current FieldCount */
    BLUE_U64    BtcTimeStamp;                   /* BTC at time of interrupt */
    BLUE_U32    SubFieldInterrupt;              /* returned value, input only. subfield interrupt number; 0 == main frame sync */
    BLUE_U32    SubFieldLines;                  /* returned value, input only. number of lines of video captured at this subfield interrupt */
    BLUE_U32    CurrentDisplayFrameID;          /* returned value, output only. current buffer id which is being displayed on the output */
    BLUE_U32    CurrentDisplayFrameUniqueID;    /* returned value, output only. unique id associated with current buffer id which is being displayed on the output */
    BLUE_U64    pad[5];
}sync_options;

typedef struct blue_external_ltc_input_sync_struct
{
	BLUE_U64 TimeCodeValue;
	BLUE_U32 TimeCodeIsValid;
	BLUE_U8  pad[20];
}blue_external_ltc_input_sync_struct;

struct blue_dma_request_struct
{
	unsigned char* pBuffer;
	BLUE_U32 video_channel;
	BLUE_U32 BufferId;
	BLUE_U32 BufferDataType;
	BLUE_U32 FrameType;
	BLUE_U32 BufferSize;
	BLUE_U32 Offset;
	unsigned long	BytesTransferred;
	unsigned char pad[64];
};

struct blue_1d_lookup_table_struct
{
	BLUE_U32 nVideoChannel;
	BLUE_U32 nLUTId;
	BLUE_U16* pLUTData;
	BLUE_U32 nLUTElementCount;
	BLUE_U8 pad[256];
};

struct blue_multi_link_info_struct
{
    BLUE_U32 Link1_Device;
    BLUE_U32 Link1_MemChannel;
    BLUE_U32 Link2_Device;
    BLUE_U32 Link2_MemChannel;
    BLUE_U32 Link3_Device;
    BLUE_U32 Link3_MemChannel;
    BLUE_U32 Link4_Device;
    BLUE_U32 Link4_MemChannel;
    BLUE_U32 Link5_Device;
    BLUE_U32 Link5_MemChannel;
    BLUE_U32 Link6_Device;
    BLUE_U32 Link6_MemChannel;
    BLUE_U32 Link7_Device;
    BLUE_U32 Link7_MemChannel;
    BLUE_U32 Link8_Device;
    BLUE_U32 Link8_MemChannel;
    BLUE_U32 Link9_Device;
    BLUE_U32 Link9_MemChannel;
    BLUE_U32 Link10_Device;
    BLUE_U32 Link10_MemChannel;
    BLUE_U32 Link11_Device;
    BLUE_U32 Link11_MemChannel;
    BLUE_U32 Link12_Device;
    BLUE_U32 Link12_MemChannel;
    BLUE_U32 Link13_Device;
    BLUE_U32 Link13_MemChannel;
    BLUE_U32 Link14_Device;
    BLUE_U32 Link14_MemChannel;
    BLUE_U32 Link15_Device;
    BLUE_U32 Link15_MemChannel;
    BLUE_U32 Link16_Device;
    BLUE_U32 Link16_MemChannel;
    BLUE_U32 InputControl;
    BLUE_U32 Padding[15];
};

struct hanc_stream_info_struct
{
    BLUE_S32 AudioDBNArray[4];			    /**< Contains the DBN values that should be used for each of the embedded audio groups*/
    BLUE_S32 AudioChannelStatusBlock[4];	/**< channel status block information for each of the embedded audio group*/
    BLUE_U32 flag_valid_time_code;		    /**< deprecated/unused flag; set to 0*/
    BLUE_U64 time_code; 					/**< RP188 time code that was extracted from the HANC buffer or RP188 timecode which should be inserted into the HANC buffer*/
    BLUE_U32* hanc_data_ptr;				/**< Hanc Buffer which should be used as the source or destination for either extraction or insertion */
    BLUE_U32 video_mode;					/**< video mode which this hanc buffer which be used with. We need this information for do the required audio distribution especially NTSC */
    BLUE_U64 ltc_time_code;
    BLUE_U64 sd_vitc_time_code;
    BLUE_U64 rp188_ltc_time_code;
    BLUE_U32 pad[126];
};

struct hanc_decode_struct
{
    BLUE_VOID* audio_pcm_data_ptr;               /* Buffer which would be used to store the extracted PCM audio data. Must be filled in by app before calling function. */
    BLUE_U32 audio_ch_required_mask;             /* Defines which audio channels should be extracted; Use enumerator BlueAudioChannelDesc to set up this mask. Must be filled in by app before calling function. */
    BLUE_U32 type_of_sample_required;            /* Defines sample characteristics:
                                                 AUDIO_CHANNEL_16BIT: for 16 bit pcm data
                                                 AUDIO_CHANNEL_24BIT: for 24 bit pcm data
                                                 If neither AUDIO_CHANNEL_16BIT nor AUDIO_CHANNEL_24BIT are set 32 bit pcm data will be extracted
                                                 Must be filled in by app before calling function. */
    BLUE_U32 no_audio_samples;		            /* number of audio samples that have decoded the hanc buffer */
    BLUE_U64 timecodes[7];			            /* Only the first four elements are currently defined:
                                                hanc_decode_struct::timcodes[0] ---> RP188 VITC timecode
                                                hanc_decode_struct::timcodes[1] ---> RP188 LTC timecode
                                                hanc_decode_struct::timcodes[2] ---> SD VITC timecode
                                                hanc_decode_struct::timcodes[3] ---> External LTC timecode */
    BLUE_VOID* raw_custom_anc_pkt_data_ptr;     /* This buffer  would contain the raw ANC packets that was found in the orac hanc buffer.
                                                this would contain any ANC packets that is not of type embedded audio and RP188 TC.
                                                Must be filled in by app before calling function. can be NULL */
    BLUE_U32 sizeof_custom_anc_pkt_data_ptr;    /* size of the ANC buffer array; Must be filled in by app before calling function. can be NULL */
    BLUE_U32 avail_custom_anc_pkt_data_bytes;   /* how many custom ANC packets has been decoded into raw_hanc_pkt_data_ptr; Must be filled in by app before calling function. can be NULL */
    BLUE_U32 audio_input_source;                /* Used to select the audio input source, whether it is AES or Embedded. Must be filled in by app before calling function. */
    BLUE_U32 audio_temp_buffer[16];             /* deprecated/not used; Must be initialised to zero by app before first instantiating the function. */
    BLUE_U32 audio_split_buffer_mask;           /* deprecated/not used; Must be initialised to zero by app before first instantiating the  function. */
    BLUE_U32 max_expected_audio_sample_count;   /* specify the maximum number of audio samples that the provided audio pcm buffer can contain. Must be filled in by app before calling function. */
    BLUE_U32 pad[124];
};

typedef struct hardware_firmware_versions
{
    BLUE_U32 FirmwareMajor;
    BLUE_U32 FirmwareMinor;
    BLUE_U32 FirmwareBuild;
    BLUE_U32 FirmwareLoadedFrom;

    BLUE_U32 CpldConfig;
    BLUE_U32 CpldStemPort;
    BLUE_U32 CpldIOExpander;

    BLUE_U32 HardwareBaseBoardFamily;
    BLUE_U32 HardwareBaseBoardModel;
    BLUE_U32 HardwareBaseBoardRevision;
    BLUE_U32 HardwareBaseBoardBuildMajor;
    BLUE_U32 HardwareBaseBoardBuildMinor;
    BLUE_U32 HardwareRegulatorBoardFamily;
    BLUE_U32 HardwareRegulatorBoardModel;
    BLUE_U32 HardwareRegulatorBoardRevision;
    BLUE_U32 HardwareRegulatorBoardBuildMajor;
    BLUE_U32 HardwareRegulatorBoardBuildMinor;

    BLUE_U32 Padding[12];
}hardware_firmware_versions;

#pragma pack(pop, video_frame)

/*///////////////////////////////////////////////////////////////////////////
// L E G A C Y   D E C L A R A T I O N S - END
///////////////////////////////////////////////////////////////////////////*/



#endif /* HG_BLUE_DRIVER_P_LEGACY_HG */
