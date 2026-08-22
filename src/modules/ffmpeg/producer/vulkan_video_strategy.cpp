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
 * Author: Niklas Andersson, niklas@niklaspandersson.se
 */

#include "video_strategy.h"

#include "vulkan_frame_import.h"

#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>

#include <common/env.h>
#include <common/log.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavutil/frame.h>
#include <libavutil/hwcontext.h>
#include <libavutil/pixdesc.h>
}

#include <algorithm>
#include <utility>

namespace caspar { namespace ffmpeg {

namespace {

/// The codecs whose Vulkan decoder we let a producer use.
///
/// FFmpeg advertises a Vulkan hwaccel for two quite different families. These are the ones backed
/// by VK_KHR_video_decode_* — real fixed-function decode hardware — and their output is verified
/// bit-identical to the software decoder.
///
/// The other family (FFmpeg calls them SDR: ProRes, ProRes RAW, FFV1, DPX, APV) is implemented as
/// compute shaders. Those are deliberately NOT here: on FFmpeg master + NVIDIA they are broken
/// today — ProRes 422 decodes to visibly wrong pixels (~6 dB PSNR against FFmpeg's own software
/// decoder, reproducible with the ffmpeg CLI alone), and ProRes 4444 loses the Vulkan device
/// outright, which would take the whole server down with it since decode and render share one
/// device. Nothing about the integration differs for them, so re-testing this list is all that is
/// needed once upstream settles.
constexpr AVCodecID trusted_codecs[] = {AV_CODEC_ID_H264, AV_CODEC_ID_HEVC, AV_CODEC_ID_AV1};

/// The deinterlacer for hardware frames. Software bwdif cannot touch them, so an FFmpeg build
/// without this filter has no way to deinterlace on the GPU — see try_create_vulkan_video_strategy.
constexpr const char* deinterlacer = "bwdif_vulkan";

AVPixelFormat select_vulkan_format(AVCodecContext* ctx, const AVPixelFormat* formats)
{
    for (auto p = formats; *p != AV_PIX_FMT_NONE; ++p) {
        if (*p == AV_PIX_FMT_VULKAN)
            return *p;
    }
    // No Vulkan hwaccel for this stream after all. Let the decoder pick its usual software
    // format; the producer notices the missing frames context and swaps in the CPU strategy.
    return avcodec_default_get_format(ctx, formats);
}

} // namespace

// ---------------------------------------------------------------------------------------

/// Decode straight into GPU images on the accelerator's Vulkan device, deinterlace and retime them
/// with the Vulkan filters, and hand the mixer the result. Nothing crosses the PCIe bus in either
/// direction: the decoder writes into the very device the mixer renders with, so vulkan_frame_import
/// only has to move the pixels into the mixer's own textures.
class vulkan_video_strategy : public video_strategy
{
    const std::unique_ptr<vulkan_frame_import> import_;

  public:
    explicit vulkan_video_strategy(std::unique_ptr<vulkan_frame_import> import)
        : import_(std::move(import))
    {
    }

    std::wstring name() const override { return L"vulkan"; }

    bool open_decoder(AVCodecContext& ctx, const AVCodec& codec) override
    {
        if (std::find(std::begin(trusted_codecs), std::end(trusted_codecs), codec.id) ==
            std::end(trusted_codecs)) {
            return false;
        }

        if (!supports_vulkan(codec))
            return false;

        ctx.hw_device_ctx = av_buffer_ref(import_->device());
        if (!ctx.hw_device_ctx)
            return false;

        ctx.get_format = select_vulkan_format;
        return true;
    }

    AVBufferRef* hw_device_context() const override { return import_->device(); }

    bool accepts(const AVFrame& frame) const override
    {
        // A software frame means get_format() declined Vulkan after all — either the codec has no
        // Vulkan hwaccel for this stream, or the hwaccel failed to initialise and FFmpeg quietly
        // retried without it.
        if (frame.format != AV_PIX_FMT_VULKAN || !frame.hw_frames_ctx)
            return false;

        const auto* frames_ctx = reinterpret_cast<const AVHWFramesContext*>(frame.hw_frames_ctx->data);
        if (!vulkan_frame_import::has_mixer_layout(frames_ctx->sw_format, frames_ctx->width, frames_ctx->height)) {
            CASPAR_LOG(info) << L"[ffmpeg] The mixer has no layout for hardware frame format "
                             << u16(av_get_pix_fmt_name(frames_ctx->sw_format))
                             << L"; this file decodes on the CPU instead.";
            return false;
        }
        return true;
    }

    std::string deinterlace_filter(const std::string& deint) const override
    {
        // Hardware frames need a deinterlacer that runs on the same device; bwdif_vulkan takes
        // the same options as the software bwdif.
        return (boost::format("%s=mode=send_field:parity=auto:deint=%s") % deinterlacer % deint).str();
    }

    std::vector<AVPixelFormat> sink_formats() const override { return {AV_PIX_FMT_VULKAN}; }

    core::draw_frame make_frame(void*                            tag,
                                std::shared_ptr<AVFrame>         video,
                                std::shared_ptr<AVFrame>         audio,
                                core::color_space                color_space,
                                core::frame_geometry::scale_mode scale_mode) override
    {
        // Frames that never became hardware frames still have to be delivered: the audio-only
        // tail at end of file, and any stream whose decoder fell back to software.
        if (!video || video->format != AV_PIX_FMT_VULKAN || !video->hw_frames_ctx || !video->data[0]) {
            return import_->host_frame(tag, std::move(video), std::move(audio), color_space, scale_mode);
        }

        return import_->import(tag, video, audio, color_space, scale_mode);
    }

  private:
    static bool supports_vulkan(const AVCodec& codec)
    {
        for (int i = 0;; ++i) {
            const auto* config = avcodec_get_hw_config(&codec, i);
            if (!config)
                return false;
            if ((config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) &&
                config->device_type == AV_HWDEVICE_TYPE_VULKAN)
                return true;
        }
    }
};

std::shared_ptr<video_strategy>
try_create_vulkan_video_strategy(const std::shared_ptr<core::frame_factory>& frame_factory)
{
    // Deinterlacing has to be possible before the rest is worth setting up: a hardware frame can
    // only be deinterlaced by a filter that runs on the device, so an FFmpeg build without one is
    // a CPU-only build for any channel that might play interlaced material.
    const auto deint =
        env::properties().get<std::wstring>(L"configuration.ffmpeg.producer.auto-deinterlace", L"interlaced");
    if (deint != L"none" && !avfilter_get_by_name(deinterlacer)) {
        CASPAR_LOG(info) << L"[ffmpeg] This FFmpeg build has no " << u16(deinterlacer)
                         << L" filter, so hardware decoding cannot deinterlace; using the CPU path.";
        return nullptr;
    }

    auto import = vulkan_frame_import::create(frame_factory);
    if (!import)
        return nullptr;

    return std::make_shared<vulkan_video_strategy>(std::move(import));
}

}} // namespace caspar::ffmpeg
