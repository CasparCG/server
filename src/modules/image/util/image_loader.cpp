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

#include "image_loader.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#if defined(_MSC_VER)
#include <windows.h>
#endif
#include <FreeImage.h>

#include <common/except.h>
#include <common/utf.h>

#include <core/frame/pixel_format.h>

#if defined(_MSC_VER)
#pragma warning(disable : 4714) // marked as __forceinline not inlined
#endif

#include <boost/algorithm/string.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/filesystem.hpp>

#include "image_algorithms.h"
#include "image_view.h"

#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
#define IMAGE_BGRA_FORMAT core::pixel_format::bgra
#define IMAGE_BGR_FORMAT core::pixel_format::bgr
#else
#define IMAGE_BGRA_FORMAT core::pixel_format::rgba
#define IMAGE_BGR_FORMAT core::pixel_format::rgb
#endif

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


int ff_load_image(uint8_t *data[4], int linesize[4],
                  int *w, int *h, enum AVPixelFormat *pix_fmt,
                  const char *filename, AVFormatContext *format_ctx, void *log_ctx)
{
    const AVInputFormat *iformat = NULL;
    const AVCodec *codec;
    AVCodecContext *codec_ctx = NULL;
    AVCodecParameters *par;
    AVFrame *frame = NULL;
    int ret = 0;
    AVPacket pkt;
    AVDictionary *opt=NULL;

    iformat = av_find_input_format("image2pipe");
    if ((ret = avformat_open_input(&format_ctx, filename, iformat, NULL)) < 0) {
        av_log(log_ctx, AV_LOG_ERROR,
               "Failed to open input file '%s'\n", filename);
        return ret;
    }

    if ((ret = avformat_find_stream_info(format_ctx, NULL)) < 0) {
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


loaded_image prepare_loaded_image(FREE_IMAGE_FORMAT fif, std::shared_ptr<FIBITMAP> bitmap, bool allow_all_formats)
{
    core::pixel_format format;
    int                stride;
    common::bit_depth  depth = common::bit_depth::bit8;

    unsigned int bpp = FreeImage_GetBPP(bitmap.get());

    if (bpp == 32) {
        format = IMAGE_BGRA_FORMAT;
        stride = 4;
    } else if (allow_all_formats && bpp == 24) {
        format = IMAGE_BGR_FORMAT;
        stride = 3;
    } else if (allow_all_formats && bpp == 64) {
        // freeimage appears to ignore endianness
        format = core::pixel_format::rgba;
        stride = 4;
        depth  = common::bit_depth::bit16;
    } else if (allow_all_formats && bpp == 48) {
        // freeimage appears to ignore endianness
        format = core::pixel_format::rgb;
        stride = 3;
        depth  = common::bit_depth::bit16;
    } else if (allow_all_formats && !FreeImage_IsTransparent(bitmap.get())) {
        format = IMAGE_BGR_FORMAT;
        stride = 3;

        bitmap = std::shared_ptr<FIBITMAP>(FreeImage_ConvertTo24Bits(bitmap.get()), FreeImage_Unload);

    } else {
        format = IMAGE_BGRA_FORMAT;
        stride = 4;

        bitmap = std::shared_ptr<FIBITMAP>(FreeImage_ConvertTo32Bits(bitmap.get()), FreeImage_Unload);
    }

    if (!bitmap)
        CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info("Unsupported image format."));

    // PNG-images need to be premultiplied with their alpha
    bool is_straight = fif == FIF_PNG && (format == core::pixel_format::bgra || format == core::pixel_format::rgba);
    if (!allow_all_formats && is_straight) {
        image_view<bgra_pixel> original_view(
            FreeImage_GetBits(bitmap.get()), FreeImage_GetWidth(bitmap.get()), FreeImage_GetHeight(bitmap.get()));
        premultiply(original_view);
    }

    return {std::move(bitmap), format, stride, depth, is_straight};
}

bool is_frame_compatible_with_mixer(const std::shared_ptr<AVFrame>& src) {
    std::vector<int> data_map;
    auto sample_pix_desc =
            ffmpeg::pixel_format_desc(
                    static_cast<AVPixelFormat>(src->format), src->width, src->height, data_map, core::color_space::bt709);
   return sample_pix_desc.format != core::pixel_format::invalid;
}

std::shared_ptr<AVFrame> convert_to_bgra32(const std::shared_ptr<AVFrame>& src) {
    const auto target_format = AV_PIX_FMT_BGRA;

    if (src->format == target_format) return src;

    std::shared_ptr<SwsContext> sws;
    sws.reset(sws_getContext(
                      src->width, src->height, static_cast<AVPixelFormat>(src->format), src->width, src->height, target_format, 0, nullptr, nullptr, nullptr),
              [](SwsContext* ptr) { sws_freeContext(ptr); });

    if (!sws) {
        CASPAR_THROW_EXCEPTION(caspar_exception());
    }

    auto dest = ffmpeg::alloc_frame();
    dest->sample_aspect_ratio = src->sample_aspect_ratio;
    dest->width               = src->width;
    dest->height              = src->height;
    dest->format              = target_format;
    dest->colorspace          = AVCOL_SPC_BT709;
    av_frame_get_buffer(dest.get(), 64);

    sws_scale_frame(sws.get(), dest.get(), src.get());

    return dest;
}

std::shared_ptr<AVFrame> load_image2(const std::wstring& filename) {
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

std::shared_ptr<AVFrame> load_from_memory2(std::vector<unsigned char> image_data) {
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

loaded_image load_image(const std::wstring& filename, bool allow_all_formats)
{
    if (!boost::filesystem::exists(filename))
        CASPAR_THROW_EXCEPTION(file_not_found() << boost::errinfo_file_name(u8(filename)));



        #ifdef WIN32
            FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeU(filename.c_str(), 0);
        #else
            FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(u8(filename).c_str(), 0);
        #endif

            if (fif == FIF_UNKNOWN)
        #ifdef WIN32
                fif = FreeImage_GetFIFFromFilenameU(filename.c_str());
        #else
                fif = FreeImage_GetFIFFromFilename(u8(filename).c_str());
        #endif

            if (fif == FIF_UNKNOWN || (FreeImage_FIFSupportsReading(fif) == 0) || fif == FIF_PNG)
                CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info("Unsupported image format."));

        #ifdef WIN32
            auto bitmap = std::shared_ptr<FIBITMAP>(FreeImage_LoadU(fif, filename.c_str(), JPEG_EXIFROTATE), FreeImage_Unload);
        #else
            auto bitmap =
                std::shared_ptr<FIBITMAP>(FreeImage_Load(fif, u8(filename).c_str(), JPEG_EXIFROTATE), FreeImage_Unload);
        #endif

            return prepare_loaded_image(fif, std::move(bitmap), allow_all_formats);
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
