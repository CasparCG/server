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

#pragma once

#include <Windows.h>

#include <BlueVelvet4.h>
#include <BlueHancUtils.h>

#include <common/memory.h>
#include <common/except.h>

namespace caspar { 

namespace core {

struct video_format_desc;

}

namespace bluefish {

extern const char* (*BlueVelvetVersion)();
extern BLUE_UINT32 (*encode_hanc_frame)(struct hanc_stream_info_struct * hanc_stream_ptr, void * audio_pcm_ptr,BLUE_UINT32 no_audio_ch,BLUE_UINT32 no_audio_samples,BLUE_UINT32 nTypeOfSample,BLUE_UINT32 emb_audio_flag);
extern BLUE_UINT32 (*encode_hanc_frame_ex)(BLUE_UINT32 card_type, struct hanc_stream_info_struct * hanc_stream_ptr, void * audio_pcm_ptr, BLUE_UINT32 no_audio_ch,	BLUE_UINT32 no_audio_samples, BLUE_UINT32 nTypeOfSample, BLUE_UINT32 emb_audio_flag);

void blue_initialize();

spl::shared_ptr<CBlueVelvet4> create_blue();
spl::shared_ptr<CBlueVelvet4> create_blue(int device_index);
bool is_epoch_card(CBlueVelvet4& blue);
std::wstring get_card_desc(CBlueVelvet4& blue);
EVideoMode get_video_mode(CBlueVelvet4& blue, const core::video_format_desc& format_desc);

template<typename T>
int set_card_property(T& pSdk, ULONG prop, ULONG value)
{
	VARIANT variantValue;
	variantValue.vt  = VT_UI4;
	variantValue.ulVal = value;
	return (pSdk->SetCardProperty(prop,variantValue));
}

}}