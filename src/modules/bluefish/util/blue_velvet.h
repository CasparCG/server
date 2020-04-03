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

#include <common/except.h>
#include <common/memory.h>
#include <core/fwd.h>
#include <core/video_format.h>

#include "../interop/BlueDriver_p.h"

namespace caspar { namespace bluefish {

class bvc_wrapper
{
  public:
    bvc_wrapper(); // bfcFactory + function pointer lookups
    const char* get_version();

    BLUE_U32 enumerate(int* iDevices);
    BLUE_U32 query_card_type(int* iCardType, int iDeviceID);

    BLUE_U32 attach(int iDeviceId);
    BLUE_U32 detach();
    BLUE_U32 set_multilink(const int iDeviceID, const int memChannel);

    BLUE_U32 get_card_property32(const int iProperty, unsigned int* nValue);
    BLUE_U32 set_card_property32(const int iProperty, const unsigned int nValue);

    BLUE_U32 get_card_property64(const int iProperty, unsigned long long* nValue);
    BLUE_U32 set_card_property64(const int iProperty, const unsigned long long nValue);

    BLUE_U32
    system_buffer_write(unsigned char* pPixels, unsigned long ulSize, unsigned long ulBufferID, unsigned long ulOffset);
    BLUE_U32
    system_buffer_read(unsigned char* pPixels, unsigned long ulSize, unsigned long ulBufferID, unsigned long ulOffset);

    BLUE_U32 video_playback_stop(int iWait, int iFlush);
    BLUE_U32 video_playback_start(int wait, int loop);
    BLUE_U32 video_playback_allocate(unsigned long* buffer_id, unsigned long* underrun);
    BLUE_U32
    video_playback_present(unsigned long buffer_id, unsigned long count, unsigned long keep, unsigned long odd);

    BLUE_U32 wait_video_output_sync(unsigned long ulUpdateType, unsigned long* ulFieldCount);
    BLUE_U32 wait_video_input_sync(unsigned long ulUpdateType, unsigned long* ulFieldCount);

    BLUE_U32 render_buffer_update(unsigned long ulBufferID);
    BLUE_U32 render_buffer_capture(unsigned long ulBufferID);

    BLUE_U32 encode_hanc_frame(unsigned int                    nCardType,
                               struct hanc_stream_info_struct* pHancEncodeInfo,
                               void*                           pAudioBuffer,
                               unsigned int                    nAudioChannels,
                               unsigned int                    nAudioSamples,
                               unsigned int                    nSampleType,
                               unsigned int                    nAudioFlags);
    BLUE_U32
    decode_hanc_frame(unsigned int nCardType, unsigned int* pHancBuffer, struct hanc_decode_struct* pHancDecodeInfo);

    BLUE_U32 get_frame_info_for_video_mode(const unsigned int nVideoModeExt,
                                           unsigned int*      nWidth,
                                           unsigned int*      nHeight,
                                           unsigned int*      nRate,
                                           unsigned int*      bIs1001,
                                           unsigned int*      bIsProgressive);
    BLUE_U32 get_bytes_per_frame(EVideoModeExt nVideoMode,
                                 EMemoryFormat nMemoryFormat,
                                 EUpdateMethod nUpdateMethod,
                                 unsigned int* nBytesPerFrame);

    std::string  get_string_for_card_type(unsigned int nCardType);
    std::wstring get_wstring_for_video_mode(unsigned int nVideoMode);

    int get_num_audio_samples_for_frame(const BLUE_U32 nVideoMode, const BLUE_U32 nFrameNo);

    // UHD Conversion functions...
    BLUE_U32 convert_2si_to_sq(const BLUE_U32 Width, const BLUE_U32 Height, BLUE_VOID* pSrc, BLUE_VOID* pDst);
    BLUE_U32 convert_sq_to_2si(const BLUE_U32 Width, const BLUE_U32 Height, BLUE_VOID* pSrc, BLUE_VOID* pDst);

  private:
    std::shared_ptr<void> bvc_;
    std::shared_ptr<void> bvc_conv_;
};

spl::shared_ptr<bvc_wrapper> create_blue();
spl::shared_ptr<bvc_wrapper> create_blue(int device_index);

core::video_format_desc get_format_desc(bvc_wrapper& blue, EVideoModeExt vid_fmt, EMemoryFormat mem_fmt);

bool         is_epoch_card(bvc_wrapper& blue);
bool         is_kronos_card(bvc_wrapper& blue);
bool         is_epoch_neutron_1i2o_card(bvc_wrapper& blue);
bool         is_epoch_neutron_3o_card(bvc_wrapper& blue);
std::wstring get_card_desc(bvc_wrapper blue, int device_id);
std::wstring get_sdi_inputs(bvc_wrapper& blue);
std::wstring get_sdi_outputs(bvc_wrapper& blue);

EVideoModeExt             get_bluefish_video_format(core::video_format fmt);
static core::video_format get_caspar_video_format(EVideoModeExt fmt);

}} // namespace caspar::bluefish
