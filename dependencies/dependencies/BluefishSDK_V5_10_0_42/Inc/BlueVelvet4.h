#ifndef	_BLUEVELVET4_H
#define	_BLUEVELVET4_H

#include "BlueVelvet.h"
#include "BlueC_Api.h"
typedef struct 
{
	VARIANT maxRange;
	VARIANT minRange;
	VARIANT currentValue;
	unsigned long uniqueSteps;
}AnalogPropertyValue;


class BLUEFISH_API CBlueVelvet4:  virtual  public CBlueVelvet 
{
public:
	// Functions for new Analog Card Property 
	virtual BErr SetAnalogCardProperty(int propType,VARIANT & propValue)=0;
	virtual BErr GetAnalogCardProperty(int propType,AnalogPropertyValue & propValue)=0;
	virtual BErr GetAnalogCardProperty(int propType,VARIANT & propValue)=0;

	virtual BErr has_analog_connector(int	DeviceId) = 0;
	virtual int has_svideo_input(int	DeviceId) = 0;
	virtual int has_component_input(int	DeviceId)=0;
	virtual int	has_composite_input(int	DeviceId)=0;
	virtual int	has_svideo_output(int	DeviceId)=0;
	virtual int	has_component_output(int	DeviceId)=0;
	virtual int	has_analog_rgb_output(int	DeviceId)=0;
	virtual int	has_composite_output(int	DeviceId)=0;

	// Functions for new Audio Input architecture
	// Functions for Future use not implemented 
	virtual BErr wait_audio_input_interrupt(unsigned long & noQueuedSample,unsigned long & noFreeSample) = 0;
	virtual BErr InitAudioCaptureMode()=0;
	virtual BErr StartAudioCapture(int syncCount)=0;
	virtual BErr StopAudioCapture()=0;
	virtual BErr ReadAudioSample(int nSampleType,void * pBuffer,long  nNoSample,int bFlag,long nNoSamplesWritten)=0;
	virtual BErr EndAudioCaptureMode()=0; 

	 
	virtual BErr SetCardProperty(int propType,VARIANT & Value)=0;
	virtual BErr QueryCardProperty(int propType,VARIANT & Value)=0;

	// RS422 Serial Port Functions
	// Functions for Future use not implemented 
	virtual BErr	Wait_For_SerialPort_InputData(unsigned long   bFlag,unsigned long & NoDataAvailable)=0;
	virtual int 	IsSerialPort_OutputBuffer_Full(unsigned long bFlag)=0;
	virtual int		Read_From_SerialPort(unsigned long bFlag,unsigned char * pBuffer,unsigned long ReadLength)=0;
	virtual int 	Write_To_SerialPort(unsigned long bFlag,unsigned char * pBuffer,unsigned long WriteLength)=0;
};




extern "C" {
BLUEFISH_API CBlueVelvet4*	BlueVelvetFactory4();
BLUEFISH_API void BlueVelvetDestroy(CBlueVelvet4* pObj);
// this would give you number of VANC/VBI line 
BLUEFISH_API unsigned int BlueVelvetVANCLineCount(unsigned int CardType,unsigned long VidFmt,unsigned long FrameType);

// this would give you golden value for the VANC frame size
BLUEFISH_API unsigned int BlueVelvetVANCGoldenValue(	unsigned int CardType,
													unsigned long VidFmt,
													unsigned long MemFmt,
													unsigned long FrameType);

// this would give you number of bytes contained in a VANC line 
BLUEFISH_API unsigned int BlueVelvetVANCLineBytes(	unsigned int CardType,
													unsigned long	VidFmt,
													unsigned long	MemFmt);

BLUEFISH_API unsigned int BlueVelvetBytesForGroupPixels(unsigned long MemFmt,unsigned int nPixelCount);
BLUEFISH_API BErr SetVideo_MetaDataInfo(CBlueVelvet4 * pSdk,LPOVERLAPPED pOverLap,ULONG FrameId,ULONG prop,VARIANT value);

BLUEFISH_API BErr GetVideo_CaptureFrameInfo(CBlueVelvet4 * pSdk,LPOVERLAPPED pOverlap,struct blue_videoframe_info & video_capture_frame,int	CompostLater);
BLUEFISH_API BErr GetVideo_CaptureFrameInfoEx(CBlueVelvet4 * pSdk,LPOVERLAPPED pOverlap,struct blue_videoframe_info_ex & video_capture_frame,int	CompostLater,unsigned int *capture_fifo_size);

BLUEFISH_API int GetHancQueuesInfo(CBlueVelvet4 * pSdk, LPOVERLAPPED pOverlap, UINT32 video_channel, UINT32* pFreeBuffers, UINT32* pMaximumBuffers, UINT32* pStatus);
BLUEFISH_API int GetHancBuffer(CBlueVelvet4 * pSdk,LPOVERLAPPED pOverlap,UINT32 video_channel);
BLUEFISH_API BERR PutHancBuffer(CBlueVelvet4 * pSdk,LPOVERLAPPED pOverlap,int hanc_buffer_id,UINT32 video_channel);
BLUEFISH_API BERR HancInputFifoControl(CBlueVelvet4 * pSdk,LPOVERLAPPED pOverlap,UINT32 video_channel, UINT32 control);
BLUEFISH_API BERR HancOutputFifoControl(CBlueVelvet4 * pSdk,LPOVERLAPPED pOverlap,UINT32 video_channel, UINT32 control);
BLUEFISH_API int GetHancInputBuffer(CBlueVelvet4* pSdk,LPOVERLAPPED pOverlap,UINT32 video_channel,UINT32* signal_type, UINT32* field_count);
BLUEFISH_API BLUE_UINT32 emb_audio_decoder( BLUE_UINT32 * src_hanc_buffer,
										    void  * dest_data_ptr,
										    BLUE_UINT32 req_audio_sample_count,
										    BLUE_UINT32 required_audio_channels,
											BLUE_UINT32 sample_type);
BLUEFISH_API BERR blue_wait_video_sync_async(CBlueVelvet4 * pSdk,
											LPOVERLAPPED pOverlap,
											blue_video_sync_struct * sync_struct);
BLUEFISH_API BERR blue_dma_read_async(	CBlueVelvet4 * pSdk,
										LPOVERLAPPED pOverlap,
										struct blue_dma_request_struct  *pUserDmaRequest);


BLUEFISH_API BERR blue_load_1D_lookup_table(CBlueVelvet4 * pSdk, struct blue_1d_lookup_table_struct * lut);

BLUEFISH_API BERR blue_control_video_scaler(CBlueVelvet4 * pSdk, unsigned int nScalerId,
											bool bOnlyReadValue,
											float *pSrcVideoHeight,
											float *pSrcVideoWidth,
											float *pSrcVideoYPos,
											float *pSrcVideoXPos,
											float *pDestVideoHeight,
											float *pDestVideoWidth,
											float *pDestVideoYPos,
											float *pDestVideoXPos);

BLUEFISH_API BERR blue_Epoch_GetTimecodes(CBlueVelvet4 * pSdk, UINT32 VideoChannel, UINT64* pArray, UINT32* FieldCount);

BLUEFISH_API unsigned int count_scaler_video_mode(CBlueVelvet4 * pSdk,
													int device_id,
													unsigned int video_channel,
													unsigned int video_mode);
BLUEFISH_API EVideoMode enum_scaler_video_mode(CBlueVelvet4 * pSdk,
												 int device_id,
												 unsigned int video_channel,
												 unsigned int video_mode,
												 unsigned int index);
BLUEFISH_API BERR blue_video_scaler_filter_coefficent(	CBlueVelvet4 * pSdk,
											unsigned int nScalerId,
											bool bOnlyReadValue,
											unsigned int nFilterType,
											float *pCoefficentWeightArray,
											unsigned int nArrayElementCount
											);
BLUEFISH_API BERR blue_audioplayback_pause(CBlueVelvet4 * pSdk);
BLUEFISH_API BERR blue_audioplayback_resume(CBlueVelvet4 * pSdk, int syncCount);
BLUEFISH_API BERR GetHancQueuesInfoEx(CBlueVelvet4 * pSdk, 
						LPOVERLAPPED pOverlap, 
						UINT32 video_channel, 
						UINT32* pFreeBuffers, 
						UINT32* pMaximumBuffers,
						UINT32 * pStatus,
						UINT32 * pSamplesUsed,
						UINT64 *pStartHancFifoTimeStamp,
						UINT64 *pVideoSyncTimeStamp);


BLUEFISH_API BERR blue_color_matrix(CBlueVelvet4 * pSdk,bool bGetValue,blue_color_matrix_struct * color_matrix_ptr);
}


#endif //_BLUEVELVET4_H