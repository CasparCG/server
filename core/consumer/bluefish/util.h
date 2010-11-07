#pragma once

#include <BlueVelvet4.h>
#include "../../frame/frame_format.h"

#include <memory>

namespace caspar { namespace core { namespace bluefish {
	
inline bool is_epoch_card(int card_type)
{
	return	card_type == CRD_BLUE_EPOCH_2K ||	
			card_type == CRD_BLUE_EPOCH_HORIZON || 
			card_type == CRD_BLUE_EPOCH_2K_CORE ||  
			card_type == CRD_BLUE_EPOCH_2K_ULTRA || 
			card_type == CRD_BLUE_EPOCH_CORE || 
			card_type == CRD_BLUE_EPOCH_ULTRA;
}

inline EVideoMode vid_fmt_from_frame_format(const frame_format& fmt) 
{
	switch(fmt)
	{
	case frame_format::pal:			return VID_FMT_PAL;
	case frame_format::ntsc:		return VID_FMT_NTSC;
	case frame_format::x576p2500:	return VID_FMT_INVALID;	//not supported
	case frame_format::x720p5000:	return VID_FMT_720P_5000;
	case frame_format::x720p5994:	return VID_FMT_720P_5994;
	case frame_format::x720p6000:	return VID_FMT_720P_6000;
	case frame_format::x1080p2397:	return VID_FMT_1080P_2397;
	case frame_format::x1080p2400:	return VID_FMT_1080P_2400;
	case frame_format::x1080i5000:	return VID_FMT_1080I_5000;
	case frame_format::x1080i5994:	return VID_FMT_1080I_5994;
	case frame_format::x1080i6000:	return VID_FMT_1080I_6000;
	case frame_format::x1080p2500:	return VID_FMT_1080P_2500;
	case frame_format::x1080p2997:	return VID_FMT_1080P_2997;
	case frame_format::x1080p3000:	return VID_FMT_1080P_3000;
	default:						return VID_FMT_INVALID;
	}
}

inline wchar_t* get_card_desc(int cardType)
{
	switch(cardType) 
	{
	case CRD_BLUEDEEP_LT:				return L"Deepblue LT";// D64 Lite
	case CRD_BLUEDEEP_SD:				return L"Iridium SD";// Iridium SD
	case CRD_BLUEDEEP_AV:				return L"Iridium AV";// Iridium AV
	case CRD_BLUEDEEP_IO:				return L"Deepblue IO";// D64 Full
	case CRD_BLUEWILD_AV:				return L"Wildblue AV";// D64 AV
	case CRD_IRIDIUM_HD:				return L"Iridium HD";// * Iridium HD
	case CRD_BLUEWILD_RT:				return L"Wildblue RT";// D64 RT
	case CRD_BLUEWILD_HD:				return L"Wildblue HD";// * BadAss G2
	case CRD_REDDEVIL:					return L"Iridium Full";// Iridium Full
	case CRD_BLUEDEEP_HD:	
	case CRD_BLUEDEEP_HDS:				return L"Reserved for \"BasAss G2";// * BadAss G2 variant, proposed, reserved
	case CRD_BLUE_ENVY:					return L"Blue envy"; // Mini Din 
	case CRD_BLUE_PRIDE:				return L"Blue pride";//Mini Din Output 
	case CRD_BLUE_GREED:				return L"Blue greed";
	case CRD_BLUE_INGEST:				return L"Blue ingest";
	case CRD_BLUE_SD_DUALLINK:			return L"Blue SD duallink";
	case CRD_BLUE_CATALYST:				return L"Blue catalyst";
	case CRD_BLUE_SD_DUALLINK_PRO:		return L"Blue SD duallink pro";
	case CRD_BLUE_SD_INGEST_PRO:		return L"Blue SD ingest pro";
	case CRD_BLUE_SD_DEEPBLUE_LITE_PRO:	return L"Blue SD deepblue lite pro";
	case CRD_BLUE_SD_SINGLELINK_PRO:	return L"Blue SD singlelink pro";
	case CRD_BLUE_SD_IRIDIUM_AV_PRO:	return L"Blue SD iridium AV pro";
	case CRD_BLUE_SD_FIDELITY:			return L"Blue SD fidelity";
	case CRD_BLUE_SD_FOCUS:				return L"Blue SD focus";
	case CRD_BLUE_SD_PRIME:				return L"Blue SD prime";
	case CRD_BLUE_EPOCH_2K_CORE:		return L"Blue epoch 2k core";
	case CRD_BLUE_EPOCH_2K_ULTRA:		return L"Blue epoch 2k ultra";
	case CRD_BLUE_EPOCH_HORIZON:		return L"Blue epoch horizon";
	case CRD_BLUE_EPOCH_CORE:			return L"Blue epoch core";
	case CRD_BLUE_EPOCH_ULTRA:			return L"Blue epoch ultra";
	case CRD_BLUE_CREATE_HD:			return L"Blue create HD";
	case CRD_BLUE_CREATE_2K:			return L"Blue create 2k";
	case CRD_BLUE_CREATE_2K_ULTRA:		return L"Blue create 2k ultra";
	default:							return L"Unknown";
	}
}

inline int set_card_property(CBlueVelvet4 * pSdk, ULONG prop, ULONG value)
{
	VARIANT variantValue;
	variantValue.vt  = VT_UI4;
	variantValue.ulVal = value;
	return (pSdk->SetCardProperty(prop,variantValue));
}

inline int set_card_property(const std::shared_ptr<CBlueVelvet4> pSdk, ULONG prop, ULONG value)
{
	return set_card_property(pSdk.get(), prop, value);
}

}}}