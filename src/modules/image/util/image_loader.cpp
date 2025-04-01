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
 * Author: Julian Waller, julian@superfly.tv
 */

#include "image_loader.h"
#include "image_algorithms.h"

#include <common/except.h>

#if defined(_MSC_VER)
#pragma warning(disable : 4714) // marked as __forceinline not inlined
#endif

#include "common/scope_exit.h"

#include <ffmpeg/util/av_assert.h>

#include <boost/algorithm/string.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/filesystem.hpp>

#include <set>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
extern "C" {
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace caspar { namespace image {

// Based on: https://github.com/FFmpeg/FFmpeg/blob/master/libavfilter/lavfutils.c
std::shared_ptr<AVFrame> ff_load_image(const char* filename, AVFormatContext* format_ctx)
{
    auto frame = ffmpeg::alloc_frame();

    bool has_input_format_ctx = format_ctx != nullptr;

#if LIBAVFORMAT_VERSION_MAJOR >= 59
    const AVInputFormat* input_format = nullptr;
#else
    AVInputFormat* input_format = nullptr;
#endif

    input_format = av_find_input_format("image2pipe");
    FF(avformat_open_input(&format_ctx, filename, input_format, nullptr));
    CASPAR_SCOPE_EXIT
    {
        if (!has_input_format_ctx)
            avformat_close_input(&format_ctx);
    };

    FF(avformat_find_stream_info(format_ctx, nullptr));

    const AVCodecParameters* par   = format_ctx->streams[0]->codecpar;
    const AVCodec*           codec = avcodec_find_decoder(par->codec_id);
    if (!codec) {
        FF_RET(AVERROR(EINVAL), "avcodec_find_decoder");
    }

    auto codec_ctx = std::shared_ptr<AVCodecContext>(avcodec_alloc_context3(codec),
                                                     [](AVCodecContext* ptr) { avcodec_free_context(&ptr); });
    if (!codec_ctx) {
        FF_RET(AVERROR(ENOMEM), "avcodec_alloc_context3");
    }

    FF(avcodec_parameters_to_context(codec_ctx.get(), par));

    AVDictionary* opt = nullptr;
    CASPAR_SCOPE_EXIT { av_dict_free(&opt); };

    av_dict_set(&opt, "thread_type", "slice", 0);
    FF(avcodec_open2(codec_ctx.get(), codec, &opt));

    AVPacket pkt;
    FF(av_read_frame(format_ctx, &pkt));

    const int ret = avcodec_send_packet(codec_ctx.get(), &pkt);
    av_packet_unref(&pkt);
    FF_RET(ret, "avcodec_send_packet");

    FF(avcodec_receive_frame(codec_ctx.get(), frame.get()));

    return frame;
}

std::shared_ptr<AVFrame> load_image(const std::wstring& filename)
{
    if (!boost::filesystem::exists(filename))
        CASPAR_THROW_EXCEPTION(file_not_found() << boost::errinfo_file_name(u8(filename)));

    return ff_load_image(u8(filename).c_str(), nullptr);
}

static int readFunction(void* opaque, uint8_t* buf, int buf_size)
{
    auto& data = *static_cast<std::vector<unsigned char>*>(opaque);
    memcpy(buf, data.data(), buf_size);
    return static_cast<int>(data.size());
}

std::shared_ptr<AVFrame> load_from_memory(std::vector<unsigned char> image_data)
{
    const std::shared_ptr<unsigned char> buffer(static_cast<unsigned char*>(av_malloc(image_data.size())), &av_free);

    auto avioContext = std::shared_ptr<AVIOContext>(avio_alloc_context(buffer.get(),
                                                                       static_cast<int>(image_data.size()),
                                                                       0,
                                                                       reinterpret_cast<void*>(&image_data),
                                                                       &readFunction,
                                                                       nullptr,
                                                                       nullptr),
                                                    [](AVIOContext* ptr) { avio_context_free(&ptr); });

    const auto avFormat = std::shared_ptr<AVFormatContext>(avformat_alloc_context(), &avformat_free_context);
    avFormat->pb        = avioContext.get();

    return ff_load_image("base64", avFormat.get());
}

bool is_valid_file(const boost::filesystem::path& filename)
{
    static const std::set<std::wstring> extensions = {L".png",
                                                      L".tga",
                                                      L".bmp",
                                                      L".jpg",
                                                      L".jpeg",
                                                      L".gif",
                                                      L".tiff",
                                                      L".tif",
                                                      L".jp2",
                                                      L".jpx",
                                                      L".j2k",
                                                      L".j2c",
                                                      L".webp"};

    auto ext = boost::to_lower_copy(boost::filesystem::path(filename).extension().wstring());
    if (extensions.find(ext) == extensions.end()) {
        return false;
    }

    return true;
}

}} // namespace caspar::image
