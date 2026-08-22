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

#include <common/memory.h>

#include <core/frame/draw_frame.h>
#include <core/frame/geometry.h>
#include <core/frame/pixel_format.h>

extern "C" {
#include <libavutil/pixfmt.h>
}

#include <memory>
#include <string>
#include <vector>

struct AVBufferRef;
struct AVCodec;
struct AVCodecContext;
struct AVFrame;

namespace caspar { namespace core {
class frame_factory;
}} // namespace caspar::core

namespace caspar { namespace ffmpeg {

/// How the video half of the producer is realised, so that hardware decoding is a peer of
/// CPU decoding rather than a set of branches threaded through it.
///
/// A strategy owns only the three things the two genuinely disagree about: how the decoder
/// is opened, what the filter graph is allowed to contain, and how a decoded frame reaches
/// the mixer. Everything else about playback — packet scheduling, timing, seeking, looping,
/// buffering — is written once in AVProducer and never asks which strategy it holds.
class video_strategy
{
  protected:
    video_strategy() = default;

  public:
    video_strategy(const video_strategy&)            = delete;
    video_strategy& operator=(const video_strategy&) = delete;
    virtual ~video_strategy()                        = default;

    /// Identifies the strategy in the producer's log line.
    virtual std::wstring name() const = 0;

    /// Called on a video stream's codec context after avcodec_parameters_to_context and
    /// before avcodec_open2, so a hardware strategy can attach its device context and
    /// pixel-format callback. Returning false means this stream cannot be decoded this way
    /// (unsupported codec, no hardware present) and the producer falls back to CPU decoding.
    virtual bool open_decoder(AVCodecContext& ctx, const AVCodec& codec) = 0;

    /// The device decoded frames live on, or nullptr when they are ordinary host frames.
    /// Non-null carries a second meaning for the producer: the filter graph's source has to
    /// be told the decoder's AVHWFramesContext, and a hardware decoder only publishes that
    /// once it has seen a packet — so the producer primes the decoder before building the
    /// graph. Ownership stays with the strategy.
    virtual AVBufferRef* hw_device_context() const { return nullptr; }

    /// Whether frames described by this AVHWFramesContext can actually be handed to the mixer.
    /// Called once the decoder has published one, so a strategy that turns out not to cover the
    /// format the decoder chose can decline before any frame is lost rather than dropping video.
    /// Only meaningful for a strategy with a hw_device_context().
    virtual bool accepts_frames_context(AVBufferRef*) const { return true; }

    /// The deinterlacer for `deint` ("all" / "interlaced"), as one filter graph element, or
    /// empty for none. Hardware frames need a deinterlacer that runs on the same device, so
    /// this is not simply a shared constant.
    virtual std::string deinterlace_filter(const std::string& deint) const = 0;

    /// The pixel formats the graph's sink may end in. Never empty.
    virtual std::vector<AVPixelFormat> sink_formats() const = 0;

    /// One filtered frame, as the mixer takes it. `video` and `audio` may each be null, and
    /// `video` is in one of the formats sink_formats() allowed.
    virtual core::draw_frame make_frame(void*                            tag,
                                        std::shared_ptr<AVFrame>         video,
                                        std::shared_ptr<AVFrame>         audio,
                                        core::color_space                color_space,
                                        core::frame_geometry::scale_mode scale_mode) = 0;
};

/// Decode into host memory, filter with the software filters, upload through the frame
/// factory. Always available.
spl::shared_ptr<video_strategy> create_cpu_video_strategy(std::shared_ptr<core::frame_factory> frame_factory);

#ifdef ENABLE_VULKAN
/// Decode straight into GPU images on the accelerator's own Vulkan device, filter with the
/// Vulkan filters, and hand the mixer the resulting textures — no host round trip.
/// Returns nullptr when this channel or this machine cannot do it, which is the caller's cue
/// to use the CPU strategy.
std::shared_ptr<video_strategy> try_create_vulkan_video_strategy(const std::shared_ptr<core::frame_factory>& frame_factory);
#endif

}} // namespace caspar::ffmpeg
