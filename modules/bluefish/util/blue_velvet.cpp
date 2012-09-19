/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "../StdAfx.h"

#include "blue_velvet.h"

#include <common/utility/string.h>

#include <core/video_format.h>

namespace caspar { namespace bluefish {
	
CBlueVelvet4* (*BlueVelvetFactory4)() = nullptr;
void (*BlueVelvetDestroy)(CBlueVelvet4* pObj) = nullptr;
const char*	(*BlueVelvetVersion)() = nullptr;
BLUE_UINT32 (*encode_hanc_frame)(struct hanc_stream_info_struct * hanc_stream_ptr, void * audio_pcm_ptr,BLUE_UINT32 no_audio_ch,BLUE_UINT32 no_audio_samples,BLUE_UINT32 nTypeOfSample,BLUE_UINT32 emb_audio_flag) = nullptr;
BLUE_UINT32 (*encode_hanc_frame_ex)(BLUE_UINT32 card_type, struct hanc_stream_info_struct * hanc_stream_ptr, void * audio_pcm_ptr, BLUE_UINT32 no_audio_ch,	BLUE_UINT32 no_audio_samples, BLUE_UINT32 nTypeOfSample, BLUE_UINT32 emb_audio_flag) = nullptr;

void blue_velvet_initialize()
{
#ifdef _DEBUG
	std::string module_str = "BlueVelvet3_d.dll";
#else
	std::string module_str = "BlueVelvet3.dll";
#endif

	auto module = LoadLibrary(widen(module_str).c_str());
	if(!module)
		LoadLibrary(widen(std::string(getenv("SystemDrive")) + "\\Program Files\\Bluefish444\\Driver\\" + module_str).c_str());
	if(!module)
		LoadLibrary(widen(std::string(getenv("SystemDrive")) + "\\Program Files (x86)\\BlueFish444\\Driver\\" + module_str).c_str());
	if(!module)
		BOOST_THROW_EXCEPTION(file_not_found() << msg_info("Could not find BlueVelvet3.dll. Required drivers are not installed."));
	static std::shared_ptr<void> lib(module, FreeLibrary);
	BlueVelvetFactory4 = reinterpret_cast<decltype(BlueVelvetFactory4)>(GetProcAddress(module, "BlueVelvetFactory4"));
	BlueVelvetDestroy  = reinterpret_cast<decltype(BlueVelvetDestroy)>(GetProcAddress(module, "BlueVelvetDestroy"));
	BlueVelvetVersion  = reinterpret_cast<decltype(BlueVelvetVersion)>(GetProcAddress(module, "BlueVelvetVersion"));
}

void blue_hanc_initialize()
{
#ifdef _DEBUG
	std::string module_str = "BlueHancUtils_d.dll";
#else
	std::string module_str = "BlueHancUtils.dll";
#endif
	
	auto module = LoadLibrary(widen(module_str).c_str());
	if(!module)
		LoadLibrary(widen(std::string(getenv("SystemDrive")) + "\\Program Files\\Bluefish444\\Driver\\" + module_str).c_str());
	if(!module)
		LoadLibrary(widen(std::string(getenv("SystemDrive")) + "\\Program Files (x86)\\BlueFish444\\Driver\\" + module_str).c_str());
	if(!module)
		BOOST_THROW_EXCEPTION(file_not_found() << msg_info("Could not find BlueHancUtils.dll. Required drivers are not installed."));
	static std::shared_ptr<void> lib(module, FreeLibrary);
	encode_hanc_frame	 = reinterpret_cast<decltype(encode_hanc_frame)>(GetProcAddress(module, "encode_hanc_frame"));
	encode_hanc_frame_ex = reinterpret_cast<decltype(encode_hanc_frame_ex)>(GetProcAddress(module, "encode_hanc_frame_ex"));
}

void blue_initialize()
{
	blue_hanc_initialize();
	blue_velvet_initialize();
}

EVideoMode vid_fmt_from_video_format(const core::video_format::type& fmt) 
{
	switch(fmt)
	{
	case core::video_format::pal:			return VID_FMT_PAL;
	case core::video_format::ntsc:			return VID_FMT_NTSC;
	case core::video_format::x576p2500:		return VID_FMT_INVALID;	//not supported
	case core::video_format::x720p2398:		return VID_FMT_720P_2398;
	case core::video_format::x720p2400:		return VID_FMT_720P_2400;
	case core::video_format::x720p2500:		return VID_FMT_720P_2500;
	case core::video_format::x720p5000:		return VID_FMT_720P_5000;
	case core::video_format::x720p2997:		return VID_FMT_720P_2997;
	case core::video_format::x720p5994:		return VID_FMT_720P_5994;
	case core::video_format::x720p3000:		return VID_FMT_720P_3000;
	case core::video_format::x720p6000:		return VID_FMT_720P_6000;
	case core::video_format::x1080p2398:	return VID_FMT_1080P_2397;
	case core::video_format::x1080p2400:	return VID_FMT_1080P_2400;
	case core::video_format::x1080i5000:	return VID_FMT_1080I_5000;
	case core::video_format::x1080i5994:	return VID_FMT_1080I_5994;
	case core::video_format::x1080i6000:	return VID_FMT_1080I_6000;
	case core::video_format::x1080p2500:	return VID_FMT_1080P_2500;
	case core::video_format::x1080p2997:	return VID_FMT_1080P_2997;
	case core::video_format::x1080p3000:	return VID_FMT_1080P_3000;
	case core::video_format::x1080p5000:	return VID_FMT_1080P_5000;
	case core::video_format::x1080p5994:	return VID_FMT_1080P_5994;
	case core::video_format::x1080p6000:	return VID_FMT_1080P_6000;
	default:								return VID_FMT_INVALID;
	}
}

bool is_epoch_card(CBlueVelvet4& blue)
{
	switch(blue.has_video_cardtype())
	{
	case CRD_BLUE_EPOCH_HORIZON:
	case CRD_BLUE_EPOCH_CORE:
	case CRD_BLUE_EPOCH_ULTRA:
	case CRD_BLUE_EPOCH_2K_HORIZON:
	case CRD_BLUE_EPOCH_2K_CORE:
	case CRD_BLUE_EPOCH_2K_ULTRA:
	case CRD_BLUE_CREATE_HD:
	case CRD_BLUE_CREATE_2K:
	case CRD_BLUE_CREATE_2K_ULTRA:
	case CRD_BLUE_SUPER_NOVA:
		return true;
	default:
		return false;
	}
}

std::wstring get_card_desc(CBlueVelvet4& blue)
{
	switch(blue.has_video_cardtype()) 
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
	case CRD_BLUE_ENVY:					return L"Blue Envy"; // Mini Din 
	case CRD_BLUE_PRIDE:				return L"Blue Pride";//Mini Din Output 
	case CRD_BLUE_GREED:				return L"Blue Greed";
	case CRD_BLUE_INGEST:				return L"Blue Ingest";
	case CRD_BLUE_SD_DUALLINK:			return L"Blue SD Duallink";
	case CRD_BLUE_CATALYST:				return L"Blue Catalyst";
	case CRD_BLUE_SD_DUALLINK_PRO:		return L"Blue SD Duallink Pro";
	case CRD_BLUE_SD_INGEST_PRO:		return L"Blue SD Ingest pro";
	case CRD_BLUE_SD_DEEPBLUE_LITE_PRO:	return L"Blue SD Deepblue lite Pro";
	case CRD_BLUE_SD_SINGLELINK_PRO:	return L"Blue SD Singlelink Pro";
	case CRD_BLUE_SD_IRIDIUM_AV_PRO:	return L"Blue SD Iridium AV Pro";
	case CRD_BLUE_SD_FIDELITY:			return L"Blue SD Fidelity";
	case CRD_BLUE_SD_FOCUS:				return L"Blue SD Focus";
	case CRD_BLUE_SD_PRIME:				return L"Blue SD Prime";
	case CRD_BLUE_EPOCH_2K_CORE:		return L"Blue Epoch 2K Core";
	case CRD_BLUE_EPOCH_2K_ULTRA:		return L"Blue Epoch 2K Ultra";
	case CRD_BLUE_EPOCH_HORIZON:		return L"Blue Epoch Horizon";
	case CRD_BLUE_EPOCH_CORE:			return L"Blue Epoch Core";
	case CRD_BLUE_EPOCH_ULTRA:			return L"Blue Epoch Ultra";
	case CRD_BLUE_CREATE_HD:			return L"Blue Create HD";
	case CRD_BLUE_CREATE_2K:			return L"Blue Create 2K";
	case CRD_BLUE_CREATE_2K_ULTRA:		return L"Blue Create 2K Ultra";
	default:							return L"Unknown";
	}
}

EVideoMode get_video_mode(CBlueVelvet4& blue, const core::video_format_desc& format_desc)
{
	EVideoMode vid_fmt = VID_FMT_INVALID;
	auto desiredVideoFormat = vid_fmt_from_video_format(format_desc.format);
	int videoModeCount = blue.count_video_mode();
	for(int videoModeIndex = 1; videoModeIndex <= videoModeCount; ++videoModeIndex) 
	{
		EVideoMode videoMode = blue.enum_video_mode(videoModeIndex);
		if(videoMode == desiredVideoFormat) 
			vid_fmt = videoMode;			
	}
	if(vid_fmt == VID_FMT_INVALID)
		BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("video-mode not supported.") << arg_value_info(narrow(format_desc.name)));

	return vid_fmt;
}

safe_ptr<CBlueVelvet4> create_blue()
{
	if(!BlueVelvetFactory4 || !encode_hanc_frame || !encode_hanc_frame)
		BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Bluefish drivers not found."));

	return safe_ptr<CBlueVelvet4>(BlueVelvetFactory4(), BlueVelvetDestroy);
}

safe_ptr<CBlueVelvet4> create_blue(size_t device_index)
{
	auto blue = create_blue();
	
	if(BLUE_FAIL(blue->device_attach(device_index, FALSE))) 
		BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to attach device."));

	return blue;
}

}}