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

#pragma once

#include <accelerator/vulkan/util/gpu_producer.h>

#include <core/frame/draw_frame.h>
#include <core/frame/geometry.h>
#include <core/frame/pixel_format.h>

extern "C" {
#include <libavutil/pixfmt.h>
}

#include <memory>
#include <optional>
#include <vector>

struct AVBufferRef;
struct AVFrame;

namespace caspar { namespace core {
class frame_factory;
}} // namespace caspar::core

namespace caspar { namespace ffmpeg {

/// The half of a hardware video strategy that does not care how the frame reached the GPU.
///
/// Both hardware strategies end with an AV_PIX_FMT_VULKAN frame on the accelerator's own device
/// and have to turn it into mixer textures; only the way the pixels get there differs (FFmpeg's
/// Vulkan decoder writes them, or NVDEC writes them and FFmpeg copies them across). That common
/// half lives here: the shared AVHWDeviceContext wrapping our device, the producer's command
/// context, the AVVkFrame -> texture conversion, and the host path for frames that never became
/// hardware frames at all.
class vulkan_frame_import
{
  public:
    /// For a strategy whose decoder writes into our device, which FFmpeg therefore has to be told
    /// about. Null when this channel is not on the Vulkan accelerator, or when the device cannot
    /// be shared with FFmpeg — either way the caller should fall back.
    static std::unique_ptr<vulkan_frame_import> create(const std::shared_ptr<core::frame_factory>& frame_factory);

    /// For a strategy that imports frames some other API decoded into its own memory. FFmpeg
    /// never sees our device, so this needs nothing of FFmpeg's Vulkan support — which matters,
    /// because the macOS builds ship none: their libavutil has the VideoToolbox hardware device
    /// type and no other. device() is null on an import made this way.
    static std::unique_ptr<vulkan_frame_import>
    create_for_import(const std::shared_ptr<core::frame_factory>& frame_factory);

    ~vulkan_frame_import();

    /// The AVHWDeviceContext wrapping the accelerator's Vulkan device, shared process-wide, or
    /// null when this import was made with create_for_import. Owned here; a caller may reference
    /// it, never free it.
    AVBufferRef* device() const;

    /// Whether the accelerator's device was created with an optional extension, for a strategy
    /// whose interop depends on one.
    bool has_device_extension(const char* name) const;

    /// The accelerator's Vulkan device, for a strategy that creates images of its own (importing
    /// them from another API) rather than receiving them from FFmpeg.
    vk::Device vk_device() const;

    /// Whether the mixer has a plane layout for a decoder's software format. The mixer samples
    /// one texture per plane, so a format it cannot describe cannot be imported however it
    /// arrives — every strategy screens its decoder's chosen format through this.
    static bool has_mixer_layout(AVPixelFormat sw_format, int width, int height);

    /// That layout itself, for a strategy that has to build the images before it can hand them
    /// over — the plane count, sizes and sample widths it must match.
    static std::optional<core::pixel_format_desc>
    mixer_layout(AVPixelFormat sw_format, int width, int height, core::color_space color_space);

    /// An AV_PIX_FMT_VULKAN frame, as the mixer takes it: one mixer texture per plane, copied on
    /// the producer's own queue, with FFmpeg's frame released back to its pool on the same submit.
    core::draw_frame import(void*                            tag,
                            const std::shared_ptr<AVFrame>&  video,
                            const std::shared_ptr<AVFrame>&  audio,
                            core::color_space                color_space,
                            core::frame_geometry::scale_mode scale_mode);

    /// A frame that already lives in images on our device, as the mixer takes it: one mixer
    /// texture per plane, copied on the producer's own queue.
    ///
    /// `images` is one per plane or a single multi-planar image, and `layouts` says what each is
    /// in on arrival. `sync` carries the source API's timelines when it has any — FFmpeg's Vulkan
    /// frames do — and is empty when the frame is simply finished by the time we see it, which is
    /// the VideoToolbox case. `keep_alive` is then held until the copy retires, because a source
    /// with no timeline has no other way of learning that its surface may be recycled.
    core::draw_frame import_images(void*                                     tag,
                                   const std::vector<vk::Image>&             images,
                                   const std::vector<vk::ImageLayout>&       layouts,
                                   const core::pixel_format_desc&            desc,
                                   const accelerator::vulkan::external_sync& sync,
                                   const std::shared_ptr<AVFrame>&           audio,
                                   core::frame_geometry::scale_mode          scale_mode,
                                   std::shared_ptr<void>                     keep_alive = nullptr);

    /// Wait until every copy this import submitted has retired, so a caller may destroy images it
    /// handed over. Only a strategy that owns its own images needs this.
    void drain();

    /// Anything that never became a hardware frame — the audio-only tail at end of file, or a
    /// stream whose decoder fell back to software — delivered through the ordinary host path.
    core::draw_frame host_frame(void*                            tag,
                                std::shared_ptr<AVFrame>         video,
                                std::shared_ptr<AVFrame>         audio,
                                core::color_space                color_space,
                                core::frame_geometry::scale_mode scale_mode);

  private:
    struct impl;
    std::unique_ptr<impl> impl_;

    explicit vulkan_frame_import(std::unique_ptr<impl> i);
};

}} // namespace caspar::ffmpeg
