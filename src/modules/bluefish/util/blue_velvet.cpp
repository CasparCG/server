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

#include "blue_velvet.h"
#include "../StdAfx.h"
#include <core/video_format.h>

#define IMPLEMENTATION_BLUEVELVETC_FUNC_PTR
#define IMPLEMENTATION_BLUEVELVETC_CONVERSION_FUNC_PTR
#include "../interop/BlueVelvetCFuncPtr.h"

namespace caspar { namespace bluefish {

bvc_wrapper::bvc_wrapper()
{
    if (LoadFunctionPointers_BlueVelvetC())
        bvc_ = std::shared_ptr<void>(bfcFactory(), bfcDestroy);

    if (LoadFunctionPointers_BlueConversion())
        bvc_conv_ = std::shared_ptr<void>(bfcConversionFactory(), bfcConversionDestroy);

    if (!bvc_ || !bvc_conv_)
        CASPAR_THROW_EXCEPTION(not_supported() << msg_info(
                                   "Bluefish drivers not found.\nDriver not installed?\nMinimum Version is V6.3.0.2"));
}

const char* bvc_wrapper::get_version() { return bfcGetVersion(); }

BLUE_U32 bvc_wrapper::attach(int iDeviceId) { return bfcAttach(bvc_.get(), iDeviceId); }

BLUE_U32 bvc_wrapper::detach() { return bfcDetach(bvc_.get()); }

BLUE_U32 bvc_wrapper::set_multilink(const int iDeviceID, const int memChannel)
{
    if (memChannel == -1)
        return (BLUE_U32)bfcSetMultiLinkMode(bvc_.get(), NULL); // ie. detach from multiLink

    blue_multi_link_info_struct attach_info = {};
    attach_info.InputControl                = 1;
    attach_info.Link1_Device                = iDeviceID;
    attach_info.Link2_Device                = iDeviceID;
    attach_info.Link3_Device                = iDeviceID;
    attach_info.Link4_Device                = iDeviceID;

    if (memChannel == BLUE_VIDEO_INPUT_CHANNEL_1) {
        attach_info.Link1_MemChannel = BLUE_VIDEO_INPUT_CHANNEL_1;
        attach_info.Link2_MemChannel = BLUE_VIDEO_INPUT_CHANNEL_2;
        attach_info.Link3_MemChannel = BLUE_VIDEO_INPUT_CHANNEL_3;
        attach_info.Link4_MemChannel = BLUE_VIDEO_INPUT_CHANNEL_4;
    } else if (memChannel == BLUE_VIDEO_INPUT_CHANNEL_5) {
        attach_info.Link5_MemChannel = BLUE_VIDEO_INPUT_CHANNEL_5;
        attach_info.Link6_MemChannel = BLUE_VIDEO_INPUT_CHANNEL_6;
        attach_info.Link7_MemChannel = BLUE_VIDEO_INPUT_CHANNEL_7;
        attach_info.Link8_MemChannel = BLUE_VIDEO_INPUT_CHANNEL_8;
    } else if (memChannel == BLUE_VIDEO_OUTPUT_CHANNEL_1) {
        attach_info.Link1_MemChannel = BLUE_VIDEO_OUTPUT_CHANNEL_1;
        attach_info.Link2_MemChannel = BLUE_VIDEO_OUTPUT_CHANNEL_2;
        attach_info.Link3_MemChannel = BLUE_VIDEO_OUTPUT_CHANNEL_3;
        attach_info.Link4_MemChannel = BLUE_VIDEO_OUTPUT_CHANNEL_4;
    } else if (memChannel == BLUE_VIDEO_OUTPUT_CHANNEL_5) {
        attach_info.Link5_MemChannel = BLUE_VIDEO_OUTPUT_CHANNEL_5;
        attach_info.Link6_MemChannel = BLUE_VIDEO_OUTPUT_CHANNEL_6;
        attach_info.Link7_MemChannel = BLUE_VIDEO_OUTPUT_CHANNEL_7;
        attach_info.Link8_MemChannel = BLUE_VIDEO_OUTPUT_CHANNEL_8;
    }

    return (BLUE_U32)bfcSetMultiLinkMode(bvc_.get(), &attach_info);
}

BLUE_U32 bvc_wrapper::get_card_property32(const int iProperty, unsigned int* nValue)
{
    return (BLUE_U32)bfcQueryCardProperty32(bvc_.get(), iProperty, nValue);
}

BLUE_U32 bvc_wrapper::set_card_property32(const int iProperty, const unsigned int nValue)
{
    return bfcSetCardProperty32(bvc_.get(), iProperty, nValue);
}

BLUE_U32 bvc_wrapper::get_card_property64(const int iProperty, unsigned long long* nValue)
{
    return (BLUE_U32)bfcQueryCardProperty64(bvc_.get(), iProperty, nValue);
}

BLUE_U32 bvc_wrapper::set_card_property64(const int iProperty, const unsigned long long nValue)
{
    return bfcSetCardProperty64(bvc_.get(), iProperty, nValue);
}

BLUE_U32 bvc_wrapper::enumerate(int* iDevices) { return bfcEnumerate(bvc_.get(), iDevices); }

BLUE_U32 bvc_wrapper::query_card_type(int* iCardType, int iDeviceID)
{
    return bfcQueryCardType(bvc_.get(), iCardType, iDeviceID);
}

BLUE_U32 bvc_wrapper::system_buffer_write(unsigned char* pPixels,
                                          unsigned long  ulSize,
                                          unsigned long  ulBufferID,
                                          unsigned long  ulOffset)
{
    return bfcSystemBufferWriteAsync(bvc_.get(), pPixels, ulSize, nullptr, ulBufferID, ulOffset);
}

BLUE_U32 bvc_wrapper::system_buffer_read(unsigned char* pPixels,
                                         unsigned long  ulSize,
                                         unsigned long  ulBufferID,
                                         unsigned long  ulOffset)
{
#if 1
    return bfcSystemBufferReadAsync(bvc_.get(), pPixels, ulSize, nullptr, ulBufferID, ulOffset);
#else
    BFC_SYNC_INFO bsi    = bfcSyncInfoCreate(bvc_.get());
    BLUE_U32      retVal = 0;
    retVal               = bfcDmaReadFromCardAsync(bvc_.get(), pPixels, ulSize, bsi, ulBufferID, ulOffset);
    int wrv              = bfcSyncInfoWait(bvc_.get(), bsi, 20);
    int x                = wrv + 0;
    x++;
    bfcSyncInfoDelete(bvc_.get(), bsi);
    return retVal;
#endif
}

BLUE_U32 bvc_wrapper::video_playback_stop(int iWait, int iFlush)
{
    return bfcVideoPlaybackStop(bvc_.get(), iWait, iFlush);
}

BLUE_U32 bvc_wrapper::video_playback_start(int step, int loop) { return bfcVideoPlaybackStart(bvc_.get(), step, loop); }

BLUE_U32 bvc_wrapper::video_playback_allocate(unsigned long* buffer_id, unsigned long* underrun)
{
    void* unused = nullptr;
    return bfcVideoPlaybackAllocate(bvc_.get(), &unused, buffer_id, underrun);
}

BLUE_U32
bvc_wrapper::video_playback_present(unsigned long buffer_id, unsigned long count, unsigned long keep, unsigned long odd)
{
    unsigned long unique_id;
    return bfcVideoPlaybackPresent(bvc_.get(), &unique_id, buffer_id, count, keep, odd);
}

BLUE_U32 bvc_wrapper::wait_video_output_sync(unsigned long ulUpdateType, unsigned long* ulFieldCount)
{
    return bfcWaitVideoOutputSync(bvc_.get(), ulUpdateType, ulFieldCount);
}

BLUE_U32 bvc_wrapper::wait_video_input_sync(unsigned long ulUpdateType, unsigned long* ulFieldCount)
{
    return bfcWaitVideoInputSync(bvc_.get(), ulUpdateType, ulFieldCount);
}

BLUE_U32 bvc_wrapper::render_buffer_update(unsigned long ulBufferID)
{
    return bfcRenderBufferUpdate(bvc_.get(), ulBufferID);
}

BLUE_U32 bvc_wrapper::render_buffer_capture(unsigned long ulBufferID)
{
    return bfcRenderBufferCapture(bvc_.get(), ulBufferID);
}

BLUE_U32 bvc_wrapper::encode_hanc_frame(unsigned int             nCardType,
                                        hanc_stream_info_struct* pHancEncodeInfo,
                                        void*                    pAudioBuffer,
                                        unsigned int             nAudioChannels,
                                        unsigned int             nAudioSamples,
                                        unsigned int             nSampleType,
                                        unsigned int             nAudioFlags)
{
    return bfcEncodeHancFrameEx(bvc_.get(),
                                CRD_BLUE_NEUTRON,
                                pHancEncodeInfo,
                                pAudioBuffer,
                                nAudioChannels,
                                nAudioSamples,
                                nSampleType,
                                nAudioFlags);
}

BLUE_U32
bvc_wrapper::decode_hanc_frame(unsigned int nCardType, unsigned int* pHancBuffer, hanc_decode_struct* pHancDecodeInfo)
{
    return bfcDecodeHancFrameEx(bvc_.get(), CRD_BLUE_NEUTRON, pHancBuffer, pHancDecodeInfo);
}

BLUE_U32 bvc_wrapper::get_frame_info_for_video_mode(const unsigned int nVideoModeExt,
                                                    unsigned int*      nWidth,
                                                    unsigned int*      nHeight,
                                                    unsigned int*      nRate,
                                                    unsigned int*      bIs1001,
                                                    unsigned int*      bIsProgressive)
{
    return bfcUtilsGetFrameInfoForVideoModeExt(nVideoModeExt, nWidth, nHeight, nRate, bIs1001, bIsProgressive);
}

BLUE_U32 bvc_wrapper::get_bytes_per_frame(EVideoModeExt nVideoModeExt,
                                          EMemoryFormat nMemoryFormat,
                                          EUpdateMethod nUpdateMethod,
                                          unsigned int* nBytesPerFrame)
{
    return bfcGetVideoBytesPerFrame(nVideoModeExt, nUpdateMethod, nMemoryFormat, nBytesPerFrame);
}

std::string bvc_wrapper::get_string_for_card_type(unsigned int nCardType)
{
    return bfcUtilsGetStringForCardType(nCardType);
}

std::wstring bvc_wrapper::get_wstring_for_video_mode(unsigned int nVideoModeExt)
{
    std::wstring mode_desc;
    switch (nVideoModeExt) {
        case VID_FMT_EXT_PAL:
            mode_desc = L"pal";
            break;
        case VID_FMT_EXT_NTSC:
            mode_desc = L"ntsc";
            break;
        case VID_FMT_EXT_720P_2398:
            mode_desc = L"720p23";
            break;
        case VID_FMT_EXT_720P_2400:
            mode_desc = L"720p24";
            break;
        case VID_FMT_EXT_720P_2500:
            mode_desc = L"720p25";
            break;
        case VID_FMT_EXT_720P_5000:
            mode_desc = L"720p50";
            break;
        case VID_FMT_EXT_720P_2997:
            mode_desc = L"720p29";
            break;
        case VID_FMT_EXT_720P_5994:
            mode_desc = L"720p59";
            break;
        case VID_FMT_EXT_720P_3000:
            mode_desc = L"720p30";
            break;
        case VID_FMT_EXT_720P_6000:
            mode_desc = L"720p60";
            break;
        case VID_FMT_EXT_1080P_2398:
            mode_desc = L"1080p23";
            break;
        case VID_FMT_EXT_1080P_2400:
            mode_desc = L"1080p24";
            break;
        case VID_FMT_EXT_1080I_5000:
            mode_desc = L"1080i50";
            break;
        case VID_FMT_1080I_5994:
            mode_desc = L"1080i59";
            break;
        case VID_FMT_EXT_1080I_6000:
            mode_desc = L"1080i60";
            break;
        case VID_FMT_EXT_1080P_2500:
            mode_desc = L"1080p25";
            break;
        case VID_FMT_EXT_1080P_2997:
            mode_desc = L"1080p29";
            break;
        case VID_FMT_EXT_1080P_3000:
            mode_desc = L"1080p30";
            break;
        case VID_FMT_EXT_1080P_5000:
            mode_desc = L"1080p50";
            break;
        case VID_FMT_EXT_1080P_5994:
            mode_desc = L"1080p59";
            break;
        case VID_FMT_EXT_1080P_6000:
            mode_desc = L"1080p60";
            break;
        case VID_FMT_EXT_2160P_2398:
            mode_desc = L"2160p23";
            break;
        case VID_FMT_EXT_2160P_2400:
            mode_desc = L"2160p24";
            break;
        case VID_FMT_EXT_2160P_2500:
            mode_desc = L"2160p25";
            break;
        case VID_FMT_EXT_2160P_2997:
            mode_desc = L"2160p29";
            break;
        case VID_FMT_EXT_2160P_3000:
            mode_desc = L"2160p30";
            break;
        case VID_FMT_EXT_2160P_5000:
            mode_desc = L"2160p50";
            break;
        case VID_FMT_EXT_2160P_5994:
            mode_desc = L"2160p59";
            break;
        case VID_FMT_EXT_2160P_6000:
            mode_desc = L"2160p60";
            break;
        default:
            mode_desc = L"invalid";
            break;
    }
    return mode_desc;
}

int bvc_wrapper::get_num_audio_samples_for_frame(const BLUE_U32 nVideoModeExt, const BLUE_U32 nFrameNo)
{
    return bfcUtilsGetAudioSamplesPerFrame(nVideoModeExt, nFrameNo);
}

BLUE_U32 bvc_wrapper::convert_2si_to_sq(const BLUE_U32 Width, const BLUE_U32 Height, BLUE_VOID* pSrc, BLUE_VOID* pDst)
{
    return bfcConvert_TsiToSquareDivision_RGB(bvc_conv_.get(), Width, Height, pSrc, pDst);
}

BLUE_U32 bvc_wrapper::convert_sq_to_2si(const BLUE_U32 Width, const BLUE_U32 Height, BLUE_VOID* pSrc, BLUE_VOID* pDst)
{
    return bfcConvert_SquareDivisionToTsi_ARGB32(bvc_conv_.get(), Width, Height, pSrc, pDst);
}

EVideoModeExt vid_fmt_from_video_format(const core::video_format& fmt)
{
    switch (fmt) {
        case core::video_format::pal:
            return VID_FMT_EXT_PAL;
        case core::video_format::ntsc:
            return VID_FMT_EXT_NTSC;
        case core::video_format::x576p2500:
            return VID_FMT_EXT_INVALID; // not supported
        case core::video_format::x720p2398:
            return VID_FMT_EXT_720P_2398;
        case core::video_format::x720p2400:
            return VID_FMT_EXT_720P_2400;
        case core::video_format::x720p2500:
            return VID_FMT_EXT_720P_2500;
        case core::video_format::x720p5000:
            return VID_FMT_EXT_720P_5000;
        case core::video_format::x720p2997:
            return VID_FMT_EXT_720P_2997;
        case core::video_format::x720p5994:
            return VID_FMT_EXT_720P_5994;
        case core::video_format::x720p3000:
            return VID_FMT_EXT_720P_3000;
        case core::video_format::x720p6000:
            return VID_FMT_EXT_720P_6000;
        case core::video_format::x1080p2398:
            return VID_FMT_EXT_1080P_2398;
        case core::video_format::x1080p2400:
            return VID_FMT_EXT_1080P_2400;
        case core::video_format::x1080i5000:
            return VID_FMT_EXT_1080I_5000;
        case core::video_format::x1080i5994:
            return VID_FMT_EXT_1080I_5994;
        case core::video_format::x1080i6000:
            return VID_FMT_EXT_1080I_6000;
        case core::video_format::x1080p2500:
            return VID_FMT_EXT_1080P_2500;
        case core::video_format::x1080p2997:
            return VID_FMT_EXT_1080P_2997;
        case core::video_format::x1080p3000:
            return VID_FMT_EXT_1080P_3000;
        case core::video_format::x1080p5000:
            return VID_FMT_EXT_1080P_5000;
        case core::video_format::x1080p5994:
            return VID_FMT_EXT_1080P_5994;
        case core::video_format::x1080p6000:
            return VID_FMT_EXT_1080P_6000;
        case core::video_format::x2160p2398:
            return VID_FMT_EXT_2160P_2398;
        case core::video_format::x2160p2400:
            return VID_FMT_EXT_2160P_2400;
        case core::video_format::x2160p2500:
            return VID_FMT_EXT_2160P_2500;
        case core::video_format::x2160p2997:
            return VID_FMT_EXT_2160P_2997;
        case core::video_format::x2160p3000:
            return VID_FMT_EXT_2160P_3000;
        case core::video_format::x2160p5000:
            return VID_FMT_EXT_2160P_5000;
        case core::video_format::x2160p5994:
            return VID_FMT_EXT_2160P_5994;
        case core::video_format::x2160p6000:
            return VID_FMT_EXT_2160P_6000;

        default:
            return VID_FMT_EXT_INVALID;
    }
}

bool is_epoch_card(bvc_wrapper& blue)
{
    int device_id = 1;
    int card_type = 0;
    blue.query_card_type(&card_type, device_id);

    switch (card_type) {
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
        case CRD_BLUE_SUPER_NOVA_S_PLUS:
        case CRD_BLUE_NEUTRON:
        case CRD_BLUE_EPOCH_CG:
            return true;
        default:
            return false;
    }
}

bool is_kronos_card(bvc_wrapper& blue)
{
    int device_id = 1;
    int card_type = 0;
    blue.query_card_type(&card_type, device_id);

    switch (card_type) {
        case CRD_BLUE_KRONOS_ELEKTRON:
        case CRD_BLUE_KRONOS_OPTIKOS:
        case CRD_BLUE_KRONOS_K8:
            return true;
        default:
            return false;
    }
}

bool is_epoch_neutron_1i2o_card(bvc_wrapper& blue)
{
    BLUE_U32 val = 0;
    blue.get_card_property32(EPOCH_GET_PRODUCT_ID, &val);
    if (val == ORAC_NEUTRON_1_IN_2_OUT_FIRMWARE_PRODUCTID)
        return true;
    return false;
}

bool is_epoch_neutron_3o_card(bvc_wrapper& blue)
{
    BLUE_U32 val = 0;
    blue.get_card_property32(EPOCH_GET_PRODUCT_ID, &val);

    if (val == ORAC_NEUTRON_0_IN_3_OUT_FIRMWARE_PRODUCTID)
        return true;
    return false;
}

std::wstring get_card_desc(bvc_wrapper blue, int device_id)
{
    std::wstring card_desc;
    int          card_type = 0;
    blue.query_card_type(&card_type, device_id);

    switch (card_type) {
        case CRD_BLUE_EPOCH_2K_CORE:
            card_desc = L"Bluefish Epoch 2K Core";
            break;
        case CRD_BLUE_EPOCH_2K_ULTRA:
            card_desc = L"Bluefish Epoch 2K Ultra";
            break;
        case CRD_BLUE_EPOCH_HORIZON:
            card_desc = L"Bluefish Epoch Horizon";
            break;
        case CRD_BLUE_EPOCH_CORE:
            card_desc = L"Blufishe Epoch Core";
            break;
        case CRD_BLUE_EPOCH_ULTRA:
            card_desc = L"Bluefish Epoch Ultra";
            break;
        case CRD_BLUE_CREATE_HD:
            card_desc = L"Bluefish Create HD";
            break;
        case CRD_BLUE_CREATE_2K:
            card_desc = L"Bluefish Create 2K";
            break;
        case CRD_BLUE_CREATE_2K_ULTRA:
            card_desc = L"Bluefish Create 2K Ultra";
            break;
        case CRD_BLUE_SUPER_NOVA:
            card_desc = L"Bluefish SuperNova";
            break;
        case CRD_BLUE_SUPER_NOVA_S_PLUS:
            card_desc = L"Bluefish SuperNova s+";
            break;
        case CRD_BLUE_NEUTRON:
            card_desc = L"Bluefish Neutron 4k";
            break;
        case CRD_BLUE_EPOCH_CG:
            card_desc = L"Bluefish Epoch CG";
            break;
        case CRD_BLUE_KRONOS_ELEKTRON:
            card_desc = L"Bluefish Kronos Elektron";
            break;
        case CRD_BLUE_KRONOS_OPTIKOS:
            card_desc = L"Bluefish Kronos Optikos";
            break;
        case CRD_BLUE_KRONOS_K8:
            card_desc = L"Bluefish Kronos K8";
            break;
        default:
            card_desc = L"Unknown";
    }
    return card_desc;
}

EVideoModeExt get_video_mode(bvc_wrapper& blue, const core::video_format_desc& format_desc)
{
    EVideoModeExt vid_fmt_ext = VID_FMT_EXT_INVALID;

    if ((format_desc.width <= 2048) || is_kronos_card(blue))
        vid_fmt_ext = vid_fmt_from_video_format(format_desc.format);

    if (vid_fmt_ext == VID_FMT_EXT_INVALID)
        CASPAR_THROW_EXCEPTION(not_supported() << msg_info(L"video-mode not supported: " + format_desc.name));

    return vid_fmt_ext;
}

spl::shared_ptr<bvc_wrapper> create_blue()
{
    auto pWrap = new bvc_wrapper();
    return spl::shared_ptr<bvc_wrapper>(pWrap);
}

spl::shared_ptr<bvc_wrapper> create_blue(int device_index)
{
    auto blue = create_blue();

    if (blue->attach(device_index))
        CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to attach device."));

    return blue;
}

core::video_format_desc get_format_desc(bvc_wrapper& blue, EVideoModeExt vid_fmt_ext, EMemoryFormat mem_fmt)
{
    core::video_format_desc fmt;
    unsigned int     width, height, duration = 0, time_scale = 0, rate = 0, bIs1001 = 0, is_progressive = 0, size = 0;
    std::vector<int> audio_cadence;
    int              field_count = 1;

    blue.get_frame_info_for_video_mode(vid_fmt_ext, &width, &height, &rate, &bIs1001, &is_progressive);
    CASPAR_LOG(warning) << L"vfe is " << vid_fmt_ext << L" w: " << width << L" h:" << height << L" rate:" << rate
                        << L"is1001 " << bIs1001 << L"isProgressive " << is_progressive;
    blue.get_bytes_per_frame(vid_fmt_ext, mem_fmt, UPD_FMT_FRAME, &size);

    CASPAR_LOG(warning) << L"vfe is " << vid_fmt_ext << L" size is: " << size;

    switch (vid_fmt_ext) {
        case VID_FMT_EXT_NTSC:
        case VID_FMT_EXT_1080I_5994:
            duration      = 30000;
            time_scale    = 1001;
            audio_cadence = {1601, 1602, 1601, 1602, 1602};
            break;
        case VID_FMT_EXT_2K_1080P_2500:
        case VID_FMT_EXT_2K_1080PSF_2500:
        case VID_FMT_EXT_576I_5000:
        case VID_FMT_EXT_1080P_2500:
        case VID_FMT_EXT_1080I_5000:
        case VID_FMT_EXT_1080PSF_2500:
        case VID_FMT_EXT_720P_2500:
        case VID_FMT_EXT_2160P_2500:
            duration      = 25000;
            time_scale    = 1000;
            audio_cadence = {1920, 1920, 1920, 1920, 1920};
            break;

        case VID_FMT_EXT_720P_5994:
        case VID_FMT_EXT_2K_1080P_5994:
        case VID_FMT_EXT_1080P_5994:
        case VID_FMT_EXT_2160P_5994:
            duration      = 60000;
            time_scale    = 1001;
            audio_cadence = {801, 800, 801, 800, 800};
            break;

        case VID_FMT_EXT_1080P_6000:
        case VID_FMT_EXT_2K_1080P_6000:
        case VID_FMT_EXT_720P_6000:
        case VID_FMT_EXT_2160P_6000:
            duration      = 60000;
            time_scale    = 1000;
            audio_cadence = {801, 800, 801, 800, 800};
            break;

        case VID_FMT_EXT_1080PSF_2398:
        case VID_FMT_EXT_1080P_2398:
        case VID_FMT_EXT_720P_2398:
        case VID_FMT_EXT_2K_1080PSF_2398:
        case VID_FMT_EXT_2K_1080P_2398:
        case VID_FMT_EXT_2160P_2398:
            duration      = 24000;
            time_scale    = 1000;
            audio_cadence = {2002, 2002, 2002, 2002, 2002};
            break;

        case VID_FMT_EXT_1080PSF_2400:
        case VID_FMT_EXT_1080P_2400:
        case VID_FMT_EXT_720P_2400:
        case VID_FMT_EXT_2K_1080PSF_2400:
        case VID_FMT_EXT_2K_1080P_2400:
        case VID_FMT_EXT_2160P_2400:
            duration      = 24000;
            time_scale    = 1000;
            audio_cadence = {2000, 2000, 2000, 2000, 2000};
            break;

        case VID_FMT_EXT_1080I_6000:
        case VID_FMT_EXT_1080PSF_3000:
            duration      = 30000;
            time_scale    = 1000;
            audio_cadence = {1600, 1600, 1600, 1600, 1600};
            break;

        case VID_FMT_EXT_720P_2997:
        case VID_FMT_EXT_1080P_2997:
        case VID_FMT_EXT_2K_1080PSF_2997:
        case VID_FMT_EXT_2K_1080P_2997:
        case VID_FMT_EXT_1080PSF_2997:
        case VID_FMT_EXT_2160P_2997:
            duration      = 30000;
            time_scale    = 1001;
            audio_cadence = {1602, 1601, 1602, 1601, 1602};
            break;

        case VID_FMT_EXT_720P_3000:
        case VID_FMT_EXT_1080P_3000:
        case VID_FMT_EXT_2K_1080PSF_3000:
        case VID_FMT_EXT_2K_1080P_3000:
        case VID_FMT_EXT_2160P_3000:
            duration      = 30000;
            time_scale    = 1001;
            audio_cadence = {1600, 1600, 1600, 1600, 1600};
            break;

        case VID_FMT_EXT_720P_5000:
        case VID_FMT_EXT_1080P_5000:
        case VID_FMT_EXT_2K_1080P_5000:
        case VID_FMT_EXT_2160P_5000:
            audio_cadence = {960, 960, 960, 960, 960};
            duration      = 50000;
            time_scale    = 1000;
            break;
    }

    if (is_progressive == 0u)
        field_count = 2;

    fmt.field_count   = field_count;
    fmt               = get_caspar_video_format(vid_fmt_ext);
    fmt.size          = size;
    fmt.audio_cadence = std::move(audio_cadence);
    fmt.name          = blue.get_wstring_for_video_mode(vid_fmt_ext);

    return fmt;
}

std::wstring get_sdi_inputs(bvc_wrapper& blue)
{
    BLUE_U32 val = 0;
    blue.get_card_property32(CARD_FEATURE_CONNECTOR_INFO, &val);
    int connectors = CARD_FEATURE_GET_SDI_INPUT_CONNECTOR_COUNT(val);
    return std::to_wstring(connectors);
}

std::wstring get_sdi_outputs(bvc_wrapper& blue)
{
    BLUE_U32 val = 0;
    blue.get_card_property32(CARD_FEATURE_CONNECTOR_INFO, &val);
    int connectors = CARD_FEATURE_GET_SDI_OUTPUT_CONNECTOR_COUNT(val);
    return std::to_wstring(connectors);
}

EVideoModeExt get_bluefish_video_format(core::video_format fmt)
{
    // TODO: add supoprt for UHD 4K formats

    switch (fmt) {
        case core::video_format::pal:
            return VID_FMT_EXT_PAL;
        case core::video_format::ntsc:
            return VID_FMT_EXT_NTSC;
        case core::video_format::x720p2398:
            return VID_FMT_EXT_720P_2398;
        case core::video_format::x720p2400:
            return VID_FMT_EXT_720P_2400;
        case core::video_format::x720p2500:
            return VID_FMT_EXT_720P_2500;
        case core::video_format::x720p2997:
            return VID_FMT_EXT_720P_2997;
        case core::video_format::x720p3000:
            return VID_FMT_EXT_720P_3000;
        case core::video_format::x720p5000:
            return VID_FMT_EXT_720P_5000;
        case core::video_format::x720p5994:
            return VID_FMT_EXT_720P_5994;
        case core::video_format::x720p6000:
            return VID_FMT_EXT_720P_6000;
        case core::video_format::x1080i5000:
            return VID_FMT_EXT_1080I_5000;
        case core::video_format::x1080i5994:
            return VID_FMT_EXT_1080I_5994;
        case core::video_format::x1080i6000:
            return VID_FMT_EXT_1080I_6000;
        case core::video_format::x1080p2398:
            return VID_FMT_EXT_1080P_2398;
        case core::video_format::x1080p2400:
            return VID_FMT_EXT_1080P_2400;
        case core::video_format::x1080p2500:
            return VID_FMT_EXT_1080P_2500;
        case core::video_format::x1080p2997:
            return VID_FMT_EXT_1080P_2997;
        case core::video_format::x1080p3000:
            return VID_FMT_EXT_1080P_3000;
        case core::video_format::x1080p5000:
            return VID_FMT_EXT_1080P_5000;
        case core::video_format::x1080p5994:
            return VID_FMT_EXT_1080P_5994;
        case core::video_format::x1080p6000:
            return VID_FMT_EXT_1080P_6000;
        case core::video_format::x2160p2398:
            return VID_FMT_EXT_2160P_2398;
        case core::video_format::x2160p2400:
            return VID_FMT_EXT_2160P_2400;
        case core::video_format::x2160p2500:
            return VID_FMT_EXT_2160P_2500;
        case core::video_format::x2160p2997:
            return VID_FMT_EXT_2160P_2997;
        case core::video_format::x2160p3000:
            return VID_FMT_EXT_2160P_3000;
        case core::video_format::x2160p5000:
            return VID_FMT_EXT_2160P_5000;
        case core::video_format::x2160p5994:
            return VID_FMT_EXT_2160P_5994;
        case core::video_format::x2160p6000:
            return VID_FMT_EXT_2160P_6000;
        default:
            return VID_FMT_EXT_INVALID;
    }
}

static core::video_format get_caspar_video_format(EVideoModeExt fmt)
{
    switch (fmt) {
        case VID_FMT_EXT_PAL:
            return core::video_format::pal;
        case VID_FMT_EXT_NTSC:
            return core::video_format::ntsc;
        case VID_FMT_EXT_720P_2398:
            return core::video_format::x720p2398;
        case VID_FMT_EXT_720P_2400:
            return core::video_format::x720p2400;
        case VID_FMT_EXT_720P_2500:
            return core::video_format::x720p2500;
        case VID_FMT_EXT_720P_5000:
            return core::video_format::x720p5000;
        case VID_FMT_EXT_720P_2997:
            return core::video_format::x720p2997;
        case VID_FMT_EXT_720P_5994:
            return core::video_format::x720p5994;
        case VID_FMT_EXT_720P_3000:
            return core::video_format::x720p3000;
        case VID_FMT_EXT_720P_6000:
            return core::video_format::x720p6000;
        case VID_FMT_EXT_1080P_2398:
        case VID_FMT_EXT_1080PSF_2398:
            return core::video_format::x1080p2398;
        case VID_FMT_EXT_1080P_2400:
        case VID_FMT_EXT_1080PSF_2400:
            return core::video_format::x1080p2400;
        case VID_FMT_EXT_1080I_5000:
            return core::video_format::x1080i5000;
        case VID_FMT_EXT_1080I_5994:
            return core::video_format::x1080i5994;
        case VID_FMT_EXT_1080I_6000:
            return core::video_format::x1080i6000;
        case VID_FMT_EXT_1080P_2500:
        case VID_FMT_EXT_1080PSF_2500:
            return core::video_format::x1080p2500;
        case VID_FMT_EXT_1080P_2997:
        case VID_FMT_EXT_1080PSF_2997:
            return core::video_format::x1080p2997;
        case VID_FMT_EXT_1080P_3000:
        case VID_FMT_EXT_1080PSF_3000:
            return core::video_format::x1080p3000;
        case VID_FMT_EXT_1080P_5000:
            return core::video_format::x1080p5000;
        case VID_FMT_EXT_1080P_5994:
            return core::video_format::x1080p5994;
        case VID_FMT_EXT_1080P_6000:
            return core::video_format::x1080p6000;
        case VID_FMT_EXT_2160P_2398:
            return core::video_format::x2160p2398;
        case VID_FMT_EXT_2160P_2400:
            return core::video_format::x2160p2400;
        case VID_FMT_EXT_2160P_2500:
            return core::video_format::x2160p2500;
        case VID_FMT_EXT_2160P_2997:
            return core::video_format::x2160p2997;
        case VID_FMT_EXT_2160P_3000:
            return core::video_format::x2160p3000;
        case VID_FMT_EXT_2160P_5000:
            return core::video_format::x2160p5000;
        case VID_FMT_EXT_2160P_5994:
            return core::video_format::x2160p5994;
        case VID_FMT_EXT_2160P_6000:
            return core::video_format::x2160p6000;
        default:
            return core::video_format::invalid;
    }
}

}} // namespace caspar::bluefish
