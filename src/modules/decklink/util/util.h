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

#include <common/except.h>
#include <common/log.h>
#include <core/video_format.h>

#include "../decklink_api.h"

#include <string>

namespace caspar { namespace decklink {

static BMDDisplayMode get_decklink_video_format(core::video_format fmt)
{
    switch (fmt) {
        case core::video_format::pal:
            return bmdModePAL;
        case core::video_format::ntsc:
            return bmdModeNTSC;
        case core::video_format::x576p2500:
            return (BMDDisplayMode)ULONG_MAX;
        case core::video_format::x720p2398:
            return (BMDDisplayMode)ULONG_MAX;
        case core::video_format::x720p2400:
            return (BMDDisplayMode)ULONG_MAX;
        case core::video_format::x720p2500:
            return (BMDDisplayMode)ULONG_MAX;
        case core::video_format::x720p5000:
            return bmdModeHD720p50;
        case core::video_format::x720p2997:
            return (BMDDisplayMode)ULONG_MAX;
        case core::video_format::x720p5994:
            return bmdModeHD720p5994;
        case core::video_format::x720p3000:
            return (BMDDisplayMode)ULONG_MAX;
        case core::video_format::x720p6000:
            return bmdModeHD720p60;
        case core::video_format::x1080p2398:
            return bmdModeHD1080p2398;
        case core::video_format::x1080p2400:
            return bmdModeHD1080p24;
        case core::video_format::x1080i5000:
            return bmdModeHD1080i50;
        case core::video_format::x1080i5994:
            return bmdModeHD1080i5994;
        case core::video_format::x1080i6000:
            return bmdModeHD1080i6000;
        case core::video_format::x1080p2500:
            return bmdModeHD1080p25;
        case core::video_format::x1080p2997:
            return bmdModeHD1080p2997;
        case core::video_format::x1080p3000:
            return bmdModeHD1080p30;
        case core::video_format::x1080p5000:
            return bmdModeHD1080p50;
        case core::video_format::x1080p5994:
            return bmdModeHD1080p5994;
        case core::video_format::x1080p6000:
            return bmdModeHD1080p6000;
        case core::video_format::x1556p2398:
            return bmdMode2k2398;
        case core::video_format::x1556p2400:
            return bmdMode2k24;
        case core::video_format::x1556p2500:
            return bmdMode2k25;
        case core::video_format::x2160p2398:
            return bmdMode4K2160p2398;
        case core::video_format::x2160p2400:
            return bmdMode4K2160p24;
        case core::video_format::x2160p2500:
            return bmdMode4K2160p25;
        case core::video_format::x2160p2997:
            return bmdMode4K2160p2997;
        case core::video_format::x2160p3000:
            return bmdMode4K2160p30;
        case core::video_format::x2160p5000:
            return bmdMode4K2160p50;
        case core::video_format::x2160p5994:
            return bmdMode4K2160p5994;
        case core::video_format::x2160p6000:
            return bmdMode4K2160p60;
        default:
            return (BMDDisplayMode)ULONG_MAX;
    }
}

static core::video_format get_caspar_video_format(BMDDisplayMode fmt)
{
    switch (fmt) {
        case bmdModePAL:
            return core::video_format::pal;
        case bmdModeNTSC:
            return core::video_format::ntsc;
        case bmdModeHD720p50:
            return core::video_format::x720p5000;
        case bmdModeHD720p5994:
            return core::video_format::x720p5994;
        case bmdModeHD720p60:
            return core::video_format::x720p6000;
        case bmdModeHD1080p2398:
            return core::video_format::x1080p2398;
        case bmdModeHD1080p24:
            return core::video_format::x1080p2400;
        case bmdModeHD1080i50:
            return core::video_format::x1080i5000;
        case bmdModeHD1080i5994:
            return core::video_format::x1080i5994;
        case bmdModeHD1080i6000:
            return core::video_format::x1080i6000;
        case bmdModeHD1080p25:
            return core::video_format::x1080p2500;
        case bmdModeHD1080p2997:
            return core::video_format::x1080p2997;
        case bmdModeHD1080p30:
            return core::video_format::x1080p3000;
        case bmdModeHD1080p50:
            return core::video_format::x1080p5000;
        case bmdModeHD1080p5994:
            return core::video_format::x1080p5994;
        case bmdModeHD1080p6000:
            return core::video_format::x1080p6000;
        case bmdMode2k2398:
            return core::video_format::x1556p2398;
        case bmdMode2k24:
            return core::video_format::x1556p2400;
        case bmdMode2k25:
            return core::video_format::x1556p2500;
        case bmdMode4K2160p2398:
            return core::video_format::x2160p2398;
        case bmdMode4K2160p24:
            return core::video_format::x2160p2400;
        case bmdMode4K2160p25:
            return core::video_format::x2160p2500;
        case bmdMode4K2160p2997:
            return core::video_format::x2160p2997;
        case bmdMode4K2160p30:
            return core::video_format::x2160p3000;
        case bmdMode4K2160p50:
            return core::video_format::x2160p5000;
        case bmdMode4K2160p5994:
            return core::video_format::x2160p5994;
        case bmdMode4K2160p60:
            return core::video_format::x2160p6000;
        default:
            return core::video_format::invalid;
    }
}

static std::wstring get_mode_name(const com_ptr<IDeckLinkDisplayMode>& mode)
{
    String mode_name;
    if (SUCCEEDED(mode->GetName(&mode_name)))
        return to_string(mode_name);
    return L"Unknown";
}

template <typename T, typename F>
com_ptr<IDeckLinkDisplayMode> get_display_mode(const T& device, BMDDisplayMode format, BMDPixelFormat pix_fmt, F flag)
{
    IDeckLinkDisplayMode*         m = nullptr;
    IDeckLinkDisplayModeIterator* iter;
    if (SUCCEEDED(device->GetDisplayModeIterator(&iter))) {
        auto iterator = wrap_raw<com_ptr>(iter, true);
        while (SUCCEEDED(iterator->Next(&m)) && m != nullptr && m->GetDisplayMode() != format) {
            m->Release();
        }
    }

    if (!m)
        CASPAR_THROW_EXCEPTION(user_error()
                               << msg_info("Device could not find requested video-format: " + std::to_string(format)));

    com_ptr<IDeckLinkDisplayMode> mode = wrap_raw<com_ptr>(m, true);

    BMDDisplayModeSupport displayModeSupport;

    if (FAILED(device->DoesSupportVideoMode(mode->GetDisplayMode(), pix_fmt, flag, &displayModeSupport, nullptr)))
        CASPAR_THROW_EXCEPTION(caspar_exception()
                               << msg_info(L"Could not determine whether device supports requested video format: " +
                                           get_mode_name(mode)));
    else if (displayModeSupport == bmdDisplayModeNotSupported)
        CASPAR_LOG(info) << L"Device may not support video-format: " << get_mode_name(mode);
    else if (displayModeSupport == bmdDisplayModeSupportedWithConversion)
        CASPAR_LOG(warning) << L"Device supports video-format with conversion: " << get_mode_name(mode);

    return mode;
}

template <typename T, typename F>
static com_ptr<IDeckLinkDisplayMode>
get_display_mode(const T& device, core::video_format fmt, BMDPixelFormat pix_fmt, F flag)
{
    return get_display_mode(device, get_decklink_video_format(fmt), pix_fmt, flag);
}

template <typename T>
static std::wstring version(T iterator)
{
    auto info = iface_cast<IDeckLinkAPIInformation>(iterator);
    if (info) {
        String ver;
        if (SUCCEEDED(info->GetString(BMDDeckLinkAPIVersion, &ver)))
            return to_string(ver);
    }
    return L"Not found";
}

static com_ptr<IDeckLink> get_device(size_t device_index)
{
    auto pDecklinkIterator = create_iterator();

    size_t             n = 0;
    com_ptr<IDeckLink> decklink;
    IDeckLink*         current = nullptr;
    while (n < device_index && pDecklinkIterator->Next(&current) == S_OK) {
        ++n;
        decklink = wrap_raw<com_ptr>(current);
        current->Release();
    }

    if (n != device_index || !decklink)
        CASPAR_THROW_EXCEPTION(user_error()
                               << msg_info("Decklink device " + std::to_string(device_index) + " not found."));

    return decklink;
}

template <typename T>
static std::wstring get_model_name(const T& device)
{
    String pModelName;
    if (SUCCEEDED(device->GetModelName(&pModelName)))
        return to_string(pModelName);
    return L"Unknown";
}

class reference_signal_detector
{
    com_iface_ptr<IDeckLinkOutput> output_;
    BMDReferenceStatus             last_reference_status_ = static_cast<BMDReferenceStatus>(-1);

  public:
    reference_signal_detector(const com_iface_ptr<IDeckLinkOutput>& output)
        : output_(output)
    {
    }

    template <typename Print>
    void detect_change(const Print& print)
    {
        BMDReferenceStatus reference_status;

        if (output_->GetReferenceStatus(&reference_status) != S_OK) {
            CASPAR_LOG(error) << print() << L" Reference signal: failed while querying status";
        } else if (reference_status != last_reference_status_) {
            last_reference_status_ = reference_status;

            if (reference_status == 0)
                CASPAR_LOG(info) << print() << L" Reference signal: not detected.";
            else if (reference_status & bmdReferenceNotSupportedByHardware)
                CASPAR_LOG(info) << print() << L" Reference signal: not supported by hardware.";
            else if (reference_status & bmdReferenceLocked)
                CASPAR_LOG(info) << print() << L" Reference signal: locked.";
            else
                CASPAR_LOG(info) << print() << L" Reference signal: Unhandled enum bitfield: " << reference_status;
        }
    }
};

}} // namespace caspar::decklink
