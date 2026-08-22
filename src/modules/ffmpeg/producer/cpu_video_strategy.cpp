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

#include "../util/av_util.h"

#include <core/frame/frame_factory.h>

#include <boost/format.hpp>

#include <utility>

namespace caspar { namespace ffmpeg {

/// The original video path, unchanged: the decoder writes host frames, the software filter
/// graph deinterlaces and retimes them, and the frame factory uploads the result.
class cpu_video_strategy : public video_strategy
{
    const std::shared_ptr<core::frame_factory> frame_factory_;

  public:
    explicit cpu_video_strategy(std::shared_ptr<core::frame_factory> frame_factory)
        : frame_factory_(std::move(frame_factory))
    {
    }

    std::wstring name() const override { return L"cpu"; }

    bool open_decoder(AVCodecContext&, const AVCodec&) override { return true; }

    std::string deinterlace_filter(const std::string& deint) const override
    {
        return (boost::format("bwdif=mode=send_field:parity=auto:deint=%s") % deint).str();
    }

    std::vector<AVPixelFormat> sink_formats() const override
    {
        return {AV_PIX_FMT_RGB24,
                AV_PIX_FMT_BGR24,
                AV_PIX_FMT_BGRA,
                AV_PIX_FMT_ARGB,
                AV_PIX_FMT_RGBA,
                AV_PIX_FMT_ABGR,
                AV_PIX_FMT_YUV444P,
                AV_PIX_FMT_YUV444P10,
                AV_PIX_FMT_YUV444P12,
                AV_PIX_FMT_YUV422P,
                AV_PIX_FMT_YUV422P10,
                AV_PIX_FMT_YUV422P12,
                AV_PIX_FMT_YUV420P,
                AV_PIX_FMT_YUV420P10,
                AV_PIX_FMT_YUV420P12,
                AV_PIX_FMT_YUV410P,
                AV_PIX_FMT_YUVA444P,
                AV_PIX_FMT_YUVA422P,
                AV_PIX_FMT_YUVA420P,
                AV_PIX_FMT_UYVY422,
                // bwdif needs planar rgb
                AV_PIX_FMT_GBRP,
                AV_PIX_FMT_GBRP10,
                AV_PIX_FMT_GBRP12,
                AV_PIX_FMT_GBRP16,
                AV_PIX_FMT_GBRAP,
                AV_PIX_FMT_GBRAP16};
    }

    core::draw_frame make_frame(void*                            tag,
                                std::shared_ptr<AVFrame>         video,
                                std::shared_ptr<AVFrame>         audio,
                                core::color_space                color_space,
                                core::frame_geometry::scale_mode scale_mode) override
    {
        return core::draw_frame(ffmpeg::make_frame(
            tag, *frame_factory_, std::move(video), std::move(audio), color_space, scale_mode));
    }
};

spl::shared_ptr<video_strategy> create_cpu_video_strategy(std::shared_ptr<core::frame_factory> frame_factory)
{
    return spl::make_shared<cpu_video_strategy>(std::move(frame_factory));
}

}} // namespace caspar::ffmpeg
