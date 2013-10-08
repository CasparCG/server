#pragma once

// Are files exported ?
#ifdef	COMPILE_PROCESSING_AIRSEND
#define	PROCESSING_AIRSEND_API	__declspec(dllexport)
#else	// COMPILE_PROCESSING_AIRSEND
#define	PROCESSING_AIRSEND_API	__declspec(dllimport)
#endif	// COMPILE_PROCESSING_AIRSEND

// Create and initialize an AirSend instance. This will return NULL if it fails.
extern "C" PROCESSING_AIRSEND_API	
			void*	AirSend_Create( // The video resolution. This should be a multiple of 8 pixels wide.
								    // This is the full frame resolution and not the per field resolution
									// so for instance a 1920x1080 interlaced video stream would store
									// xres=1920 yres=1080
									const int xres, const int yres, 
									// The frame-rate as a numerator and denominator. Examples :
									// NTSC, 480i30, 1080i30 : 30000/1001
									// NTSC, 720p60 : 60000/1001
									// PAL, 576i50, 1080i50 : 30000/1200
									// PAL, 720p50 : 60000/1200
									const int frame_rate_n, const int frame_rate_d, 
									// Is this field interlaced or not ?
									const bool progressive,
									// The image aspect ratio as a floating point. For instance
									// 4:3  = 4.0/3.0  = 1.33333
									// 16:9 = 16.0/9.0 = 1.77778
									const float aspect_ratio,
									// Do we want audio ?
									const bool audio_enabled,
									// The number of audio channels. 
									const int no_channels,
									// The audio sample-rate
									const int sample_rate );

// Destroy an instance of AirSend that was created by AirSend_Create.
extern "C" PROCESSING_AIRSEND_API
			void AirSend_Destroy( void* p_instance );

// Add a video frame. This is in YCbCr format and may have an optional alpha channel.
// This is stored in an uncompressed video buffer of FourCC UYVY which has 16 bits per pixel
// YUV 4:2:2 (Y sample at every pixel, U and V sampled at every second pixel horizontally on each line). 
// This means that the stride of the image is xres*2 bytes pointed to by p_ycbcr. 
// For fielded video, the two fields are interleaved together and it is assumed that field 0 is always
// above field 1 which matches all modern video formats. It is recommended that if you desire to send
// 486 line video that you drop the first line and the bottom 5 lines and send 480 line video. 
// The return value is true when connected, false when not connected.
extern "C" PROCESSING_AIRSEND_API
			bool AirSend_add_frame_ycbcr( void* p_instance, const BYTE* p_ycbcr );

extern "C" PROCESSING_AIRSEND_API
			bool AirSend_add_frame_ycbcr_alpha( void* p_instance, const BYTE* p_ycbcr, const BYTE* p_alpha );

// These methods allow you to add video in BGRA and BGRX formats. (BGRX is 32 bit BGR with the alpha channel
// ignored.) Frames are provided as uncompressed buffers. YCbCr is the preferred color space and these are
// provided as a conveniance.
// The return value is true when connected, false when not connected.
extern "C" PROCESSING_AIRSEND_API
			bool AirSend_add_frame_bgra( void* p_instance, const BYTE* p_bgra );
extern "C" PROCESSING_AIRSEND_API
			bool AirSend_add_frame_bgrx( void* p_instance, const BYTE* p_bgrx );

// Because Windows tends to create images bottom to top by default in memory, there are versions of the
// BGR? functions that will send the video frame vertically flipped to avoid you needing to use CPU time
// and memory bandwidth doing this yourself.
// The return value is true when connected, false when not connected.
extern "C" PROCESSING_AIRSEND_API
			bool AirSend_add_frame_bgra_flipped( void* p_instance, const BYTE* p_bgra );
extern "C" PROCESSING_AIRSEND_API
			bool AirSend_add_frame_bgrx_flipped( void* p_instance, const BYTE* p_bgrx );

// Add audio data. This should be in 16 bit PCM uncompressed and all channels are interleaved together.
// Because audio and video are muxed together and send to the video source it is important that you send
// these at the same rate since video frames will be "held" in the muxer until the corresponding audio
// is received so that all data can be sent in "display" order to the TriCaster.
// The return value is true when connected, false when not connected.
extern "C" PROCESSING_AIRSEND_API
			bool AirSend_add_audio( void* p_instance, const short* p_src, const int no_samples );

// This allows you to tell a particular TriCaster that is on "Receive" mode to watch this video source.
// By default, on a TriCaster "Net 1" is on port 7000, and "Net 2" is on port 7001. Note that a full implementation
// should use Bonjour to locate the TriCaster as described in the SDK documentation; when working this way
// you would always know the true port numbers.
extern "C" PROCESSING_AIRSEND_API
			void AirSend_request_connection( void* p_instance, const ULONG IP, const USHORT Port );
