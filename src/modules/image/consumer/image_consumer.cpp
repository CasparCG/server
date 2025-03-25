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

#include "image_consumer.h"

#if defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include <common/array.h>
#include <common/env.h>
#include <common/except.h>
#include <common/future.h>
#include <common/scope_exit.h>

#include <core/frame/frame.h>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <utility>
#include <vector>
#include <fstream>

#include <ffmpeg/util/av_assert.h>
#include <ffmpeg/util/av_util.h>

#include "../util/image_algorithms.h"
#include "../util/image_view.h"

extern "C" {
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include <libavutil/imgutils.h>
#include <libavutil/dict.h>
#include <libavutil/log.h>
#include <libavutil/pixfmt.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

namespace caspar { namespace image {

struct image_consumer : public core::frame_consumer
{
    const std::wstring filename_;

  public:
    // frame_consumer

    explicit image_consumer(std::wstring filename)
        : filename_(std::move(filename))
    {
    }

    void initialize(const core::video_format_desc& /*format_desc*/, int /*channel_index*/) override {}

    std::future<bool> send(core::video_field field, core::const_frame frame) override
    {
        auto filename = filename_;

        std::thread async([frame, filename] {
            try {
                std::string filename2;

                if (frame.pixel_format_desc().format != core::pixel_format::bgra) CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("image_consumer received frame with wrong format"));

                if (filename.empty())
                    filename2 = u8(env::media_folder() +
                                boost::posix_time::to_iso_wstring(boost::posix_time::second_clock::local_time()) +
                                L".png");
                else
                    filename2 = u8(env::media_folder() + filename + L".png");

                std::fstream file_stream(filename2, std::fstream::out | std::fstream::trunc | std::fstream::binary);
                if (!file_stream) FF_RET(AVERROR(EINVAL), "fstream_open");

                const AVCodec* codec = avcodec_find_encoder_by_name("png");
                if (!codec) FF_RET(AVERROR(EINVAL), "avcodec_find_encoder_by_name");

                AVCodecContext *ctx = avcodec_alloc_context3(codec);
                CASPAR_SCOPE_EXIT { avcodec_free_context(&ctx); };

                ctx->width = frame.width();
                ctx->height = frame.height();
                ctx->pix_fmt = AV_PIX_FMT_BGRA;
                ctx->time_base = {1, 1};
                ctx->framerate = {0,1};

                FF(avcodec_open2(ctx, codec, nullptr));

                auto av_frame = ffmpeg::alloc_frame();
                av_frame->width = frame.width();
                av_frame->height = frame.height();
                av_frame->format = AV_PIX_FMT_BGRA;
                av_frame->pts = 0;
                av_frame->linesize[0] = frame.width() * 4;
                av_frame->data[0] = (uint8_t*)frame.image_data(0).data();

                FF(avcodec_send_frame(ctx, av_frame.get()));
                FF(avcodec_send_frame(ctx, nullptr));

                AVPacket *pkt = av_packet_alloc();
                CASPAR_SCOPE_EXIT { av_packet_free(&pkt); };
                int ret = 0;
                while (ret >= 0) {
                    ret = avcodec_receive_packet(ctx, pkt);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    FF_RET(ret, "avcodec_receive_packet");

                    file_stream.write(reinterpret_cast<const char *>(pkt->data), pkt->size);
                    av_packet_unref(pkt);
                }

            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION()
            }
        });
        async.detach();

        return make_ready_future(false);
    }

    std::wstring print() const override { return L"image[]"; }

    std::wstring name() const override { return L"image"; }

    int index() const override { return 100; }

    core::monitor::state state() const override
    {
        core::monitor::state state;
        state["image/filename"] = u8(filename_);
        return state;
    }
};

spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>&     params,
                                                      const core::video_format_repository& format_repository,
                                                      const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                                                      common::bit_depth                                        depth)
{
    if (params.empty() || !boost::iequals(params.at(0), L"IMAGE"))
        return core::frame_consumer::empty();

    if (depth != common::bit_depth::bit8)
        CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Image consumer only supports 8-bit color depth."));

    std::wstring filename;

    if (params.size() > 1)
        filename = params.at(1);

    return spl::make_shared<image_consumer>(filename);
}

}} // namespace caspar::image
