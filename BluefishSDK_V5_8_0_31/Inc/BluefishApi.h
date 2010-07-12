// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the BLUEC_API_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// BLUEC_API_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef BLUEFISH_EXPORTS
#define BLUEFISH_API __declspec(dllexport)
#else
#define BLUEFISH_API __declspec(dllimport)
#endif
#include "BlueDriver_p.h"


#define IGNORE_SYNC_WAIT_TIMEOUT_VALUE	(0xFFFFFFFF)
typedef void * BLUEFISHAPI_HANDLE;
typedef int BErr;

extern "C" {


struct bluefishapi_card_feature
{
	BLUE_UINT32 card_id;
	BLUE_UINT32 video_output_channel;
	BLUE_UINT32 sdi_output_connector;
	BLUE_UINT32 output_analog_connector;
	BLUE_UINT32 no_video_input_channel;
	BLUE_UINT32 no_sdi_input_connector;
	BLUE_UINT32 input_analog_connector;
	BLUE_UINT32 digital_audio_output_connector;
	BLUE_UINT32 digital_audio_input_connector;
	BLUE_UINT32 analog_audio_output_connector;
	BLUE_UINT32 analog_audio_input_connector;
	BLUE_UINT32 pad[128];
};

BLUEFISH_API unsigned int bluefishapi_device_count();
BLUEFISH_API void * bluefishapi_attach(int device_id,unsigned int *card_type);
BLUEFISH_API BErr bluefishapi_detach(void ** device_handle);

/**
 * These function can be used set/get various card property
 * Also the is_prop_supported function can be used to check if the particular property is 
 * supported on that card or not.
 * This includes the video property and the video connection/routing property.
 */
BLUEFISH_API BErr bluefishapi_set_card_property(BLUEFISHAPI_HANDLE  device_handle,unsigned int video_channel,int card_prop,void *value);
BLUEFISH_API BErr bluefishapi_get_card_property(BLUEFISHAPI_HANDLE  device_handle,unsigned int video_channel,int card_prop,void *value);
BLUEFISH_API bool bluefishapi_is_prop_supported(BLUEFISHAPI_HANDLE  device_handle,unsigned int video_channel,int card_prop,int *no_parameters,int *parameter_type);
/*
BLUEFISH_API BErr	bluefishapi_get_connector_property(BLUEFISHAPI_HANDLE device_handle,
												BLUE_UINT32 VideoChannel,
												BLUE_UINT32 *settingsCount,	// In: element count of settings array
																// Out: if settings is non-NULL, number of valid elements
																// Out: if settings is NULL, number of elements required
												EBlueConnectorPropertySetting *settings // Caller allocates/frees memory
												);

BLUEFISH_API BErr	bluefishapi_set_connector_property(
												BLUEFISHAPI_HANDLE device_handle,
												BLUE_UINT32 settingsCount,
												EBlueConnectorPropertySetting *settings
												);
*/
/**
 * These function can be used to transfer frames in and out of the 
 * card using the DMA engine on the card.
 *
 * These functions are already there in BlueVelvet4.h , wanted to check if you are ok with this interface.
 * In BlueVelvet4.h instead of BLUE_DEVICE_HANDLE being passed as first parameter it is passing a pointer to
 * CBlueVelvet4.
 *
 */
BLUEFISH_API BErr bluefishapi_dma_ex(	BLUEFISHAPI_HANDLE  device_handle,
										BLUE_UINT32 dma_direction,
										struct blue_dma_request_struct  *user_dma_request_ptr);

BLUEFISH_API BErr bluefishapi_dma_async(	BLUEFISHAPI_HANDLE  device_handle,
											BLUE_UINT32 dma_direction,
											LPOVERLAPPED overlap_ptr,
											struct blue_dma_request_struct  *user_dma_request_ptr);

BErr bluefishapi_dma(	BLUEFISHAPI_HANDLE  device_handle,
						BLUE_UINT32 dma_direction,
						void * pFrameBuffer,
						BLUE_UINT32 video_channel,
						BLUE_UINT32 FrameSize,
						BLUE_UINT32 BufferId,
						BLUE_UINT32 BufferDataType,
						BLUE_UINT32 FrameType,
						BLUE_UINT32 CardFrameOffset);


/**
 * wait for sync functions
 * 
 * These functions are already there in BlueVelvet4.h , wanted to check if you are ok with this interface.
 * In BlueVelvet4.h instead of BLUE_DEVICE_HANDLE being passed as first parameter it is passing a pointer to
 * CBlueVelvet4.
 *
 * The rational for these function is it will help us pass more information with these function 
 * in the future. The blue_video_sync_struct has a member called pad which can be used to add more member variables 
 * with out changing the signature .
 */ 


BLUEFISH_API BErr bluefishapi_wait_video_sync_ex(BLUEFISHAPI_HANDLE device_handle,blue_video_sync_struct * sync_struct);

BLUEFISH_API BErr bluefishapi_wait_video_sync_async(BLUEFISHAPI_HANDLE device_handle,
													LPOVERLAPPED overlap_ptr,
													blue_video_sync_struct * sync_struct);

BLUEFISH_API BErr bluefishapi_wait_video_sync(		BLUEFISHAPI_HANDLE device_handle,
													BLUE_UINT32 video_channel,
													BLUE_UINT32 upd_type,
													BLUE_UINT32 *field_count);

BLUEFISH_API BErr bluefishapi_complete_async_req(BLUEFISHAPI_HANDLE device_handle,LPOVERLAPPED overlap_ptr);


/**
 * video fifo functions
 *
 * These are a one to one mapping of the C++ functions
 *
 */
BLUEFISH_API BErr bluefishapi_set_video_engine(BLUEFISHAPI_HANDLE device_handle,
									BLUE_UINT32 video_channel,
									BLUE_UINT32 *video_engine ,
									BLUE_UINT32 has_playthru,
									BLUE_UINT32 playthru_video_channel);

BLUEFISH_API BErr bluefishapi_get_free_playback_frame(void * device_handle,BLUE_UINT32  video_channel,BLUE_INT32 *BufferId,BLUE_UINT32 * Underrun);
BLUEFISH_API BErr bluefishapi_present_playback_frame(	void * device_handle,
											BLUE_UINT32  video_channel,
											BLUE_UINT32 video_repeat_count,
											BLUE_UINT32 video_frame_flag,
											BLUE_INT32  BufferId,
											BLUE_UINT32 FrameType,
											BLUE_UINT32 * unique_id);


BLUEFISH_API BErr	bluefishapi_video_playback_start(	BLUEFISHAPI_HANDLE device_handle,
														int video_channel,		
														int		Step,
														int		Loop) ;

BLUEFISH_API BErr	bluefishapi_video_playback_stop(	BLUEFISHAPI_HANDLE device_handle,
											int video_channel,		
											int		Wait,
											int		Flush) ;

BLUEFISH_API BErr video_capture_harvest(	BLUEFISHAPI_HANDLE device_handle,
											int video_channel,
											int	* BufferId,
											unsigned int *	Count,
											unsigned int*	Frames);

BLUEFISH_API BErr	video_capture_start(	BLUEFISHAPI_HANDLE device_handle,
											int video_channel);

BLUEFISH_API BErr bluefishapi_get_capture_video_frame(	void * device_handle,
											BLUE_UINT32  video_channel,
											blue_videoframe_info_ex * frame_info,
											BLUE_UINT32 *dropped_frame_count,
											BLUE_UINT32 *current_fifo_size,
											BLUE_UINT32 flag);

BLUEFISH_API BErr	video_capture_stop(		BLUEFISHAPI_HANDLE device_handle,int video_channel);
BLUEFISH_API BErr video_playback_flush(	BLUEFISHAPI_HANDLE device_handle,int video_channel);


/*audio capture*/
BLUEFISH_API BErr	bluefishapi_start_audio_capture(BLUEFISHAPI_HANDLE device_handle,
													BLUE_UINT32  video_channel,
													BLUE_UINT32 sync_start_count);

BLUEFISH_API BErr	bluefishapi_stop_audio_capture(BLUEFISHAPI_HANDLE device_handle,BLUE_UINT32  video_channel);

BLUEFISH_API BErr	bluefishapi_wait_audio_input_interrupt(BLUEFISHAPI_HANDLE device_handle,
														   BLUE_UINT32 video_channel,
														   BLUE_UINT32 * queued_sample_count,
														   BLUE_UINT32 * free_sample_space);

BLUEFISH_API BErr	bluefishapi_read_audio_sample(	BLUEFISHAPI_HANDLE device_handle,
													BLUE_UINT32 video_channel,
													void * pBuffer,
													BLUE_UINT32 read_sample_count,
													BLUE_UINT32 audio_channel_select_mask,
													BLUE_UINT32 audio_sample_flags);

/*audio playback*/
BLUEFISH_API BErr	bluefishapi_wait_audio_output_interrupt(BLUEFISHAPI_HANDLE device_handle,
															BLUE_UINT32 video_channel,
															BLUE_UINT32 * queued_sample_count,
															BLUE_UINT32 * free_sample_space);

BLUEFISH_API BErr   bluefishapi_open_playback_engine(BLUEFISHAPI_HANDLE device_handle);
BLUEFISH_API BErr	bluefishapi_write_audio_sample(	BLUEFISHAPI_HANDLE device_handle,
													BLUE_UINT32 video_channel,
													void * pBuffer,
													BLUE_UINT32 write_sample_count,
													BLUE_UINT32 audio_channel_select_mask,
													BLUE_UINT32 audio_sample_flags);

BLUEFISH_API BErr   bluefishapi_close_playback_engine(BLUEFISHAPI_HANDLE device_handle);
BLUEFISH_API BErr bluefishapi_start_audio_playback(	BLUEFISHAPI_HANDLE device_handle,int syncCount);
BLUEFISH_API BErr bluefishapi_stop_audio_playback(	BLUEFISHAPI_HANDLE device_handle);

BLUEFISH_API BErr bluefishapi_get_hancoutput_buffer(void * device_handle,UINT32 video_channel);
BLUEFISH_API BErr bluefishapi_put_hancoutput_buffer(void * device_handle,int hanc_buffer_id,UINT32 video_channel);
BLUEFISH_API BErr bluefishapi_hancoutput_fifo_control(void * device_handle,
						  UINT32 video_channel,
						  UINT32 control);
BLUEFISH_API BErr blue_control_pciconfig_space(
											void * device_handle,
											BLUE_UINT32 configspace_offset,
											BLUE_UINT32 * configspace_value,
											BLUE_UINT32 bReadFlag);
BLUEFISH_API BErr bluefishapi_control_pcicontrol_reg(
											void * device_handle,
											BLUE_UINT32 bar_id,
											BLUE_UINT32 controlreg_offset,
											BLUE_UINT32 * controlreg_value,
											BLUE_UINT32 bReadFlag);
}