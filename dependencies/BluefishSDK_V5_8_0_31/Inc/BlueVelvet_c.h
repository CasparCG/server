/*
// ==========================================================================
//	Bluefish444 BlueVelvet SDK library
//
//  BlueSD_c.h
//  Constants header
//	LARGELY superseded by dynamic calculations


  $Id: BlueVelvet_c.h,v 1.4 2002/10/02 00:29:53 cameron Exp $
//
//  developed by  : Cameron Duffy   (C) 2002 Bluefish444 P/L
// ==========================================================================
//
*/

//----------------------------------------------------------------------------
#ifndef _BLUEVELVET_C_H
#define _BLUEVELVET_C_H

//----------------------------------------------------------------------------------------------------------------------
// File SUB-types supported
//
typedef enum
{
	EBlue_10BIT_NTSC=0,		// 10 BIT NTSC
	EBlue_10BIT_PAL,		// 10 BIT PAL
	EBlue_08BIT_NTSC,		//  8 BIT NTSC
	EBlue_08BIT_PAL,		//  8 BIT PAL
	EBlue_32BIT_NTSC,		// 32 BIT NTSC (ARGB - uncompressed)
	EBlue_32BIT_PAL		// 32 BIT PAL  (ARGB - uncompressed)
} EBlueFileId;

// File subtype ID;s
#define	BLUE_FILE_10BIT_NTSC	0	// 10 BIT NTSC
#define	BLUE_FILE_10BIT_PAL		1	// 10 BIT PAL
#define	BLUE_FILE_08BIT_NTSC	2	//  8 BIT NTSC
#define	BLUE_FILE_08BIT_PAL		3	//  8 BIT PAL
#define	BLUE_FILE_32BIT_NTSC	4	// 32 BIT NTSC (ARGB - uncompressed)
#define	BLUE_FILE_32BIT_PAL		5	// 32 BIT PAL  (ARGB - uncompressed)

// File subtype FOURCC
#define	BLUE_SUBTYPE_10BIT_NTSC	0x52594e5f	//'RYN_'	// 10 BIT NTSC
#define	BLUE_SUBTYPE_10BIT_PAL	0x5259515f	//'RYP_'	// 10 BIT PAL
#define	BLUE_SUBTYPE_08BIT_NTSC	0x52384e5f	//'R8N_'	//  8 BIT NTSC
#define	BLUE_SUBTYPE_08BIT_PAL	0x5238515f	//'R8P_'	//  8 BIT PAL
#define	BLUE_SUBTYPE_32BIT_NTSC	0x52524e5f	//'RRN_'	// 32 BIT NTSC (ARGB - uncompressed)
#define	BLUE_SUBTYPE_32BIT_PAL	0x5252415f	//'RRP_'	// 32 BIT PAL  (ARGB - uncompressed)

#define	BLUE_ROOTED_SUBTYPE		0x62626262	//'XXXX'
#define	BLUE_FILE_NOVIDEO		0x62626262	//'XXXX'
#define	BLUE_FILE_TYPE			0x5244565f	//'RDV_'
#define	BLUE_CLASS_ID			0x52444456	//'RDDV'

// align this structure on 512 byte boundary!
typedef struct
{
	char				name[20];		// "PREMIERE RDV_FILE";
	ULONG				hasAudio;		// See BLUE_SUBTYPE_???
	ULONG				VideoSubtype;	// See BLUE_SUBTYPE_???
	ULONG				width;			// width of frame in pixels
	ULONG				height;			// height of frame in pixels (can get video mode)
	ULONG				rowbytes;		// total bytes in row (can get mem format from this and width)
	ULONG				numFrames;		// number of frames in file
	ULONG				frameOffset;	// GOLDEN frame size
	ULONG				duration;		// TDB - value = total number of frames
	long				scale;			// TDB - scale = scale / samplesize = timebase
	long				sampleSize;		// TDB - sampleSize = 1 or 100 if 29.97 fps

	ULONG				gFmtVid;
	ULONG				gFmtMem;
	ULONG				gFmtUpd;
	ULONG				gFmtRes;
	// 76 bytes
	char				orgtime[20];	// These fields map directly to those in imTimeInfoRec.
	char				alttime[20];
	char				orgreel[40];
	char				altreel[40];
	// 196 bytes
	char				logcomment[256];
	// 452 bytes
//	char				pad[512-452-4];
	char				pad[56];
	// For disk speed to work, this structure MUST be a multiple of sector size
	ULONG				len;			// Length of TRAILER, *always* last!
} RDV_File2_OLD;
//#define	SIZE_RDV_FILE	512

#define kGoldenPageSize	4096

typedef struct
{
	char				name[20];		// "PREMIERE RDV_FILE ";
	ULONG				hasAudio;		// See BLUE_SUBTYPE_???
	ULONG				VideoSubtype;	// See BLUE_SUBTYPE_???
	ULONG				width;			// width of frame in pixels
	ULONG				height;			// height of frame in pixels (can get video mode)
	ULONG				rowbytes;		// total bytes in row (can get mem format from this and width)
	ULONG				numFrames;		// number of frames in file
	ULONG				frameOffset;	// GOLDEN frame size
	ULONG				duration;		// TDB - value = total number of frames
	long				scale;			// TDB - scale = scale / samplesize = timebase
	long				sampleSize;		// TDB - sampleSize = 1 or 100 if 29.97 fps

	ULONG				gFmtVid;
	ULONG				gFmtMem;
	ULONG				gFmtUpd;
	ULONG				gFmtRes;
	// 76 bytes
	char				orgtime[20];	// These fields map directly to those in imTimeInfoRec.
	char				alttime[20];
	char				orgreel[40];
	char				altreel[40];
	// 196 bytes
	char				logcomment[256];
	// 452 bytes
//	char				pad[512-452-4];
	ULONG				audioSampleRate;		// 48000 or 96000
	ULONG				numChannels;			// 2, 4, or 6
	ULONG				numAudioBlocks;			// how many in the file?
	// 464 bytes
	char				pad[36];

	_int64				audioBlockOffsets[kGoldenPageSize * 4];		// something like 4.5 hours max length (enough for now I guess)
	ULONG				audioBlockSizes[kGoldenPageSize * 4];

	// For disk speed to work, this structure MUST be a multiple of sector size
	ULONG				len;			// Length of TRAILER, *always* last!
} RDV_File2;

#endif //_BLUEVELVET_C_H
