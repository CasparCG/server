/*
// ==========================================================================
//	Bluefish444 BlueVelvet SDK library
//
//  BlueVelvet.h
//  Public Header
//
//  developed by  : Cameron Duffy   (C) 2002 Bluefish444 P/L
//
//  derived from work begun by Vizrt Austria (C) 2001.
//
// ==========================================================================

  $Id: BlueVelvet.h,v 1.32.8.1 2011/08/04 03:34:36 tim Exp $
*/
#ifndef	_BLUEVELVET_H
#define	_BLUEVELVET_H

#ifdef BLUEFISH_EXPORTS
#define BLUEFISH_API __declspec(dllexport)
#else
#define BLUEFISH_API __declspec(dllimport)
#endif

//#include "BlueVelvet_c.h"

#define BLUE_UINT32  	unsigned int 
#define BLUE_INT32  	int 
#define BLUE_UINT8 	unsigned char
#define BLUE_INT8 	char
#define BLUE_UINT16	unsigned short
#define BLUE_INT16	short
#define BLUE_UINT64	unsigned __int64


#ifndef BLUEVELVET_2_DLL
#define BLUEVELVET_SDK_VERSION3
#endif

#include "BlueDriver_p.h"


//----------------------------------------------------------------------------
// Some simple macros and definitions
#define	BLUEVELVET_MAX_DEVICES	(5)			// Maximum number of Blue Cards recognised by driver

typedef int BErr;

#define	BLUE_OK(a)				(!a)		// Test for succcess of a method returning BErr
#define	BLUE_FAIL(a)			(a)			// Test for failure of a method returning BErr
#define	BLUE_PASS(a)			(a>=0)		// Use this where +ve return values still indicate success


//----------------------------------------------------------------------------
// The class definition
class BLUEFISH_API CBlueVelvet
{
public:
	//	4.1		Startup Functions
	//---------------------------------
	//	4.1.1	device_enumerate
	//			Counts accessible blue cards in target system. 
	virtual
	BErr	device_enumerate(
					int& Devices
					) = 0;

	//	4.1.2	device_attach
	//			Attach the class instance to the indexed device. 
	virtual
	BErr	device_attach(
					int			DeviceId,
					int			do_audio	// DEPRECATED; SET TO 0
					) = 0;

	//	4.1.3	device_detach
	//			Detach the current device from the class instance.
	virtual
	BErr	device_detach(
					void
					) = 0;

	//	4.1.4	device_attach_audio
	//			Attach the class instance to the audio I/O component of the current device.
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	device_attach_audio(
					void
					) = 0;

	//	4.1.5	device_detach_audio
	//			Remove audio I/O components of the current device from the class instance.
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	device_detach_audio(
					void
					) = 0;

	//	4.1.6	device_attach_audio_in
	//			Attach the class instance to the audio IINPUT component of the current device.
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	device_attach_audio_in(
					void
					) = 0;

	//	4.1.7	device_detach_audio_in
	//			Remove audio INPUT component of the current device from the class instance.
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	device_detach_audio_in(
					void
					) = 0;

	//	4.1.8	device_attach_audio_out
	//			Attach the class instance to the audio OUTPUT component of the current device.
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	device_attach_audio_out(
					void
					) = 0;

	//	4.1.9	device_detach_audio_out
	//			Remove audio OUTPUT component of the current device from the class instance. 
	// DEPRECATED; DO NOT USE!
    virtual
	BErr	device_detach_audio_out(
					void
					) = 0;

	//	4.1.10	device_get_bar
	//			Get device Bar assets from driver
    virtual
	BErr	device_get_bar(
					unsigned long	BarN,
					void**			ppAddress,
					unsigned long&	Length
					) = 0;



	//	4.2		Feature Assessment Functions
	//---------------------------------
	//	4.2.1	has_timing_adjust
	virtual
	int		has_timing_adjust(
					int	DeviceId=0
					) = 0;

	//	4.2.2	has_vertical_flip
	virtual
	int		has_vertical_flip(
					int	DeviceId=0
					) = 0;

	//	4.2.3	has_half_res
	virtual
	int		has_half_res(
					int	DeviceId=0
					) = 0;

	//	4.2.4	has_dissolve
	virtual
	int		has_dissolve(
					int	DeviceId=0
					) = 0;

	//	4.2.5	has_aperture
	virtual
	int		has_aperture(
					int	DeviceId=0
					) = 0;

	//	4.2.6	has_input_sdi
	virtual
	int		has_input_sdi(
					int	DeviceId=0
					) = 0;

	//	4.2.7	has_output_sdi
	virtual
	int		has_output_sdi(
					int	DeviceId=0
					) = 0;

	//	4.2.8	has_input_composite
	virtual
	int		has_input_composite(
					int	DeviceId=0
					) = 0;

	//	4.2.9	has_output_composite
	virtual
	int		has_output_composite(
					int	DeviceId=0
					) = 0;

	//	4.2.10	has_input_yuv
	virtual
	int		has_input_yuv(
					int	DeviceId=0
					) = 0;

	//	4.2.11	has_output_yuv
	virtual
	int		has_output_yuv(
					int	DeviceId=0
					) = 0;

	//	4.2.12	has_output_rgb
	virtual
	int		has_output_rgb(
					int	DeviceId=0
					) = 0;

	//	4.2.13	has_input_svideo
	virtual
	int		has_input_svideo(
					int	DeviceId=0
					) = 0;

	//	4.2.14	has_output_svideo
	virtual
	int		has_output_svideo(
					int	DeviceId=0
					) = 0;

	//	4.2.15	has_output_key
	virtual
	int		has_output_key(
					int	DeviceId=0
					) = 0;

	//	4.2.16	has_output_key_v4444
	virtual
	int		has_output_key_v4444(
					int	DeviceId=0
					) = 0;

	//	4.2.17	has_letterbox
	virtual
	int		has_letterbox(
					int	DeviceId=0
					) = 0;

	//	4.2.18	has_video_memory
	virtual
	int		has_video_memory(
					int	DeviceId=0
					) = 0;

	//	4.2.18	has_video_memory_base
	virtual
	int		has_video_memory_base(
					int	DeviceId=0
					) = 0;

	//	4.2.19	has_video_cardtype
	virtual
	int		has_video_cardtype(
					int	DeviceId=0
					) = 0;

	//	4.2.20	count_video_mode
	virtual
	int		count_video_mode(
					int	DeviceId=0
					) = 0;

	//	4.2.21	enum_video_mode
	virtual
	EVideoMode enum_video_mode(
					int Index,
					int	DeviceId=0
					) = 0;

	//	4.2.22	count_memory_format
	virtual
	int		count_memory_format(
					int	DeviceId=0
					) = 0;

	//	4.2.23	enum_memory_format
	virtual
	EMemoryFormat enum_memory_format(
					int	Index,
					int	DeviceId=0
					) = 0;

	//	4.2.24	count_update_method
	virtual
	int		count_update_method (
					int	DeviceId=0
					) = 0;

	//	4.2.25	enum_update_method
	virtual
	EUpdateMethod enum_update_method(
					int	Index,
					int	DeviceId=0
					) = 0;

	//	4.2.26	has_audio_input
	virtual
	int		has_audio_input(
					int	DeviceId=0
					) = 0;

	//	4.2.27	has_audio_output
	virtual
	int		has_audio_output(
					int	DeviceId=0
					) = 0;

	//	4.2.28	count_audio_input_rate
	virtual
	int		count_audio_input_rate(
					int	DeviceId=0
					) = 0;

	//	4.2.29	count_audio_output_rate
	virtual
	int		count_audio_output_rate(
					int	DeviceId=0
					) = 0;

	//	4.2.30	enum_audio_input_rate
	//			Returns the enumeration for the Ith supported audio input rate.
	virtual
	EAudioRate enum_audio_input_rate(
					int	Index,
					int	DeviceId=0
					) = 0;

	//	4.2.31	enum_audio_output_rate
	//			Returns the enumeration for the Ith supported audio output rate.
	virtual
	EAudioRate enum_audio_output_rate(
					int	Index,
					int	DeviceId=0
					) = 0;


	//	4.2.32	has_audio_playthru
	virtual
	int		has_audio_playthru(
					int	DeviceId=0
					) = 0;

	//	4.2.33	has_dma_control
	virtual
	int		has_dma_control(
					int	DeviceId=0
					) = 0;

	//	4.2.34	has_scaled_rgb
	virtual
	int		has_scaled_rgb(
					int	DeviceId=0
					) = 0;

	//	4.3	Control Functions
	//---------------------------------
	//	4.3.1	set_timing_adjust
	//			Determines the video format of a signal applied to the Link A input. 
	virtual
	BErr	set_timing_adjust(
					unsigned int	HPhase,
					unsigned int	VPhase
					) = 0;

	//	4.3.2	set_vertical_flip
	virtual
	BErr	set_vertical_flip(
					int&	On
					) = 0;

	//	4.3.3	set_output_key
	virtual
	BErr	set_output_key(
					int&	On,
					int&	v4444,
					int&	Invert,
					int&	White
					) = 0;

	//	4.3.4	set_output_key_on
	virtual
	BErr	set_output_key_on(
					int&	On
					) = 0;

	//	4.3.5	set_output_key_v4444
	virtual
	BErr	set_output_key_v4444(
					int&	v4444
					) = 0;

	//	4.3.6	set_output_key_invert
	virtual
	BErr	set_output_key_invert(
					int&	Invert
					) = 0;

	//	4.3.7	set_output_key_white
	virtual
	BErr	set_output_key_white(
					int&	White
					) = 0;

	//	4.3.8	set_letterbox
	virtual
	BErr	set_letterbox(
					unsigned int&	Lines,
					int&			Black
					) = 0;

	//	4.3.9	set_letterbox_lines
	virtual
	BErr	set_letterbox_lines(
					unsigned int&	Lines
					) = 0;

	//	4.3.10	set_letterbox_black
	virtual
	BErr	set_letterbox_black(
					int&			Black
					) = 0;

	//	4.3.11	set_safearea
	virtual
	BErr	set_safearea(
					int&			Title,
					int&			Picture
					) = 0;

	//	4.3.12	set_safearea_title
	virtual
	BErr	set_safearea_title(
					int&			Title
					) = 0;

	//	4.3.13	set_safearea_picture
	virtual
	BErr	set_safearea_picture(
					int&			Picture
					) = 0;

	//	4.3.14	set_output_video
	virtual
	BErr	set_output_video(
					int&			Enable
					) = 0;

	//	4.3.15	set_audio_rate
	virtual
	BErr	set_audio_rate(
					unsigned long&	Rate
					) = 0;

	//	4.3.16	set_audio_playthrough
	virtual
	BErr	set_audio_playthrough(
					int&			Playthru
					) = 0;

	//	4.3.17	wait_output_video_synch
	virtual
	BErr	wait_output_video_synch(
					unsigned long	UpdFmt,
					unsigned long&	FieldCount
					) = 0;

	//	4.3.18	get_output_video_synch_count
	virtual
	BErr	get_output_video_synch_count(
					unsigned long&	FieldCount
					) = 0;

	//	4.3.19	set_scaled_rgb
	virtual
	BErr	set_scaled_rgb(
					unsigned long&	On
					) = 0;

	//	4.3.20	wait_pci_interrupt
	virtual
	BErr	wait_pci_interrupt(
					unsigned long	Wait
					) = 0;

	//	4.3.21	get_audio_rate
	virtual
	BErr	get_audio_rate(
					unsigned long&	Rate
					) = 0;

	//	4.3.22	wait_input_video_synch
	virtual
	BErr	wait_input_video_synch(
					unsigned long	UpdFmt,
					unsigned long&	FieldCount
					) = 0;

	//	4.3.23	get_input_video_synch_count
	virtual
	BErr	get_input_video_synch_count(
					unsigned long&	FieldCount
					) = 0;


	//	4.4		Video STYLE Functions
	//---------------------------------
	//	4.4.1	get_video_input_format
	//			Determines the video format of a signal applied to the Link A input. 
	virtual
	BErr	get_video_input_format(
					unsigned long&	VidFmt
					) = 0;

	//	4.4.2	get_video_genlock_format
	//			Determines the video format of a signal applied to the GENLOCK input. 
	virtual
	BErr	get_video_genlock_format(
					unsigned long&	VidFmt
					) = 0;

	//	4.4.3	get_video_output_format
	//			Determines the video format of the output signal. 
	virtual
	BErr	get_video_output_format(
					unsigned long&	VidFmt
					) = 0;

	//	4.4.4	set_video_output_format
	//			Changes the output signal video format of Link A output. 
	virtual
	BErr	set_video_output_format(
					unsigned long&	VidFmt
					) = 0;

	//	4.4.5	get_video_memory_format
	//			Determines the pixel format for blue card video buffers. 
	virtual
	BErr	get_video_memory_format(
					unsigned long&	MemFmt
					) = 0;

	//	4.4.6	set_video_memory_format
	//			Changes the pixel format for blue card video buffers. 
	virtual
	BErr	set_video_memory_format(
					unsigned long&	MemFmt
					) = 0;

	//	4.4.7	get_video_update_format
	//			Determines the update synchronisation style of the video buffers. 
	virtual
	BErr	get_video_update_format(
					unsigned long&	UpdFmt
					) = 0;

	//	4.4.8	set_video_update_format
	//			Changes the video synchronisation method. 
	virtual
	BErr	set_video_update_format(
					unsigned long&	UpdFmt
					) = 0;
	//	4.4.9	get_video_zoom_format
	//			Determines the video resolution style of the video buffers. 
	virtual
	BErr	get_video_zoom_format(
					unsigned long&	ResFmt
					) = 0;

	//	4.4.10	set_video_zoom_format
	//			Changes the video resolution style of the video buffers. 
	virtual
	BErr	set_video_zoom_format(
					unsigned long&	ResFmt
					) = 0;
	//	4.4.11	get_video_framestore_style
	//			Determines the video mode, memory format and update synchronisation
	//			styles of the blue card video buffers. 
	virtual
	BErr	get_video_framestore_style(
					unsigned long&	VidFmt,
					unsigned long&	MemFmt,
					unsigned long&	UpdFmt,
					unsigned long&	ResFmt
					) = 0;

	//	4.4.12	set_video_framestore_style
	//			Changes the video mode, memory format and update synchronisation styles. 
	virtual
	BErr	set_video_framestore_style(
					unsigned long&	VidFmt,
					unsigned long&	MemFmt,
					unsigned long&	UpdFmt,
					unsigned long&	ResFmt
					) = 0;

	//	4.4.13	get_video_engine
	//				Instruct the device driver to change the operational mode of the
	//				video engine. 
	virtual
	BErr	get_video_engine(
					unsigned long&	Mode
					) = 0;

	//	4.4.14	set_video_engine
	//				Instruct the device driver to change the operational mode of the
	//				video engine. 
	virtual
	BErr	set_video_engine(
					unsigned long&	Mode
					) = 0;

	
	//	4.5		DMA Memory Functions
	//---------------------------------
	//	4.5.1	system_buffer_map
	//			Obtains the virtual address of one of the driver managed system buffers. 
	virtual
	BErr	system_buffer_map(
					void**	ppAddress,
					int		BufferId
					) = 0;

	//	4.5.2	system_buffer_unmap
	//			Unmaps the virtual address of one of the driver managed system buffers
	//			from the process address space. 
	virtual
	BErr	system_buffer_unmap(
					void*		pAddress
					) = 0;

	//	4.5.3	system_buffer_assign
	//			Assign an arbitrary usermode buffer to a particular DMA function. 
	virtual
	BErr	system_buffer_assign(
					void*			pAddress,
					unsigned long	Id,
					unsigned long	Length,
					unsigned long	Target
					) = 0;

	//	4.5.4	system_buffer_write
	//			Instructs the DMA engine to begin a DMA write operation to the
	//			active blue card host buffer. 
	// DEPRECATED; DO NOT USE! USE system_buffer_write_async() instead
	virtual
	int		system_buffer_write(
					unsigned char*		pPixels,
					unsigned long		Size,
					unsigned long		Offset=0
					) = 0;

	//	4.5.5	system_buffer_read
	//			Instructs the DMA engine to begin a DMA read operation from the
	//			active blue card host buffer. 
	// DEPRECATED; DO NOT USE! USE system_buffer_read_async() instead
	virtual
	int		system_buffer_read(
					unsigned char*		pPixels,
					unsigned long		Size,
					unsigned long		Offset=0
					) = 0;

	virtual
	int		system_buffer_write_async(
					unsigned char*		pPixels,
					unsigned long		Size,
					OVERLAPPED			* pAsync,
					unsigned long		BufferID,
					unsigned long		Offset=0
					) = 0;

	//	4.5.5	system_buffer_read
	//			Instructs the DMA engine to begin a DMA read operation from the
	//			active blue card host buffer. 
	virtual
	int		system_buffer_read_async(
					unsigned char*		pPixels,
					unsigned long		Size,
					OVERLAPPED			* pAsync,
					unsigned long		BufferID,
					unsigned long		Offset=0
					) = 0;


	//	4.6		Framestore Functions
	//---------------------------------
	//	4.6.1	render_buffer_count
	//			Determines the number of buffers the blue card memory has been partitioned into. 
	virtual
	BErr	render_buffer_count(
					unsigned long&		Count
					) = 0;

	//	4.6.2	render_buffer_update
	//			Instructs the video digitiser to select a blue card buffer to rasterise. 
	virtual
	BErr	render_buffer_update(
					unsigned long		BufferId
					) = 0;

	//	4.6.3	render_buffer_update_b
	//			Instructs the video digitiser to select a blue card buffer as the video
	//			channel B source for real-time dissolves. 
	virtual
	BErr	render_buffer_update_b(
					unsigned long		BufferId
					) = 0;

	//	4.6.4	render_buffer_dissolve
	//			Set the percentage of Channel A over Channel B for real-time dissolve. 
	virtual
	BErr	render_buffer_dissolve(
					unsigned long		Dissolve
					) = 0;

	//	4.6.5	render_buffer_dissolve_a_b
	//			Set the video source for Channel A and Channel B and the dissolve
	//			percentage between them. 
	virtual
	BErr	render_buffer_dissolve_a_b(
					unsigned long		BufferId_A,
					unsigned long		BufferId_B,
					unsigned long		Dissolve
					) = 0;

	//	4.6.6	render_buffer_map
	//			Get the virtual address of the indexed blue card buffer. 
	virtual
	BErr	render_buffer_map(
					void**			pAddress,
					unsigned long	BufferId
					) = 0;

	//	4.6.7	render_buffer_map_aperture
	//			Get the virtual address of the 8-bit aperture for the indexed blue card buffer. 
	virtual
	BErr	render_buffer_map_aperture(
					void**			pAddress,
					unsigned long	BufferId
					) = 0;

	//	4.6.8	render_buffer_map_all
	//			Generates a table of the virtual addresses for all blue card buffers. 
	virtual
	BErr	render_buffer_map_all(
					void**			pTable,
					unsigned long&	Count
					) = 0;


	//	4.6.9	render_buffer_map_aperture_all
	//			Generates a table of the virtual addresses for the 8-bit aperture
	//			of all blue card buffers. 
	virtual
	BErr	render_buffer_map_aperture_all(
					void**			pTable,
					unsigned long&	Count
					) = 0;

	//	4.6.10	render_buffer_select
	//			Specify which blue card buffer will become the target of future DMA transactions. 
	virtual
	BErr	render_buffer_select(
					unsigned long	BufferId
					) = 0;

	//	4.6.11	render_buffer_capture
	//			Specify which blue card buffer will be used for capture. 
	virtual
	BErr	render_buffer_capture(
					unsigned long	BufferId,
					int				On
					) = 0;

	//	4.6.12	render_buffer_sizeof
	//			Determine the maximum byte size of each blue card memory partition. 
	virtual
	BErr	render_buffer_sizeof(
					unsigned long&	Count,
					unsigned long&	Length,
					unsigned long&	Actual,
					unsigned long&	Golden
					) = 0;

	//	4.6.13	render_buffer_quantise
	//			Control whether blue card memory is repartitioned on style changes. 
	virtual
	BErr	render_buffer_quantise(
					int		On
					) = 0;

	//	4.7	Audio Functions
	//---------------------------------
	//	4.7.1	audio_playback_start
	//			Start audio playback. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_playback_start(
					unsigned long	Synch
					) = 0;

	//	4.7.2	audio_playback_stop
	//			Stop audio playback. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_playback_stop(
					void
					) = 0;

	//	4.7.3	audio_playback_stream
	//			Register a native interleaved audio file for playback. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_playback_stream(
					char*		pName,
					int			Offset,
					int			Flags
					) = 0;

	//	4.7.4	audio_playback_stream_mono
	//			Register a native monophonic audio file for playback. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_playback_stream_mono(
					unsigned long	Chan,
					char*			pName,
					int				Offset,
					int				Flags
					) = 0;

	//	4.7.5	audio_playback_stream_stereo
	//			Register a native stereophonic audio file for playback. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_playback_stream_stereo(
					unsigned long	Pair,
					char*			pName,
					int				Offset,
					int				Flags
					) = 0;

	//	4.7.6	audio_playback_buffer
	//			Register a native 6-channel interleaved audio buffer for playback. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_playback_buffer(
					void*			pGlobal,
					unsigned long*	pBuffer,
					unsigned long	Length,
					unsigned long	Chunk,
					int	(*pFunc)(void* pGlobal, unsigned long* pBuffer, int Offset, int Length),
					int				Flags
					) = 0;

	//	4.7.7	audio_playback_buffer_mono
	//			Register a native monophonic audio buffer for playback. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_playback_buffer_mono(
					unsigned long	Chan,
					void*			pGlobal,
					unsigned long*	pBuffer,
					unsigned long	Length,
					unsigned long	Chunk,
					int	(*pFunc)(void* pGlobal, unsigned long* pBuffer, int Offset, int Length),
					int				Flags
					) = 0;

	//	4.7.8	audio_playback_buffer_stereo
	//			Register a native stereophonic audio buffer for playback. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_playback_buffer_stereo(
					unsigned long	Pair,
					void*			pGlobal,
					unsigned long*	pBuffer,
					unsigned long	Length,
					unsigned long	Chunk,
					int	(*pFunc)(void* pGlobal, unsigned long* pBuffer, int Offset, int Length),
					int				Flags
					) = 0;

	//	4.7.9	audio_playback_deregister
	//			De-registers a native 6-channel interleaved audio source from playback. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_playback_deregister(
					void
					) = 0;

	//	4.7.10	audio_playback_deregister_mono
	//			De-registers a native monophonic audio source from playback. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_playback_deregister_mono(
					unsigned long	Chan
					) = 0;

	//	4.7.11	audio_playback_deregister_stereo
	//			De-registers a native stereophonic audio source from playback. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_playback_deregister_stereo(
					unsigned long	Pair
					) = 0;

	//	4.7.12	AudioHandlerPlay
	//			Moves source audio data streams into the playback buffer. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	AudioHandlerPlay(
					unsigned long&	Snooze
					) = 0;

	//	4.7.13	audio_capture_start
	//			Begin capturing audio. 
	virtual
	BErr	audio_capture_start(
					unsigned long	Synch,
					unsigned long	PlayThru
					) = 0;

	//	4.7.14	audio_capture_stop
	//			Stop capturing audio. 
	virtual
	BErr	audio_capture_stop(
					void
					) = 0;
	//	4.7.15	audio_capture_stream
	//			Register a file for capture of native interleaved audio. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_capture_stream (
					char*			pName,
					int				Offset,
					int				Flags
					) = 0;

	//	4.7.16	audio_capture_stream_mono
	//			Register a file for capture of native monophonic audio. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_capture_stream_mono(
					unsigned long	Chan,
					char*			pName,
					int				Offset,
					int				Flags
					) = 0;

	//	4.7.17	audio_capture_stream_stereo
	//			Register a file for capture of native stereophonic audio. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_capture_stream_stereo(
					unsigned long	Pair,
					char*			pName,
					int				Offset,
					int				Flags
					) = 0;

	//	4.7.18	audio_capture_buffer
	//			Register a buffer for capture of native interleaved audio. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_capture_buffer(
					void*			pGlobal,
					unsigned long*	pBuffer,
					unsigned long	Length,
					unsigned long	Chunk,
					int	(*pFunc)(void* pGlobal, unsigned long* pBuffer, int Offset, int Length),
					int				Flags
					) = 0;

	//	4.7.19	audio_capture_buffer_mono
	//			Register a buffer for capture of native monophonic audio. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_capture_buffer_mono(
					unsigned long	Chan,
					void*			pGlobal,
					unsigned long*	pBuffer,
					unsigned long	Length,
					unsigned long	Chunk,
					int	(*pFunc)(void* pGlobal, unsigned long* pBuffer, int Offset, int Length),
					int				Flags
					) = 0;

	//	4.7.20	audio_capture_buffer_stereo
	//			Register a buffer for capture of native stereophonic audio. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_capture_buffer_stereo(
					unsigned long	Pair,
					void*			pGlobal,
					unsigned long*	pBuffer,
					unsigned long	Length,
					unsigned long	Chunk,
					int	(*pFunc)(void* pGlobal, unsigned long* pBuffer, int Offset, int Length),
					int				Flags
					) = 0;

	//	4.7.21	audio_capture_deregister
	//			De-registers a buffer from capture monitor thread. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_capture_deregister(
					void
					) = 0;

	//	4.7.22	audio_capture_deregister_mono
	//			De-registers a single monophonic audio buffer from capture monitor thread. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_capture_deregister_mono(
					unsigned long	Chan
					) = 0;

	//	4.7.23	audio_capture_deregister_stereo
	//			De-registers a stereophonic audio buffer from capture monitor thread. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_capture_deregister_stereo(
					unsigned long	Pair
					) = 0;


	//	4.7.24	audio_playback_threshold
	//			Adjust the Chunk and Snooze times for the Audio Playback Monitor Thread
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_playback_threshold(
					unsigned long	Chunk,
					unsigned long	Snooze
					) = 0;

	//	4.7.25	audio_capture_sample_count
	//			Number of samples captured. 
	// DEPRECATED; DO NOT USE!
	virtual
	ULONG	audio_capture_sample_count() = 0;

	//	4.7.26	audio_capture_sample_count_mono
	//			Number of samples captured. 
	// DEPRECATED; DO NOT USE!
	virtual
	ULONG	audio_capture_sample_count_mono(unsigned long Chan) = 0;

	//	4.7.27	audio_capture_sample_count_stereo
	//			Number of samples captured. 
	// DEPRECATED; DO NOT USE!
	virtual
	ULONG	audio_capture_sample_count_stereo(unsigned long Pair) = 0;

	//	4.7.28	audio_playback_blip
	//			Channel is to be blipped
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	audio_playback_blip(
						int Channel
						) = 0;

	//	4.8	Video Engine Functions
	//---------------------------------
	//	4.8.1	video_playback_start
	//			Start video playback. 
	virtual
	BErr	video_playback_start(
					int		Step,
					int		Loop
					) = 0;

	//	4.8.2	video_playback_stop
	//			Halts the video playback engine. 
	virtual
	BErr	video_playback_stop(
					int		Wait,
					int		Flush
					) = 0;

	//	4.8.3	video_playback_flush
	//			Flush all pending display requests from all Channels. 
	virtual
	BErr	video_playback_flush(
					void
					) = 0;

	//	4.8.4	video_playback_flush_A
	//			Flush all pending display requests from Channel A. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	video_playback_flush_A(
					void
					) = 0;

	//	4.8.5	video_playback_flush_B
	//			Flush all pending display requests from Channel-B. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	video_playback_flush_B(
					void
					) = 0;

	//	4.8.6	video_playback_allocate
	//			Obtain the address of the next available video memory buffer. 
	virtual
	BErr	video_playback_allocate(
					void**			pAddress,
					unsigned long&	BufferId,
					unsigned long&	Underrun
					) = 0;

	//	4.8.7	video_playback_release
	//			Release physical blue card video buffer. 
	virtual
	BErr	video_playback_release(
					unsigned long	BufferId
					) = 0;

	//	4.8.8	video_playback_flush_display
	//			Remove a unique display request from the display lists. 
	virtual
	BErr	video_playback_flush_display(
					unsigned long	UniqueId
					) = 0;

	//	4.8.9	video_playback_release_flush
	//			Purges all pending display requests and returns the frame to the free list.
	virtual
	BErr	video_playback_release_flush(
					unsigned long	BufferId
					) = 0;

	//	4.8.10	video_playback_present
	//			Present a buffer to the video playback engine Channel-A. 
	virtual
	BErr	video_playback_present(
					unsigned long&	UniqueId,
					unsigned long	BufferId,
					unsigned long	Count,
					int				Keep,
					int				Odd=0
					) = 0;

	//	4.8.11	video_playback_present_dissolve
	//			Present a frame with a dissolve value to the video playback engine. 
	virtual
	BErr	video_playback_present_dissolve(
					unsigned long&	UniqueId,
					unsigned long	BufferId,
					unsigned long	Count,
					unsigned long	Dissolve,
					int				Keep,
					int				Odd=0
					) = 0;

	//	4.8.12	video_playback_present_A
	//			Present a frame to the video playback engine that will be inserted into Channel-A. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	video_playback_present_A(
					unsigned long&	UniqueId,
					unsigned long	BufferId,
					unsigned long	Count,
					unsigned long	Dissolve,
					int				Synch_B,
					int				Keep,
					int				Odd=0
					) = 0;

	//	4.8.13	video_playabck_present_B
	//			Present a frame to the video playback engine that will be inserted into Channel-B. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	video_playback_present_B(
					unsigned long&	UniqueId,
					unsigned long	BufferId,
					unsigned long	Count,
					unsigned long	Dissolve,
					int				Synch_A,
					int				Keep,
					int				Odd=0
					) = 0;

	//	4.8.14	video_playback_present_detail
	//			The general purpose presentation function. 
	// DEPRECATED; DO NOT USE!
	virtual
	BErr	video_playback_present_detail(
					unsigned long&	UniqueId,
					unsigned long	BufferId,
					unsigned long	Count,
					unsigned long	Dissolve,
					unsigned long	Flags
					) = 0;

	//	4.8.15	video_capture_start
	//			Instruct the device driver to begin capturing images into the video framestore. 
	virtual
	BErr	video_capture_start(
					int		Step=0
					) = 0;

	//	4.8.16	video_capture_stop
	//			Instruct the device driver to stop the video capture. 
	virtual
	BErr	video_capture_stop(
					void
					) = 0;

	//	4.8.17	video_capture_harvest
	//			Get the details about the next frame in a capture sequence. 
	virtual
	BErr	video_capture_harvest(
					void**			ppAddress,
					unsigned long&	BufferId,
					unsigned long&	Count,
					unsigned long&	Frames,
					int				CompostLater=0
					) = 0;

	// not used for anything important...
	// DEPRECATED; DO NOT USE!
	virtual
	BErr nudge_frame(LONG nudge) = 0;

	//	4.8.18	video_playback_pause
	//			Suspend or Resume playback 
	virtual
	BErr	video_playback_pause(
					int		Suspend
					) = 0;

	//	4.8.19	video_capture_compost
	//			Return a harvested frame for recycling
	virtual
	BErr	video_capture_compost(
					unsigned long	BufferId
					) = 0;

#ifdef BLUEVELVET_SDK_VERSION3
	virtual BErr set_onboard_keyer(int & On)=0;
	virtual BErr get_onboard_keyer_status(int &On)=0;
	virtual	BErr get_timing_adjust(unsigned int	& HPhase,unsigned int & VPhase,unsigned int & MaxHPhase,unsigned int & MaxVPhase) = 0;
	virtual BErr get_letterbox_values(unsigned int&	Lines,int & bBlackEnableFlag)=0;
	virtual BErr get_safearea_info(int&	Title,int& Picture)=0;
	virtual int has_downconverter_bnc(int deviceId)=0;
	virtual int has_onboard_keyer(int deviceId)=0;
	virtual int has_duallink_input(int deviceId)=0;
	virtual int has_programmable_colorspace_matrix(int deviceId)=0;

	virtual BErr SetMatrix_Col_Green_Y(double CoeffG_R,double CoeffG_G,double CoeffG_B,double constG)=0;
	virtual BErr GetMatrix_Col_Green_Y(double & CoeffG_R,double & CoeffG_G,double & CoeffG_B,double & constG)=0;

	virtual BErr SetMatrix_Col_Red_PR(double CoeffR_R,double CoeffR_G,double CoeffR_B,double constR)=0;
	virtual BErr GetMatrix_Col_Red_PR(double & CoeffR_R,double & CoeffR_G,double & CoeffR_B,double & constR)=0;

	virtual BErr SetMatrix_Col_Blue_PB(double CoeffB_R,double CoeffB_G,double CoeffB_B,double constB)=0;
	virtual BErr GetMatrix_Col_Blue_PB(double & CoeffB_R,double & CoeffB_G,double & CoeffB_B,double & constB)=0;

	virtual BErr SetDualLink_Output_Conn_SignalColorSpace(unsigned long & signalType,unsigned long  updateMatrixFlag)=0;
	virtual BErr SetDualLink_Input(unsigned long  & EnableDualLink)=0;
	virtual BErr SetDualLink_Input_SignalFormatType(unsigned long &v4444)=0;
	virtual BErr GetDualLink_InputProperty(unsigned long & DualLink,unsigned long & connSignalColorSpace,unsigned long & v4444)=0;
	virtual BErr GetDualLink_OutputProperty(unsigned long & DualLink,unsigned long & connSignalColorSpace,unsigned long & v4444)=0;

	virtual BErr Set_DownConverterSignalType(unsigned long type)=0;
	virtual BErr GetDownConverterSignalType(unsigned long & connSignalType)=0;

	virtual BErr SetDualLink_Input_Conn_SignalColorSpace(unsigned long  & signalType)=0;
	virtual int  GetHDCardType(int nDeviceId)=0;

	// New Audio Interface 
	virtual BErr MaxAudioOutBufferSize(long & nSampleCount)=0;
	virtual BErr GetAudioOutBufferFreeSpace(long  & nSampleCount)=0;
	virtual BErr wait_audio_output_interrupt(unsigned long & noQueuedSample,unsigned long & noFreeSample) = 0;
	virtual BErr InitAudioPlaybackMode()=0;
	virtual BErr StartAudioPlayback(int syncCount)=0;
	virtual BErr StopAudioPlayback()=0;
	virtual BErr WriteAudioSample(int nSampleType,void * pBuffer,long  nNoSample,int bFlag,long nNoSamplesWritten)=0;
	virtual BErr EndAudioPlaybackMode()=0; 
	virtual int GetPCIRevId()=0;
#endif

	//	Need this so that derived destructor gets called
	virtual ~CBlueVelvet(){}
	HANDLE		m_hDevice;					// Handle to the blue card device driver
};


//------------------------------------------------------------------------------------------------------------
extern "C" {
//------------------------------------------------------------------------------------------------------------
//	4.0.0	The Blue Velvet factory
BLUEFISH_API CBlueVelvet*	BlueVelvetFactory();


//	4.0.1	Who am I
BLUEFISH_API const char*	BlueVelvetVersion();

//	4.0.2	Golden Value calculation
BLUEFISH_API unsigned long BlueVelvetGolden(
										unsigned long	VidFmt,
										unsigned long	MemFmt,
										unsigned long	UpdFmt
										);
//	4.0.3	Bytes Per Line calculation
BLUEFISH_API unsigned long BlueVelvetLineBytes(
										unsigned long	VidFmt,
										unsigned long	MemFmt
										);
//	4.0.4	Bytes Per Frame calculation
BLUEFISH_API unsigned long BlueVelvetFrameBytes(
										unsigned long	VidFmt,
										unsigned long	MemFmt,
										unsigned long	UpdFmt
										);

//	4.0.5	Lines Per Frame calculation
BLUEFISH_API unsigned long BlueVelvetFrameLines(
										unsigned long	VidFmt,
										unsigned long	UpdFmt
										);

//	4.0.6	Pixels per Line calculation
BLUEFISH_API unsigned long BlueVelvetLinePixels(
										unsigned long	VidFmt
										);

BLUEFISH_API unsigned long BlueVelvetVBILines(unsigned long VidFmt,unsigned long FrameType);

}

#endif	//_BLUEVELVET_H
