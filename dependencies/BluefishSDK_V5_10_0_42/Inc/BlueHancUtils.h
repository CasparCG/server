#pragma once
#ifndef BLUE_LINUX_CODE
#ifndef HANCUTILS_USE_STATIC_LIB
	#ifdef HANCUTILS_EXPORTS
		#define HANCUTILS_API __declspec(dllexport)
	#elif defined(__APPLE__)
		#define HANCUTILS_API
		#define ATLTRACE	printf
	#else
		#define HANCUTILS_API __declspec(dllimport)
	#endif
#else
	#define HANCUTILS_API
#endif
#else
	#define HANCUTILS_API
typedef bool BOOL;	
#endif	
#include "BlueDriver_p.h"




extern "C"
{
/**
@defgroup hanc_manipilation_function Embedded audio
@{
*/

#pragma pack(push, hanc_struct, 1)

/**
@brief The structure is used to extract/insert  Embedded audio to and from the HANC stream of Greed and Leon based cards.*/

struct hanc_stream_info_struct
{
	BLUE_INT32 AudioDBNArray[4];			/**< Contains the DBN values that should be used for each of the embedded audio groups*/
	BLUE_INT32 AudioChannelStatusBlock[4];	/**< channel status block information for each of the embedded audio group*/
	BLUE_UINT32 flag_valid_time_code;		/**< flag which identifies the validity of the time code member in the #hanc_stream_info_struct*/
	BLUE_UINT64	time_code;					/**< RP188 time code that was extracted from the HANC buffer or RP188 timecode which should be inserted 
												 into the HANC buffer*/
	BLUE_UINT32* hanc_data_ptr;				/**< Hanc Buffer which should be used as the source or destination for either extraction or insertion */
	BLUE_UINT32 video_mode;					/**< video mode which this hanc buffer which be used with. We need this information for do the required audio distribution 
												 especially NTSC */
	BLUE_UINT64 ltc_time_code;
	BLUE_UINT64 sd_vitc_time_code;
	BLUE_UINT64 rp188_ltc_time_code;
	BLUE_UINT32 pad[126];
};

#define AUDIO_INPUT_SOURCE_EMB	0
#define AUDIO_INPUT_SOURCE_AES	1
struct hanc_decode_struct
{
	void* audio_pcm_data_ptr;			// Buffer which would be used to store the extracted PCM
										// audio data. Must be filled in by app before calling function.
	BLUE_UINT32 audio_ch_required_mask;	// which all audio channels should be extracted from the 
										// audio frame .Must be filled in by app before calling function.
	BLUE_UINT32 type_of_sample_required;// type of destination  audio channel
										//ie 16 bit ,24 bit or 32 bit PCM data .
										//Must be filled in by app before calling function.
	BLUE_UINT32 no_audio_samples;		// this would contain how many audio samples has been decoded from
										// the hanc buffer.
	BLUE_UINT64 timecodes[7];			// Would extract the timecode information from the audio frame.
	void * raw_custom_anc_pkt_data_ptr;			// This buffer  would contain the raw ANC packets that was found in the orac hanc buffer.
												// this would contain any ANC packets that is not of type embedded audio and RP188 TC.
												//Must be filled in by app before calling function. can be NULL
	BLUE_UINT32 sizeof_custom_anc_pkt_data_ptr; // size of the ANC buffer array
												//Must be filled in by app before calling function. can be NULL
	BLUE_UINT32 avail_custom_anc_pkt_data_bytes;// how many custom ANC packets has been decoded into raw_hanc_pkt_data_ptr
												//Must be filled in by app before calling function. can be NULL
	BLUE_UINT32 audio_input_source;		// Used to select the audio input source. 
										// whether it is AES or Embedded.
										//Must be filled in by app before calling function.
	BLUE_UINT32 audio_temp_buffer[16];	// this is used to store split audio sample 
										// which did not contain all its audio channels
										// in one audio frame
										//Must be initialised to zero by app before first instantiating the  function. 
	BLUE_UINT32 audio_split_buffer_mask; // The mask would be used to make a note of 
										// split audio sample information for a frame.
										//Must be initialised to zero by app before first instantiating the  function. 
	BLUE_UINT32 max_expected_audio_sample_count; // specify the maximum number of audio samples 
												 // that the audio pcm buffer can contain.
												//Must be filled in by app before calling function.
	BLUE_UINT32 pad[124];
};

#pragma pack(pop, hanc_struct)

HANCUTILS_API BLUE_UINT32 encode_hanc_frame(struct hanc_stream_info_struct* hanc_stream_ptr,
											void* audio_pcm_ptr,
											BLUE_UINT32 no_audio_ch,
											BLUE_UINT32 no_audio_samples,
											BLUE_UINT32 nTypeOfSample,
											BLUE_UINT32 emb_audio_flag);

HANCUTILS_API BLUE_UINT32 encode_hanc_frame_ex( BLUE_UINT32 card_type,
												struct hanc_stream_info_struct* hanc_stream_ptr,
												void* audio_pcm_ptr,
												BLUE_UINT32 no_audio_ch,
												BLUE_UINT32 no_audio_samples,
												BLUE_UINT32 nTypeOfSample,
												BLUE_UINT32 emb_audio_flag);


HANCUTILS_API BLUE_UINT32 encode_hanc_frame_with_ucz(	BLUE_UINT32 card_type,
														struct hanc_stream_info_struct* hanc_stream_ptr,
														void* audio_pcm_ptr,
														BLUE_UINT32 no_audio_ch,
														BLUE_UINT32 no_audio_samples,
														BLUE_UINT32 nTypeOfSample,
														BLUE_UINT32 emb_audio_flag,
														BLUE_UINT8* pUCZBuffer);

HANCUTILS_API BLUE_UINT32 create_embed_audiosample(	void* raw_data_ptr,
													BLUE_UINT32* emb_data_ptr,
													BLUE_UINT32 channels_per_audio_sample,
													BLUE_UINT32 bytes_per_ch,
													BLUE_UINT32 no_samples,
													BLUE_UINT32 emb_audio_flags,
													BLUE_UINT8* Audio_Groups_DBN_Array,
													BLUE_UINT8* Audio_Groups_statusblock_Array);

HANCUTILS_API BLUE_UINT32* get_embed_audio_distribution_array(BLUE_UINT32 video_mode, BLUE_UINT32 sequence_no);
//HANCUTILS_API BLUE_UINT32 * GetAudioFrameSequence(BLUE_UINT32 video_output_standard);

HANCUTILS_API bool hanc_stream_analyzer(BLUE_UINT32 *src_hanc_buffer,struct hanc_stream_info_struct * hanc_stream_ptr);
HANCUTILS_API bool orac_hanc_stream_analyzer(BLUE_UINT32 card_type,BLUE_UINT32 *src_hanc_buffer,struct hanc_decode_struct * decode_ptr,char * analyzer_output_file);
HANCUTILS_API bool hanc_decoder_ex(	BLUE_UINT32 card_type,
									BLUE_UINT32* src_hanc_buffer,
									struct hanc_decode_struct* hanc_decode_struct_ptr);

/**
@}
*/

/**
@defgroup vanc_manipilation_function vanc packet I/O 
@{
*/


/**
@brief enumerator used by VANC manipulation function on HD cards to notify whether 
		VANC pakcet shoule be inserted/extracted from VANC Y buffers or VANC CbCr buffer.
		This enumerator will only be used on  HD video modes as it is the only with 
		2 type of ANC bufers ir Y and CbCr. On SD Modes the ANC data is inserted across 
		both Y anc CbCr values.
		
*/
enum blue_vanc_pkt_type_enum
{
	blue_vanc_pkt_y_comp=0, /**< ANC pkt should be inserted/extracted from the  Y component buffer*/
	blue_vanc_pkt_cbcr_comp=1 /**< ANC pkt should be inserted/extracted from the  CbCr component buffer*/
};

/*!
@brief Use this function to initialise VANC buffer before inserting any packets into the buffer
@param CardType type of bluefish  card to which this vanc buffer was transferred to.
@param nVideoMode video mode under which this vanc buffer will be used.
@param pixels_per_line width in pixels of the vanc buffer that has to be initialised.
@param lines_per_frame height of the vanc buffer that has to be initialised.
@param pVancBuffer vanc buffer which has to be initialised.
@remarks.

*/
HANCUTILS_API BLUE_UINT32 blue_init_vanc_buffer(BLUE_UINT32 CardType,BLUE_UINT32 nVideoMode,BLUE_UINT32 pixels_per_line,BLUE_UINT32 lines_per_frame,BLUE_UINT32 * pVancBuffer);
/*!
@brief this function can be used to extract ANC packet from HD cards. Currently we can only extract packets in the VANC space.
@param CardType type of the card from which the vanc buffer was captured.
@param vanc_pkt_type This parameter denotes whether to search for the VANC packet in Y Space or Cb/Cr Space.
					 The values this parameter accepts are defined in the enumerator #blue_vanc_pkt_type_enum
@param src_vanc_buffer Vanc buffer which was captured from bluefish card
@param src_vanc_buffer_size size of the vanc buffer which should be parsed for the specified vanc packet
@param pixels_per_line specifies how many pixels are there in each line of VANC buffer
@param vanc_pkt_did specifies the DID of the Vanc packet which should be extracted from the buffer
@param vanc_pkt_sdid Returns the SDID of the extracted VANC packet
@param vanc_pkt_data_length returns the size of the extracted VANC packet. The size is specifed as number of UDW words
							that was  contained in the packet
@param vanc_pkt_data_ptr pointer to UDW of the VANC packets . The 10 bit UDW words are packed in a 16 bit integer. The bottom 10 bit of the 
						16 bit word contains the UDW data.
@param vanc_pkt_line_no line number  where the packet was found .

@remarks.

*/
HANCUTILS_API BLUE_INT32  vanc_pkt_extract( 
											BLUE_UINT32 CardType,
											BLUE_UINT32 vanc_pkt_type,
											BLUE_UINT32 * src_vanc_buffer,
											BLUE_UINT32 src_vanc_buffer_size,
											BLUE_UINT32 pixels_per_line,
											BLUE_UINT32		vanc_pkt_did,
											BLUE_UINT16 * vanc_pkt_sdid,
											BLUE_UINT16 * vanc_pkt_data_length,
											BLUE_UINT16 * vanc_pkt_data_ptr,
											BLUE_UINT16 * vanc_pkt_line_no);

/**
@brief use this function to insert ANC packets into the VANC space of the HD cards.
@param CardType type of the card from which the vanc buffer was captured.
@param vanc_pkt_type This parameter denotes whether to search for the VANC packet in Y Space or Cb/Cr Space.
					 The values this parameter accepts are defined in the enumerator #blue_vanc_pkt_type_enum
@param vanc_pkt_line_no line in th VANC buffer where the ANC packet should inserted.
@param vanc_pkt_buffer vanc ANC packet which should be inserted into the VANC buffer.
@param vanc_pkt_buffer_size size of the ANC packet including the checksum ,ADF , SDID, DID and Data Count
@param dest_vanc_buffer VANC buffer into which the ANC packet will be inserted into.
@param pixels_per_line specifies how many pixels are there in each line of VANC buffer
*/
HANCUTILS_API BLUE_INT32  vanc_pkt_insert(
											BLUE_UINT32 CardType,
											BLUE_UINT32 vanc_pkt_type,
											BLUE_UINT32 vanc_pkt_line_no,
											BLUE_UINT32 * vanc_pkt_buffer,
											BLUE_UINT32 vanc_pkt_buffer_size,
											BLUE_UINT32 * dest_vanc_buffer,
											BLUE_UINT32 pixels_per_line);

/** @} */

/**
@defgroup vanc_decode_encoder_helper ANC encoder/decoder 
	@{
*/
HANCUTILS_API BLUE_UINT32 decode_eia_708b_pkt(BLUE_UINT32 CardType,BLUE_UINT16 * vanc_pkt_data_ptr,BLUE_UINT16 pkt_udw_count,BLUE_UINT16 eia_pkt_subtype,BLUE_UINT8 * decoded_ch_str);
//#ifndef BLUE_LINUX_CODE
//HANCUTILS_API BLUE_UINT64 decode_rp188_packet(BLUE_UINT32 CardType,BLUE_UINT32 * src_vanc_buffer,BLUE_UINT32 UDW_Count,BLUE_UINT64 *rp188_dbb);
//HANCUTILS_API bool blue_vitc_decoder_8bit_fmt(BLUE_UINT8 * raw_vbi_ptr,BLUE_UINT32 pixels_per_line,BLUE_UINT32 mem_fmt,BLUE_UINT32 vitc_line_no,BLUE_UINT64 * vitc_time_code);
//HANCUTILS_API bool blue_vitc_decoder_10bit_v210(BLUE_UINT8 * raw_vbi_ptr, BLUE_UINT32 vitc_line_no, BLUE_UINT64 * vitc_time_code);
//HANCUTILS_API unsigned int create_rp188_pkt(
//							  BLUE_UINT32 cardType,
//							  BLUE_UINT32 * emb_data_ptr,
//							  BLUE_UINT32 line_no,
//							  BLUE_UINT32 start_new_line,
//							  BLUE_UINT64 timecode,
//							  BLUE_UINT64 rp188_dbb);
//#endif


/** @} */
}
