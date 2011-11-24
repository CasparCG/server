// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the BLUEC_API_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// BLUEC_API_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef BLUEFISH_EXPORTS
#define BLUE_CSDK_API __declspec(dllexport)
#else
#define BLUE_CSDK_API __declspec(dllimport)
#endif
#include "BlueTypes.h"
#include "BlueDriver_p.h"

#define IGNORE_SYNC_WAIT_TIMEOUT_VALUE	(0xFFFFFFFF)


#pragma once

//
//typedef enum _blue_video_engine_flags
//{
//	BLUE_CAPTURE_PREVIEW_ON_OUTPUT_CHANNEL_A=0x1,
//	BLUE_CAPTURE_PREVIEW_ON_OUTPUT_CHANNEL_B=0x2,
//	BLUE_CAPTURE_PREVIEW_ON_OUTPUT_CHANNEL_C=0x3,
//	BLUE_CAPTURE_PREVIEW_ON_OUTPUT_CHANNEL_D=0x4
//}blue_video_engine_flags;
//
typedef enum _EBLUEDMA_DIRECTION
{
	BLUEDMA_WRITE=0,
	BLUEDMA_READ=1,
	BLUEDMA_INVALID=3
}EBLUEDMA_DIRECTION;

typedef struct _EBlueConnectorPropertySetting
{
	EBlueVideoChannel channel;
	EBlueConnectorIdentifier connector;
	EBlueConnectorSignalDirection	signaldirection;
	EBlueConnectorProperty propType;
	VARIANT Value;
}EBlueConnectorPropertySetting;


typedef struct _blue_card_property
{
	EBlueVideoChannel	video_channel;
	EBlueCardProperty	prop;
	VARIANT  value;
}blue_card_property;


extern "C" {
BLUE_CSDK_API unsigned int blue_device_count();
BLUE_CSDK_API void * blue_attach_to_device(int device_id);
BLUE_CSDK_API BERR blue_detach_from_device(void ** device_handle);

//
//BLUE_CSDK_API BERR blue_set_video_engine(		void * device_handle,
//											EBlueVideoChannel video_channel,
//											EEngineMode * video_engine,
//											BLUE_UINT32 video_engine_flag);
BLUE_CSDK_API BERR blue_video_dma(			void * device_handle,
											EBLUEDMA_DIRECTION	dma_direction,
											void * pFrameBuffer,
											BLUE_UINT32 FrameSize,
											BLUE_UINT32 BufferId,
											BLUE_UINT32 * dma_bytes_transferred,
											BLUE_UINT32 CardFrameOffset,
											OVERLAPPED	* pAsync
									);


/*
	functions used to set the card property . This includes the video property 
	and the video connection/routing property.
*/
BLUE_CSDK_API BERR	blue_set_video_property(		void * device_handle,
											BLUE_UINT32 prop_count,
											blue_card_property * card_prop);

BLUE_CSDK_API BERR	blue_get_video_property(		void * device_handle,
											BLUE_UINT32 prop_count,
											blue_card_property * card_prop);

BLUE_CSDK_API BERR	blue_get_connector_property(void * device_handle,
									BLUE_UINT32 VideoChannel,
									BLUE_UINT32 *settingsCount,	// In: element count of settings array
																// Out: if settings is non-NULL, number of valid elements
																// Out: if settings is NULL, number of elements required
									EBlueConnectorPropertySetting *settings // Caller allocates/frees memory
									);

BLUE_CSDK_API BERR	blue_set_connector_property(
											void * device_handle,
											BLUE_UINT32 settingsCount,
											EBlueConnectorPropertySetting *settings
											);

//BLUE_CSDK_API BERR blue_wait_video_interrupt(void * device_handle,
//											EBlueVideoChannel  video_channel,
//											EUpdateMethod upd_fmt,
//											BLUE_UINT32 field_wait_count,
//											BLUE_UINT32 *field_count
//											);
}