#pragma once

#include <BlueVelvet4.h>
#include "../../frame/Frame.h"

namespace caspar { namespace bluefish {
	
inline unsigned long VidFmtFromFrameFormat(FrameFormat fmt) 
{
	switch(fmt)
	{
	case FFormatPAL:		return VID_FMT_PAL;
	case FFormatNTSC:		return VID_FMT_NTSC;
	case FFormat576p2500:	return ULONG_MAX;	//not supported
	case FFormat720p5000:	return VID_FMT_720P_5000;
	case FFormat720p5994:	return VID_FMT_720P_5994;
	case FFormat720p6000:	return VID_FMT_720P_6000;
	case FFormat1080p2397:	return VID_FMT_1080P_2397;
	case FFormat1080p2400:	return VID_FMT_1080P_2400;
	case FFormat1080i5000:	return VID_FMT_1080I_5000;
	case FFormat1080i5994:	return VID_FMT_1080I_5994;
	case FFormat1080i6000:	return VID_FMT_1080I_6000;
	case FFormat1080p2500:	return VID_FMT_1080P_2500;
	case FFormat1080p2997:	return VID_FMT_1080P_2997;
	case FFormat1080p3000:	return VID_FMT_1080P_3000;
	default:				return ULONG_MAX;
	}
}

inline TCHAR* GetBluefishCardDesc(int cardType)
{
	switch(cardType) 
	{
	case CRD_BLUEDEEP_LT:				return TEXT("Deepblue LT");// D64 Lite
	case CRD_BLUEDEEP_SD:				return TEXT("Iridium SD");// Iridium SD
	case CRD_BLUEDEEP_AV:				return TEXT("Iridium AV");// Iridium AV
	case CRD_BLUEDEEP_IO:				return TEXT("Deepblue IO");// D64 Full
	case CRD_BLUEWILD_AV:				return TEXT("Wildblue AV");// D64 AV
	case CRD_IRIDIUM_HD:				return TEXT("Iridium HD");// * Iridium HD
	case CRD_BLUEWILD_RT:				return TEXT("Wildblue RT");// D64 RT
	case CRD_BLUEWILD_HD:				return TEXT("Wildblue HD");// * BadAss G2
	case CRD_REDDEVIL:					return TEXT("Iridium Full");// Iridium Full
	case CRD_BLUEDEEP_HD:	
	case CRD_BLUEDEEP_HDS:				return TEXT("Reserved for \"BasAss G2");// * BadAss G2 variant, proposed, reserved
	case CRD_BLUE_ENVY:					return TEXT("Blue envy"); // Mini Din 
	case CRD_BLUE_PRIDE:				return TEXT("Blue pride");//Mini Din Output 
	case CRD_BLUE_GREED:				return TEXT("Blue greed");
	case CRD_BLUE_INGEST:				return TEXT("Blue ingest");
	case CRD_BLUE_SD_DUALLINK:			return TEXT("Blue SD duallink");
	case CRD_BLUE_CATALYST:				return TEXT("Blue catalyst");
	case CRD_BLUE_SD_DUALLINK_PRO:		return TEXT("Blue SD duallink pro");
	case CRD_BLUE_SD_INGEST_PRO:		return TEXT("Blue SD ingest pro");
	case CRD_BLUE_SD_DEEPBLUE_LITE_PRO:	return TEXT("Blue SD deepblue lite pro");
	case CRD_BLUE_SD_SINGLELINK_PRO:	return TEXT("Blue SD singlelink pro");
	case CRD_BLUE_SD_IRIDIUM_AV_PRO:	return TEXT("Blue SD iridium AV pro");
	case CRD_BLUE_SD_FIDELITY:			return TEXT("Blue SD fidelity");
	case CRD_BLUE_SD_FOCUS:				return TEXT("Blue SD focus");
	case CRD_BLUE_SD_PRIME:				return TEXT("Blue SD prime");
	case CRD_BLUE_EPOCH_2K_CORE:		return TEXT("Blue epoch 2k core");
	case CRD_BLUE_EPOCH_2K_ULTRA:		return TEXT("Blue epoch 2k ultra");
	case CRD_BLUE_EPOCH_HORIZON:		return TEXT("Blue epoch horizon");
	case CRD_BLUE_EPOCH_CORE:			return TEXT("Blue epoch core");
	case CRD_BLUE_EPOCH_ULTRA:			return TEXT("Blue epoch ultra");
	case CRD_BLUE_CREATE_HD:			return TEXT("Blue create HD");
	case CRD_BLUE_CREATE_2K:			return TEXT("Blue create 2k");
	case CRD_BLUE_CREATE_2K_ULTRA:		return TEXT("Blue create 2k ultra");
	default:							return TEXT("Unknown");
	}
}

inline unsigned int GetAudioSamplesPerFrame(unsigned int nVideoSignal,unsigned int frame_no)
{
	unsigned int samples_to_read = 1920;
	UINT32 NTSC_frame_seq[]={1602,1601,1602,1601,1602};
	UINT32 p59_frame_seq[]={801,800,801,801,801};
	UINT32 p23_frame_seq[]={2002,2002,2002,2002,2002};

	switch (nVideoSignal)
	{
	case VID_FMT_1080PSF_2397:
	case VID_FMT_1080P_2397:
	case VID_FMT_2048_1080PSF_2397:
	case VID_FMT_2048_1080P_2397:
		samples_to_read = p23_frame_seq[frame_no%5];
		break;
	case VID_FMT_NTSC:
	case VID_FMT_1080I_5994:
	case VID_FMT_1080P_2997:
	case VID_FMT_1080PSF_2997:
		samples_to_read = NTSC_frame_seq[frame_no%5];
		break;
	case VID_FMT_720P_5994:
		samples_to_read = p59_frame_seq[frame_no%5];
		break;
	case VID_FMT_1080PSF_2400:
	case VID_FMT_1080P_2400:
	case VID_FMT_2048_1080PSF_2400:
	case VID_FMT_2048_1080P_2400:
		samples_to_read = 2000;
		break;
	case VID_FMT_1080I_6000:
	case VID_FMT_1080P_3000:
	case VID_FMT_1080PSF_3000:
		samples_to_read = 1600;
		break;
	case VID_FMT_720P_6000:
		samples_to_read = 800;
		break;
	case VID_FMT_720P_5000:
		samples_to_read = 960;
		break;
	case VID_FMT_PAL:
	case VID_FMT_1080I_5000:
	case VID_FMT_1080P_2500:
	case VID_FMT_1080PSF_2500:
	default:
		samples_to_read = 1920;
		break;
	}
	return samples_to_read;
}

inline int SetCardProperty(CBlueVelvet4 * pSdk,ULONG prop, ULONG value)
{
	VARIANT variantValue;
	variantValue.vt  = VT_UI4;
	variantValue.ulVal = value;
	return (pSdk->SetCardProperty(prop,variantValue));
}

inline int SetCardProperty(const std::shared_ptr<CBlueVelvet4> pSdk, ULONG prop, ULONG value)
{
	return SetCardProperty(pSdk.get(), prop, value);
}

inline int GetCardProperty(CBlueVelvet4 * pSdk,ULONG prop,ULONG & value)
{
	VARIANT variantValue;
	int errorCode;
	variantValue.vt  = VT_UI4;
	errorCode = pSdk->QueryCardProperty(prop,variantValue);
	value = variantValue.ulVal;
	return (errorCode);
}

static const int MAX_HANC_BUFFER_SIZE = 256*1024;
}}