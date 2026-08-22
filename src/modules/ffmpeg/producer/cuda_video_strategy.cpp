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

#include "../util/av_assert.h"
#include "../util/av_util.h"

#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>

#include <common/env.h>
#include <common/log.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vulkan.h>
#include <libavutil/error.h>
#include <libavutil/pixdesc.h>
}

#include <memory>
#include <string>
#include <utility>

namespace caspar { namespace ffmpeg {

namespace {

/// NVDEC's deinterlacer. Same options as the software bwdif, like the Vulkan one.
constexpr const char* deinterlacer = "bwdif_cuda";

using buffer_ref = std::shared_ptr<AVBufferRef>;

buffer_ref wrap_buffer_ref(AVBufferRef* ref)
{
    return buffer_ref(ref, [](AVBufferRef* p) { av_buffer_unref(&p); });
}

/// AVERROR code as text, for the log lines below. The FF()/FF_RET() macros throw, which is not
/// what any of these call sites want — they report and fall back.
std::wstring error_text(int ret)
{
    char buf[AV_ERROR_MAX_STRING_SIZE] = {};
    av_strerror(ret, buf, sizeof(buf));
    return u16(std::string(buf));
}

AVPixelFormat select_cuda_format(AVCodecContext* ctx, const AVPixelFormat* formats)
{
    for (auto p = formats; *p != AV_PIX_FMT_NONE; ++p) {
        if (*p == AV_PIX_FMT_CUDA)
            return *p;
    }
    // No NVDEC hwaccel for this stream after all — let the decoder pick its usual software
    // format; the producer notices the missing frames context and moves on.
    return avcodec_default_get_format(ctx, formats);
}

} // namespace

// ---------------------------------------------------------------------------------------

/// Decode with NVDEC, then hand the frame to Vulkan.
///
/// This exists for the codecs Vulkan video decode cannot do at all — MPEG-2 above all, plus VC-1,
/// MPEG-4, VP8, VP9 and MJPEG — so it is only ever reached after the Vulkan strategy has declined.
/// Where both can decode a codec, Vulkan is strictly better: it writes into the render device
/// directly, whereas this pays for a device-local copy across the CUDA/Vulkan boundary.
///
/// The interop is FFmpeg's, not ours. `av_hwframe_transfer_data` into a Vulkan frame reaches
/// `vulkan_transfer_data_from_cuda`, which exports OUR frame's memory to CUDA, waits the frame's
/// own timeline semaphore, copies NVDEC's output in, and signals that semaphore at an incremented
/// value — exactly the AVVkFrame convention vulkan_frame_import already consumes, so from the
/// transfer onwards this and the Vulkan strategy are the same code.
///
/// Note there is no `hwmap` route: FFmpeg supports mapping only VAAPI and DRM_PRIME into Vulkan,
/// not CUDA. Transfer is the supported direction.
///
/// Verified on hardware with MPEG-2 4:2:0: NVDEC decodes, the transfer lands the frame on our
/// Vulkan device and the mixer draws it, with no fallback and no dropped frames. Note that NVDEC
/// is not bit-exact with FFmpeg's software decoders and is not required to be — MPEG-2 only
/// bounds IDCT accuracy — so expect small differences rather than identical output.
///
/// Two limits worth knowing, neither of them in this file: `bwdif_cuda` needs CUDA filter kernels
/// the installed driver can load (a mismatched nvcc yields CUDA_ERROR_UNSUPPORTED_PTX_VERSION at
/// graph-configure time, and the producer falls back), and VP8/VP9 never arrive here because
/// get_decoder() pins them to libvpx for WebM alpha.
class cuda_video_strategy : public video_strategy
{
    const std::unique_ptr<vulkan_frame_import> import_;
    const buffer_ref                           cuda_device_;

    /// The Vulkan frames the transfer writes into. Ours rather than a filter's, because it has to
    /// be created with DISABLE_MULTIPLANE: CUDA cannot import a multi-planar Vulkan image, and
    /// FFmpeg's own error for that case tells you to make one image per plane. It also suits the
    /// import, which then takes the simple image-per-plane path.
    buffer_ref vulkan_frames_;
    bool       warned_transfer_failed_ = false;

  public:
    cuda_video_strategy(std::unique_ptr<vulkan_frame_import> import, buffer_ref cuda_device)
        : import_(std::move(import))
        , cuda_device_(std::move(cuda_device))
    {
    }

    std::wstring name() const override { return L"cuda"; }

    bool open_decoder(AVCodecContext& ctx, const AVCodec& codec) override
    {
        // No allow-list here, unlike the Vulkan strategy: NVDEC's hwaccels are long-standing and
        // there is no equivalent of the broken compute-shader family to screen out.
        if (!supports_cuda(codec))
            return false;

        ctx.hw_device_ctx = av_buffer_ref(cuda_device_.get());
        if (!ctx.hw_device_ctx)
            return false;

        ctx.get_format = select_cuda_format;
        return true;
    }

    AVBufferRef* hw_device_context() const override { return cuda_device_.get(); }

    bool accepts(const AVFrame& frame) const override
    {
        // A software frame means get_format() declined NVDEC after all — either the codec has no
        // NVDEC hwaccel for this stream, or the hwaccel failed to initialise and FFmpeg quietly
        // retried without it.
        if (frame.format != AV_PIX_FMT_CUDA || !frame.hw_frames_ctx)
            return false;

        const auto* frames_ctx = reinterpret_cast<const AVHWFramesContext*>(frame.hw_frames_ctx->data);
        if (!vulkan_frame_import::has_mixer_layout(frames_ctx->sw_format, frames_ctx->width, frames_ctx->height)) {
            CASPAR_LOG(info) << L"[ffmpeg] The mixer has no layout for NVDEC frame format "
                             << u16(av_get_pix_fmt_name(frames_ctx->sw_format))
                             << L"; this file decodes on the CPU instead.";
            return false;
        }
        return true;
    }

    std::string deinterlace_filter(const std::string& deint) const override
    {
        return (boost::format("%s=mode=send_field:parity=auto:deint=%s") % deinterlacer % deint).str();
    }

    /// The graph stays in CUDA end to end; the hand-off to Vulkan happens in make_frame, where we
    /// control the destination frames context. (`hwupload` would do the transfer for us, but it
    /// builds its own frames context and offers no way to ask for DISABLE_MULTIPLANE.)
    std::vector<AVPixelFormat> sink_formats() const override { return {AV_PIX_FMT_CUDA}; }

    core::draw_frame make_frame(void*                            tag,
                                std::shared_ptr<AVFrame>         video,
                                std::shared_ptr<AVFrame>         audio,
                                core::color_space                color_space,
                                core::frame_geometry::scale_mode scale_mode) override
    {
        if (!video || video->format != AV_PIX_FMT_CUDA || !video->hw_frames_ctx || !video->data[0]) {
            return import_->host_frame(tag, std::move(video), std::move(audio), color_space, scale_mode);
        }

        auto vk_frame = to_vulkan(*video);
        if (!vk_frame) {
            // Losing the interop mid-stream leaves nothing sensible to show; say so once and drop
            // the video rather than spamming per frame.
            if (!warned_transfer_failed_) {
                warned_transfer_failed_ = true;
                CASPAR_LOG(warning) << L"[ffmpeg] Could not hand an NVDEC frame to Vulkan; dropping video.";
            }
            return core::draw_frame{};
        }

        return import_->import(tag, vk_frame, audio, color_space, scale_mode);
    }

  private:
    static bool supports_cuda(const AVCodec& codec)
    {
        for (int i = 0;; ++i) {
            const auto* config = avcodec_get_hw_config(&codec, i);
            if (!config)
                return false;
            if ((config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) &&
                config->device_type == AV_HWDEVICE_TYPE_CUDA)
                return true;
        }
    }

    /// The pool the transfer writes into, built on first use because it needs the format and size
    /// the decoder settled on.
    bool ensure_vulkan_frames(const AVFrame& src)
    {
        if (vulkan_frames_)
            return true;

        const auto* src_ctx = reinterpret_cast<const AVHWFramesContext*>(src.hw_frames_ctx->data);

        auto* ref = av_hwframe_ctx_alloc(import_->device());
        if (!ref)
            return false;
        auto frames = wrap_buffer_ref(ref);

        auto* ctx     = reinterpret_cast<AVHWFramesContext*>(ref->data);
        ctx->format    = AV_PIX_FMT_VULKAN;
        ctx->sw_format = src_ctx->sw_format;
        ctx->width     = src_ctx->width;
        ctx->height    = src_ctx->height;

        auto* vk = static_cast<AVVulkanFramesContext*>(ctx->hwctx);
        // One image per plane: CUDA cannot import a multi-planar Vulkan image.
        vk->flags = static_cast<AVVkFrameFlags>(vk->flags | AV_VK_FRAME_FLAG_DISABLE_MULTIPLANE);
        // Transfer dst for the incoming copy, transfer src for the copy into mixer textures.
        vk->usage = static_cast<VkImageUsageFlagBits>(VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                                     VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                     VK_IMAGE_USAGE_SAMPLED_BIT);

        const auto ret = av_hwframe_ctx_init(ref);
        if (ret < 0) {
            CASPAR_LOG(warning) << L"[ffmpeg] Could not create a Vulkan frame pool for NVDEC output: "
                                << error_text(ret);
            return false;
        }

        vulkan_frames_ = std::move(frames);
        return true;
    }

    /// One NVDEC frame, transferred onto the render device. Null on failure.
    std::shared_ptr<AVFrame> to_vulkan(const AVFrame& src)
    {
        if (!ensure_vulkan_frames(src))
            return nullptr;

        auto dst = alloc_frame();

        auto ret = av_hwframe_get_buffer(vulkan_frames_.get(), dst.get(), 0);
        if (ret < 0) {
            CASPAR_LOG(warning) << L"[ffmpeg] Out of Vulkan frames for NVDEC output: " << error_text(ret);
            return nullptr;
        }

        // The interop proper. FFmpeg exports dst's memory to CUDA and copies into it, ordering the
        // whole thing on dst's timeline semaphores, so the frame comes back already synchronised
        // the way vulkan_frame_import expects.
        ret = av_hwframe_transfer_data(dst.get(), &src, 0);
        if (ret < 0) {
            CASPAR_LOG(warning) << L"[ffmpeg] CUDA to Vulkan transfer failed: " << error_text(ret);
            return nullptr;
        }

        // Timestamps, colour properties and interlacing flags all live on the frame, not the pool.
        ret = av_frame_copy_props(dst.get(), &src);
        if (ret < 0) {
            return nullptr;
        }

        return dst;
    }
};

std::shared_ptr<video_strategy>
try_create_cuda_video_strategy(const std::shared_ptr<core::frame_factory>& frame_factory)
{
    const auto deint =
        env::properties().get<std::wstring>(L"configuration.ffmpeg.producer.auto-deinterlace", L"interlaced");
    if (deint != L"none" && !avfilter_get_by_name(deinterlacer)) {
        CASPAR_LOG(debug) << L"[ffmpeg] This FFmpeg build has no " << u16(deinterlacer)
                          << L" filter, so NVDEC decoding cannot deinterlace; not offering it.";
        return nullptr;
    }

    auto import = vulkan_frame_import::create(frame_factory);
    if (!import)
        return nullptr;

    // Derive rather than create, so CUDA lands on the same physical GPU as the Vulkan device:
    // hwcontext_cuda matches them by device UUID. It also fails cleanly on an FFmpeg built
    // without CUDA, which is the only availability check needed.
    AVBufferRef* cuda = nullptr;
    const auto   ret  = av_hwdevice_ctx_create_derived(&cuda, AV_HWDEVICE_TYPE_CUDA, import->device(), 0);
    if (ret < 0) {
        CASPAR_LOG(debug) << L"[ffmpeg] No CUDA device alongside the Vulkan one (" << error_text(ret)
                          << L"); NVDEC decoding unavailable.";
        return nullptr;
    }

    CASPAR_LOG(info) << L"[ffmpeg] NVDEC decoding available, on the same device as the accelerator.";
    return std::make_shared<cuda_video_strategy>(std::move(import), wrap_buffer_ref(cuda));
}

}} // namespace caspar::ffmpeg
