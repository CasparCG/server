#pragma once 
#include "BlueDriver_p.h"

#ifdef _WINDOWS
#pragma pack(push,1)
#endif

#define BLUE_HANC_INVALID_DID	(0x0)

#define BLUE_HANC_AUDIOGROUP1	(0x2FF)
#define BLUE_HANC_AUDIOGROUP2	(0x1FD)
#define BLUE_HANC_AUDIOGROUP3	(0x1FB)
#define BLUE_HANC_AUDIOGROUP4	(0x2F9)
#define BLUE_HANC_RP188			(0x260)
#define BLUE_HANC_AUDIOGROUP1_CONTROL	(0x1EF)
#define BLUE_HANC_AUDIOGROUP2_CONTROL	(0x2EE)
#define BLUE_HANC_AUDIOGROUP3_CONTROL	(0x2ED)
#define BLUE_HANC_AUDIOGROUP4_CONTROL	(0x1EC)
#define BLUE_HANC_AUDIOGROUP1_EXTENDED	(0x1FE)
#define BLUE_HANC_AUDIOGROUP2_EXTENDED	(0x2FC)
#define BLUE_HANC_AUDIOGROUP3_EXTENDED	(0x2FA)
#define BLUE_HANC_AUDIOGROUP4_EXTENDED	(0x1F8)


#define HANC_PACKET_HEADER_CONST	(0xBFFFFC00)

#define BLUE_HANC_START_NEWLINE(line_number) ((0xC0000000)| (line_number << 16))

#define BLUE_HANC_CONTROL_WORD				 (0xC0000000)
#define BLUE_HANC_3DATA_PACKET_WORD			 (0x80000000)
#define BLUE_HANC_2DATA_PACKET_WORD			 (0x40000000)
#define BLUE_HANC_1DATA_PACKET_WORD			 (0x00000000)
#define BLUE_HANC_ENDOF_FRAME()				 ((0xC0000000)| (1 << 15))


#define AESAUDIO_DATA_BLOCKSIZE				(192)
#define MAX_HANC_BUFFER_SIZE				(65536) //256*256
#define MAX_HANC_BUFFER_SIZE_BYTES			(256*1024)
/* 
HANC Packet header structure
Contains 2 type of structure , 
which makes it easier to parse the data
*/

struct GenericV210_structure
{
#if defined(__LITTLE_ENDIAN__) || defined(_WINDOWS) || defined(BLUE_LINUX_CODE)
	BLUE_UINT32 first_word:10,second_word:10,third_word:10,unused:2;
#else
	BLUE_UINT32 unused:2,third_word:10,second_word:10,first_word:10;
#endif
#ifndef _WINDOWS
}__attribute__((packed));
#else
};
#endif

union GenericV210_union
{
	struct GenericV210_structure v210_struct;
	BLUE_UINT32 v210_word;
};

/* HANC packet header*/
struct HancPacketHeaderStruct
{
#if defined(__LITTLE_ENDIAN__) || defined(_WINDOWS) || defined(BLUE_LINUX_CODE)
	union GenericV210_union	ancillary_data_flag; // 0x0,0x3FF,0x3FF, This is a constant defined by smpte
	union GenericV210_union	packet_info; 	//  first 10 bit word --> Data ID
						//	Commonly used  Data ID packet values are 
						//		1) 0x2FF --> Group1 Embedded Audio packet
						//		2) 0x1FD --> Group2 Embedded Audio Packet
						//		3) 0x1FB --> Group3 Embedded Audio Packet
						//		4) 0x2F9 --> Group4 Embedded Audio packet
						//  second 10 bit word --> Data Block Number
						//  	This is used for type 1 packets.
						//  third 10 bit word --> Data Count 
						//  	This 10 bit word specifies the amount of user data 
						//  	that this hanc will contain.
#else
	union GenericV210_union packet_info;
	union GenericV210_union ancillary_data_flag;
#endif
#ifndef _WINDOWS
}__attribute__((packed));
#else
};
#endif


/* Audio SubFrame Packet */
struct BlueAudioSubFrameStruct
{	
#if defined(__LITTLE_ENDIAN__) || defined(_WINDOWS) || defined(BLUE_LINUX_CODE)
	BLUE_UINT32	ZBit:1,					//bit 0		set to declare start of channel status word
			Channel:2,					//bit 1-2
			AudioData_0_5:6,			//bit 3-8
			NotBit8:1,					//bit 9
			AudioData_6_14:9,			//bit 10-18
			NotBit18:1,					//bit 19	use same value as NotBit8 (bit 9)
			AudioData_15_19:5,			//bit 20-24
			AESSampleValidityBit:1,		//bit 25
			AESUserBit:1,				//bit 26
			AESAudioChannelStatusBit:1,	//bit 27	one bit of the channel status word
			ParityBit:1,				//bit 28	xor of all bits except (NotBit8 (bit 9) and NotBit18 (bit 19))
			NotBit31:1,					//bit 29	not of ParityBit (bit 28)
			akiraControlBits:2;			//bit 30-31
#else
	BLUE_UINT32	akiraControlBits:2,
			NotBit31:1,
			ParityBit:1,
			AESAudioChannelStatusBit:1,
			AESUserBit:1,
			AESSampleValidityBit:1,
			AudioData_15_19:5,
			NotBit18:1,
			AudioData_6_14:9,
			NotBit8:1,
			AudioData_0_5:6,
			Channel:2,
			ZBit:1;
#endif

#ifndef _WINDOWS
}__attribute__((packed));
#else
};
#endif

union BlueAudioSubFrameHeader
{
	struct BlueAudioSubFrameStruct audioSubFrame;
	BLUE_UINT32  BlueAudioSubFrameWord;
	struct GenericV210_structure audioSubFrame_v210;
};

#define  MAX_AUDIO_SUBFRAMES_IN_A_LINE (64) // 4 samples per audio group  and 4 channesl for each  audio group per sample


/*
Time code structure that the function expects is the same format as LTC time code
bits 0 - 3 :units of frame 
bits 4 - 7: binary group1 
bits 8 - 9: tens of frame 
bits 10 -11: flags 
bits 12 -15: binary group2 
bits 16-19 : units of seconds 
bits 20-23 : binary group3 

bits 24 - 26: tens of seconds
bit 27 : flag 
bits 28 - 31: group binary4  
bits 32 -35: units of minutes 

bits 36 - 39 :binary5 
bits 40 - 42: tens of minutes 
bit 43 : flag 
bits 44 - 47: binary group6 

bits 48 - 51: units of hours 
bits 52 - 55: binary group7 
bits 56 - 57: tens of hours 
bits 58 - 59: flag
bits 60 - 63: binary8

*/
struct LTC_TimeCode
{
#if defined(__LITTLE_ENDIAN__) || defined(_WINDOWS) || defined(BLUE_LINUX_CODE)
	BLUE_UINT64     unit_frame:4,binary1:4,ten_frame:2,drop_frame_flag:1,color_frame_flag:1,
				 binary2:4,unit_second:4,binary3:4,ten_second:3,unsued_1:1,binary4:4,
				 unit_minute:4,binary5:4,ten_minute:3,unsued_2:1,binary6:4,unit_hours:4,
				 binary7:4,ten_hours:2,unsued_3:2,binary8:4;
#else
	BLUE_UINT64		binary8:4,unsued_3:2,ten_hours:2,binary7:4,
				unit_hours:4,binary6:4,unused_2:1,ten_minute:3,binary5:4,unit_minute:4,
				binary4:4,unused_1:1,ten_second:3,binary3:4,unit_second:4,binary2:4,
				color_frame_flag:1,drop_frame_flag:1,ten_frame:2,binary1:4,unit_frame:4;
#endif

#ifndef _WINDOWS
}__attribute__((packed));
#else
};
#endif

struct LTC_TimeCode_union
{
	union 
	{
		struct LTC_TimeCode struct_ltc;
		BLUE_UINT64 lt_64_value;
	};
};

/*
	This is used to unpack the timecode word properly and quickly
	in RP188 each 4 bits of the timecode is put into a 10 bit word.
	So this structure helps in decoding 
*/
struct nibble_struct
{
	BLUE_UINT8	first_half:4,second_half:4;

#ifndef _WINDOWS
}__attribute__((packed));
#else
};
#endif

struct TimeCode
{
	union 
	{
	struct LTC_TimeCode struct_ltc;
	BLUE_UINT64 ltc;
	struct nibble_struct ltc_char[8]; 
	};
	
#ifndef _WINDOWS
}__attribute__((packed));
#else
};
#endif

struct HANCTimeCodeStruct 
{	
#if defined(__LITTLE_ENDIAN__) || defined(_WINDOWS) || defined(BLUE_LINUX_CODE)
	BLUE_UINT32	zero_0:3,
				DBB_0:1,
				ANC_0:4,
				partiy_0:1,
				NotBit8_0:1,
				zero_1:3,
				DBB_1:1,
				ANC_1:4,
				partiy_1:1,
				NotBit8_1:1,
				zero_2:3,
				DBB_2:1,
				ANC_2:4,
				partiy_2:1,
				NotBit8_2:1,
				akiraControlBits:2;
#else
	BLUE_UINT32 akiraControlBits:2,
				Notbit8_2:1,
				partiy_2:1,
				ANC_2:4,
				DBB_2:1,
				zero_2:3,
				NotBit81_1:1,
				partiy_1:1,
				ANC_1:4,
				DBB_1:1,
				zero_1:3,
				NotBit8_0:1,
				partiy_0:1,
				ANC_0:4,
				DBB_0:1,
				zero_0:3;
#endif

#ifndef _WINDOWS
}__attribute__((packed));
#else
};
#endif

union HANCTimeCode
{
	struct HANCTimeCodeStruct hanc_struct;
	BLUE_UINT32 hanc_word;
};

struct BAG2VancTimeCodeStruct 
{	
#if defined(__LITTLE_ENDIAN__) || defined(_WINDOWS) || defined(BLUE_LINUX_CODE)
	BLUE_UINT16	zero_0:3,
				DBB_0:1,
				ANC_0:4,
				partiy_0:1,
				NotBit8_0:1;
#else
	BLUE_UINT16	NotBit8_0:1,
				partiy_0:1,
				ANC_0:4,
				DBB_0:1,
				zero_0:3;
#endif

#ifndef _WINDOWS
}__attribute__((packed));
#else
};
#endif

union BAG2VancTimeCode
{
	struct BAG2VancTimeCodeStruct vanc_struct;
	BLUE_UINT16 vanc_word;
};


inline BLUE_UINT64 convert_countto_timecode(BLUE_UINT32 frame_count,BLUE_UINT32 framePerSec)
{
	unsigned int frames ,second,minutes ,hour   ;
	struct TimeCode rp188_timcode;
	hour = frame_count/(60*60*framePerSec);
	minutes = frame_count%(60*60*framePerSec);
	second = minutes%(60*framePerSec);
	frames = second %framePerSec;
	second = second/(framePerSec);
	minutes=minutes/(60*framePerSec);
	rp188_timcode.ltc = 0;
	rp188_timcode.struct_ltc.unit_frame = (frames%10);
	rp188_timcode.struct_ltc.ten_frame = (frames/10);
	rp188_timcode.struct_ltc.unit_second = (second%10);
	rp188_timcode.struct_ltc.ten_second = (second/10);
	rp188_timcode.struct_ltc.unit_minute = (minutes%10);
	rp188_timcode.struct_ltc.ten_minute = (minutes/10);
	rp188_timcode.struct_ltc.unit_hours = (hour%10);
	rp188_timcode.struct_ltc.ten_hours = (hour/10);
	
	return rp188_timcode.ltc;
}


inline BLUE_UINT64 convert_timecode_to_count(BLUE_UINT64 timecode,
											 BLUE_UINT32 framePerSec,
											 unsigned int & frames ,
											 unsigned int & second,
											 unsigned int & minutes ,
											 unsigned int & hours)
{
	
	struct TimeCode rp188_timecode;
	rp188_timecode.ltc = timecode;
	hours  = (BLUE_UINT32)((unsigned int)rp188_timecode.struct_ltc.ten_hours*10)+(unsigned int)rp188_timecode.struct_ltc.unit_hours;
	minutes = (BLUE_UINT32)((unsigned int)rp188_timecode.struct_ltc.ten_minute*10)+(unsigned int)rp188_timecode.struct_ltc.unit_minute;
	second =  (BLUE_UINT32)((unsigned int)rp188_timecode.struct_ltc.ten_second*10)+(unsigned int)rp188_timecode.struct_ltc.unit_second;
	frames =	 (BLUE_UINT32)((unsigned int)rp188_timecode.struct_ltc.ten_frame*10)+(unsigned int)rp188_timecode.struct_ltc.unit_frame;		
	return rp188_timecode.ltc;
}

// Determine endianess at run-time
inline BLUE_UINT32 Int32SwapBigLittle(const BLUE_UINT32 i)
{
    unsigned char c1, c2, c3, c4;
	const int endian = 1;
	#define is_bigendian() ( (*(char*) & endian) == 0 )

    if (is_bigendian())
	{
        c1 = i & 255;
        c2 = (i >> 8) & 255;
        c3 = (i >> 16) & 255;
        c4 = (i >> 24) & 255;
		
		return ((int)c1 << 24) + ((int)c2 << 16) + ((int)c3 << 8) + c4;
    }
	else
	{
        return i;
	}
}

#ifdef _WINDOWS
#pragma pack(pop)
#endif