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

#include <boost/algorithm/string.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/filesystem.hpp>

#include <set>

extern "C" {
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/dict.h>
#include <libavutil/log.h>
#include <libavutil/pixfmt.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

namespace caspar { namespace image {

// Based on: https://github.com/FFmpeg/FFmpeg/blob/master/libavfilter/lavfutils.c
int ff_load_image(uint8_t *data[4], int linesize[4],
                  int *w, int *h, enum AVPixelFormat *pix_fmt,
                  const char *filename, AVFormatContext *format_ctx, void *log_ctx)
{
    const AVCodec *codec;
    AVCodecContext *codec_ctx = NULL;
    AVCodecParameters *par;
    AVFrame *frame = NULL;
    int ret = 0;
    AVPacket pkt;
    AVDictionary *opt = NULL;

    const AVInputFormat* iformat = av_find_input_format("image2pipe");
    if ((ret = avformat_open_input(&format_ctx, filename, iformat, nullptr)) < 0) {
        av_log(log_ctx, AV_LOG_ERROR,
               "Failed to open input file '%s'\n", filename);
        return ret;
    }

    if ((ret = avformat_find_stream_info(format_ctx, nullptr)) < 0) {
        av_log(log_ctx, AV_LOG_ERROR, "Find stream info failed\n");
        goto end;
    }

    par = format_ctx->streams[0]->codecpar;
    codec = avcodec_find_decoder(par->codec_id);
    if (!codec) {
        av_log(log_ctx, AV_LOG_ERROR, "Failed to find codec\n");
        ret = AVERROR(EINVAL);
        goto end;
    }

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        av_log(log_ctx, AV_LOG_ERROR, "Failed to alloc video decoder context\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    ret = avcodec_parameters_to_context(codec_ctx, par);
    if (ret < 0) {
        av_log(log_ctx, AV_LOG_ERROR, "Failed to copy codec parameters to decoder context\n");
        goto end;
    }

    av_dict_set(&opt, "thread_type", "slice", 0);
    if ((ret = avcodec_open2(codec_ctx, codec, &opt)) < 0) {
        av_log(log_ctx, AV_LOG_ERROR, "Failed to open codec\n");
        goto end;
    }

    if (!(frame = av_frame_alloc()) ) {
        av_log(log_ctx, AV_LOG_ERROR, "Failed to alloc frame\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    ret = av_read_frame(format_ctx, &pkt);
    if (ret < 0) {
        av_log(log_ctx, AV_LOG_ERROR, "Failed to read frame from file\n");
        goto end;
    }

    ret = avcodec_send_packet(codec_ctx, &pkt);
    av_packet_unref(&pkt);
    if (ret < 0) {
        av_log(log_ctx, AV_LOG_ERROR, "Error submitting a packet to decoder\n");
        goto end;
    }

    ret = avcodec_receive_frame(codec_ctx, frame);
    if (ret < 0) {
        av_log(log_ctx, AV_LOG_ERROR, "Failed to decode image from file\n");
        goto end;
    }

    *w       = frame->width;
    *h       = frame->height;
    *pix_fmt = static_cast<AVPixelFormat>(frame->format);

    if ((ret = av_image_alloc(data, linesize, *w, *h, *pix_fmt, 16)) < 0)
        goto end;
    ret = 0;

    av_image_copy2(data, linesize, frame->data, frame->linesize,
                   *pix_fmt, *w, *h);

end:
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);
    av_frame_free(&frame);
    av_dict_free(&opt);

    if (ret < 0)
        av_log(log_ctx, AV_LOG_ERROR, "Error loading image file '%s'\n", filename);
    return ret;
}

std::shared_ptr<AVFrame> load_image(const std::wstring& filename) {
    if (!boost::filesystem::exists(filename))
        CASPAR_THROW_EXCEPTION(file_not_found() << boost::errinfo_file_name(u8(filename)));

    auto src_video = ffmpeg::alloc_frame();
    int res = ff_load_image(src_video->data, src_video->linesize, &src_video->width, &src_video->height,
                            reinterpret_cast<AVPixelFormat *>(&src_video->format), u8(filename).c_str(), nullptr, nullptr);
    if (res != 0)
        CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info("Unsupported image format."));

    return std::move(src_video);
}


static int readFunction(void* opaque, uint8_t* buf, int buf_size) {
    auto& data = *reinterpret_cast<std::vector<unsigned char>*>(opaque);
    memcpy(buf, data.data(), buf_size);
    return data.size();
}

std::shared_ptr<AVFrame> load_from_memory(std::vector<unsigned char> image_data) {
    const std::shared_ptr<unsigned char> buffer(reinterpret_cast<unsigned char*>(av_malloc(image_data.size())), &av_free);

    auto avioContext = avio_alloc_context(buffer.get(), image_data.size(), 0, reinterpret_cast<void*>(&image_data), &readFunction, nullptr, nullptr);

    const auto avFormat = std::shared_ptr<AVFormatContext>(avformat_alloc_context(), &avformat_free_context);
    avFormat->pb = avioContext;

    auto src_video = ffmpeg::alloc_frame();
    int res = ff_load_image(src_video->data, src_video->linesize, &src_video->width, &src_video->height,
                            reinterpret_cast<AVPixelFormat *>(&src_video->format), "base64",  avFormat.get(), nullptr);

    avio_context_free(&avioContext);

    if (res != 0)
        CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info("Unsupported image format."));

    return std::move(src_video);
}

bool is_valid_file(const boost::filesystem::path& filename)
{
    static const std::set<std::wstring> extensions = {
        L".png", L".tga", L".bmp", L".jpg", L".jpeg", L".gif", L".tiff", L".tif", L".jp2", L".jpx", L".j2k", L".j2c"};

    auto ext = boost::to_lower_copy(boost::filesystem::path(filename).extension().wstring());
    if (extensions.find(ext) == extensions.end()) {
        return false;
    }

    return true;
}

}} // namespace caspar::image
