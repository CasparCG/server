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
 * Author: Niklas Andersson, niklas@nxtedition.com
 */

#include "../StdAfx.h"

#ifdef USE_SIMDE
#define SIMDE_ENABLE_NATIVE_ALIASES
#include <simde/x86/avx2.h>
#endif

#include "hdr_v210_strategy.h"

#include <common/memshfl.h>

#include <tbb/parallel_for.h>
#include <tbb/scalable_allocator.h>

namespace caspar { namespace decklink {

std::vector<int32_t> create_int_matrix(const std::vector<float>& matrix)
{
    static const float LumaRangeWidth   = 876.f * (1024.f / 1023.f);
    static const float ChromaRangeWidth = 896.f * (1024.f / 1023.f);

    std::vector<float> color_matrix_f(matrix);

    color_matrix_f[0] *= LumaRangeWidth;
    color_matrix_f[1] *= LumaRangeWidth;
    color_matrix_f[2] *= LumaRangeWidth;

    color_matrix_f[3] *= ChromaRangeWidth;
    color_matrix_f[4] *= ChromaRangeWidth;
    color_matrix_f[5] *= ChromaRangeWidth;
    color_matrix_f[6] *= ChromaRangeWidth;
    color_matrix_f[7] *= ChromaRangeWidth;
    color_matrix_f[8] *= ChromaRangeWidth;

    std::vector<int32_t> int_matrix(color_matrix_f.size());

    transform(color_matrix_f.cbegin(), color_matrix_f.cend(), int_matrix.begin(), [](const float& f) {
        return (int32_t)round(f * 1024.f);
    });

    return int_matrix;
};

inline void rgb_to_yuv_avx2(__m256i                pixel_pairs[4],
                            const std::vector<int32_t>& color_matrix,
                            __m256i*               luma_out,
                            __m256i*               chroma_out)
{
    /* COMPUTE LUMA */
    {
        __m256i y_coeff =
            _mm256_broadcastsi128_si256(_mm_set_epi32(0, color_matrix[2], color_matrix[1], color_matrix[0]));
        __m256i y_offset = _mm256_set1_epi32(64 << 20);

        // Multiply by y-coefficients
        __m256i y4[4];
        for (int i = 0; i < 4; i++) {
            y4[i] = _mm256_mullo_epi32(pixel_pairs[i], y_coeff);
        }

        // sum products
        __m256i y2_sum0123    = _mm256_hadd_epi32(y4[0], y4[1]);
        __m256i y2_sum4567    = _mm256_hadd_epi32(y4[2], y4[3]);
        __m256i y_sum01452367 = _mm256_hadd_epi32(y2_sum0123, y2_sum4567);
        *luma_out                  = _mm256_srli_epi32(_mm256_add_epi32(y_sum01452367, y_offset),
                                           20); // add offset and shift down to 10 bit precision
    }

    /* COMPUTE CHROMA */
    {
        __m256i cb_coeff =
            _mm256_broadcastsi128_si256(_mm_set_epi32(0, color_matrix[5], color_matrix[4], color_matrix[3]));
        __m256i cr_coeff =
            _mm256_broadcastsi128_si256(_mm_set_epi32(0, color_matrix[8], color_matrix[7], color_matrix[6]));
        __m256i c_offset = _mm256_set1_epi32((1025) << 19);

        // Multiply by cb-coefficients
        __m256i cbcr4[4]; // 0 = cb02, 1 = cr02, 2 = cb46, 3 = cr46
        for (int i = 0; i < 2; i++) {
            cbcr4[i * 2]     = _mm256_mullo_epi32(pixel_pairs[i * 2], cb_coeff);
            cbcr4[i * 2 + 1] = _mm256_mullo_epi32(pixel_pairs[i * 2], cr_coeff);
        }

        // sum products
        __m256i cbcr_sum02    = _mm256_hadd_epi32(cbcr4[1], cbcr4[0]);
        __m256i cbcr_sum46    = _mm256_hadd_epi32(cbcr4[3], cbcr4[2]);
        __m256i cbcr_sum_0426 = _mm256_hadd_epi32(cbcr_sum02, cbcr_sum46);
        *chroma_out                = _mm256_srli_epi32(_mm256_add_epi32(cbcr_sum_0426, c_offset),
                                             20); // add offset and shift down to 10 bit precision
    }
}

inline void pack_v210_avx2(__m256i luma[6], __m256i chroma[6], __m128i** v210_dest)
{
    __m256i luma_16bit[3];
    __m256i chroma_16bit[3];
    __m256i offsets = _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0);
    for (int i = 0; i < 3; i++) {
        auto y16    = _mm256_packus_epi32(luma[i * 2], luma[i * 2 + 1]);
        auto cbcr16 = _mm256_packus_epi32(chroma[i * 2],
                                               chroma[i * 2 + 1]); // cbcr0 cbcr4 cbcr8 cbcr12
                                                                   // cbcr2 cbcr6 cbcr10 cbcr14
        luma_16bit[i] =
            _mm256_permutevar8x32_epi32(y16,
                                             offsets); // layout 0 1   2 3   4 5   6 7   8 9   10 11   12 13   14 15
        chroma_16bit[i] =
            _mm256_permutevar8x32_epi32(cbcr16,
                                             offsets); // cbcr0 cbcr2 cbcr4 cbcr6   cbcr8 cbcr10 cbcr12 cbcr14
    }

    __m128i chroma_mult = _mm_set_epi16(0, 0, 4, 16, 1, 4, 16, 1);
    __m128i chroma_shuf = _mm_set_epi8(-1, 11, 10, -1, 9, 8, 7, 6, -1, 5, 4, -1, 3, 2, 1, 0);

    __m128i luma_mult = _mm_set_epi16(0, 0, 16, 1, 4, 16, 1, 4);
    __m128i luma_shuf = _mm_set_epi8(11, 10, 9, 8, -1, 7, 6, -1, 5, 4, 3, 2, -1, 1, 0, -1);

    uint16_t* luma_ptr   = reinterpret_cast<uint16_t*>(luma_16bit);
    uint16_t* chroma_ptr = reinterpret_cast<uint16_t*>(chroma_16bit);
    for (int i = 0; i < 8; ++i) {
        __m128i luma_values   = _mm_loadu_si128(reinterpret_cast<__m128i*>(luma_ptr));
        __m128i chroma_values = _mm_loadu_si128(reinterpret_cast<__m128i*>(chroma_ptr));
        __m128i luma_packed   = _mm_mullo_epi16(luma_values, luma_mult);
        __m128i chroma_packed = _mm_mullo_epi16(chroma_values, chroma_mult);

        luma_packed   = _mm_shuffle_epi8(luma_packed, luma_shuf);
        chroma_packed = _mm_shuffle_epi8(chroma_packed, chroma_shuf);

        auto res = _mm_or_si128(luma_packed, chroma_packed);
        _mm_store_si128((*v210_dest)++, res);

        luma_ptr += 6;
        chroma_ptr += 6;
    }
}

struct ARGBPixel
{
    uint16_t R;
    uint16_t G;
    uint16_t B;
    uint16_t A;
};

void pack_v210(const ARGBPixel* src, const std::vector<int32_t>& color_matrix, uint32_t* dest, int num_pixels)
{
    auto write_v210 = [dest, index = 0, shift = 0](uint32_t val) mutable {
        dest[index] |= ((val & 0x3FF) << shift);

        shift += 10;
        if (shift >= 30) {
            index++;
            shift = 0;
        }
    };

    for (int x = 0; x < num_pixels; ++x, ++src) {
        auto r = src->R >> 6;
        auto g = src->G >> 6;
        auto b = src->B >> 6;

        if (x % 2 == 0) {
            // Compute Cr
            uint32_t v = 1025 << 19;
            v += (uint32_t)(color_matrix[6] * r + color_matrix[7] * g + color_matrix[8] * b);
            v >>= 20;
            write_v210(v);
        }

        // Compute Y
        uint32_t luma = 64 << 20;
        luma += (color_matrix[0] * r + color_matrix[1] * g + color_matrix[2] * b);
        luma >>= 20;
        write_v210(luma);

        if (x % 2 == 0) {
            // Compute Cb
            uint32_t u = 1025 << 19;
            u += (int32_t)(color_matrix[3] * r + color_matrix[4] * g + color_matrix[5] * b);
            u >>= 20;
            write_v210(u);
        }
    }
}

struct hdr_v210_strategy::impl final
{
    std::vector<float> bt709{0.2126, 0.7152, 0.0722, -0.1146, -0.3854, 0.5, 0.5, -0.4542, -0.0458};
    __m128i       black_batch;

  public:
    impl()
    {
        // setup black batch (6 pixels of black, encoded as v210)
        auto      color_matrix = create_int_matrix(bt709);
        ARGBPixel black[6];
        memset(black, 0, sizeof(black));
        memset(&black_batch, 0, sizeof(__m128i));
        pack_v210(black, color_matrix, reinterpret_cast<uint32_t*>(&black_batch), 6);
    }

    BMDPixelFormat get_pixel_format() { return bmdFormat10BitYUV; }

    int get_row_bytes(int width) { return ((width + 47) / 48) * 128; }

    std::shared_ptr<void> allocate_frame_data(const core::video_format_desc& format_desc)
    {
        auto size = get_row_bytes(format_desc.width) * format_desc.height;
        return create_aligned_buffer(size, 128);
    }

    void convert_frame(const core::video_format_desc& channel_format_desc,
                       const core::video_format_desc& decklink_format_desc,
                       const port_configuration&      config,
                       std::shared_ptr<void>&         image_data,
                       bool                           topField,
                       const core::const_frame&       frame)
    {
        // No point copying an empty frame
        if (!frame)
            return;

        int  firstLine    = topField ? 0 : 1;
        auto color_matrix = create_int_matrix(bt709);

        if (config.region_w == 0 && config.region_h == 0 && config.dest_x == 0) {
            // Fast path

            // Pack R16G16B16A16 as v210
            int       pixels_to_copy  = std::min(decklink_format_desc.width, channel_format_desc.width - config.src_x);
            const int NUM_THREADS     = 6;
            auto      rows_per_thread = decklink_format_desc.height / NUM_THREADS;
            size_t    dest_line_bytes = get_row_bytes(decklink_format_desc.width);
            int       fullspeed_x_batches = pixels_to_copy / 48;
            int       rest_x_pixels       = pixels_to_copy - fullspeed_x_batches * 48;
            tbb::parallel_for(0, NUM_THREADS, [&](int thread_index) {
                auto start_y = firstLine + thread_index * rows_per_thread;
                auto end_y   = (thread_index + 1) * rows_per_thread;

                for (uint64_t y = start_y; y < end_y; y += decklink_format_desc.field_count) {
                    auto dest_row =
                        reinterpret_cast<uint8_t*>(image_data.get()) + y * dest_line_bytes; // start of dest row
                    __m128i* v210_dest = reinterpret_cast<__m128i*>(dest_row); // current position in dest row

                    if (y < config.dest_y) {
                        // Fill the row with black
                        auto black_batch_count = dest_line_bytes / sizeof(__m128i);
                        for (int i = 0; i < black_batch_count; ++i) {
                            _mm_storeu_si128(v210_dest++, black_batch);
                        }
                        continue;
                    }

                    const uint64_t src_y = y - config.dest_y + config.src_y;

                    if (src_y >= channel_format_desc.height) {
                        // Fill the row with black
                        auto black_batch_count = dest_line_bytes / sizeof(__m128i);
                        for (int i = 0; i < black_batch_count; ++i) {
                            _mm_storeu_si128(v210_dest++, black_batch);
                        }
                        continue;
                    }

                    auto src = reinterpret_cast<const ARGBPixel*>(frame.image_data(0).data()) +
                               (src_y * channel_format_desc.width + config.src_x);
                    // Pack pixels in batches of 48
                    for (int batch_index_x = 0; batch_index_x < fullspeed_x_batches; batch_index_x++) {
                        const __m256i* pixeldata = reinterpret_cast<const __m256i*>(src);

                        __m256i luma[6];
                        __m256i chroma[6];

                        __m256i zero = _mm256_setzero_si256();
                        for (int packet_index = 0; packet_index < 6; packet_index++) {
                            __m256i p0123 = _mm256_loadu_si256(pixeldata + packet_index * 2);
                            __m256i p4567 = _mm256_loadu_si256(pixeldata + packet_index * 2 + 1);

                            // shift down to 10 bit precision
                            p0123 = _mm256_srli_epi16(p0123, 6);
                            p4567 = _mm256_srli_epi16(p4567, 6);

                            // unpack 16 bit values to 32 bit registers, padding with zeros
                            __m256i pixel_pairs[4];
                            pixel_pairs[0] = _mm256_unpacklo_epi16(p0123, zero); // pixels 0 2
                            pixel_pairs[1] = _mm256_unpackhi_epi16(p0123, zero); // pixels 1 3
                            pixel_pairs[2] = _mm256_unpacklo_epi16(p4567, zero); // pixels 4 6
                            pixel_pairs[3] = _mm256_unpackhi_epi16(p4567, zero); // pixels 5 7

                            rgb_to_yuv_avx2(pixel_pairs, color_matrix, &luma[packet_index], &chroma[packet_index]);
                        }

                        pack_v210_avx2(luma, chroma, &v210_dest);

                        src += 48; // Move to the next batch of pixels
                    }

                    // Pack the final pixels one by one
                    if (rest_x_pixels > 0) {
                        auto dest = reinterpret_cast<uint8_t*>(v210_dest);
                        // clear the remainder of the row
                        auto rest_bytes =
                            reinterpret_cast<uint8_t*>(dest_row) + dest_line_bytes - reinterpret_cast<uint8_t*>(dest);
                        memset(dest, 0, rest_bytes);

                        int rest_x_6pixels = (rest_x_pixels / 6) * 6; // Round down to the nearest multiple of 6

                        // pack pixels
                        pack_v210(src, color_matrix, reinterpret_cast<uint32_t*>(v210_dest), rest_x_6pixels);
                        src += rest_x_6pixels;             // Move the source pointer forward
                        v210_dest += (rest_x_6pixels / 6); // one _m128i (32x4 bits) is 6 pixels

                        // // pad the rest with black pixels to fill the v210 packet
                        int last_x_pixels = rest_x_pixels - rest_x_6pixels;
                        if (last_x_pixels > 0) {
                            ARGBPixel pixels[6];
                            memset(pixels, 0, sizeof(pixels));
                            memcpy(pixels, src, last_x_pixels * 8);
                            pack_v210(pixels, color_matrix, reinterpret_cast<uint32_t*>(v210_dest), 6);
                            v210_dest++;
                            src += last_x_pixels;
                        }
                    }

                    // // Fill the rest of the row with black
                    auto bytes_written     = reinterpret_cast<uint8_t*>(v210_dest) - dest_row;
                    auto padding_bytes     = dest_line_bytes - bytes_written;
                    auto black_batch_count = padding_bytes / sizeof(black_batch);
                    for (int i = 0; i < black_batch_count; ++i) {
                        _mm_storeu_si128(v210_dest++, black_batch);
                    }
                }
            });
        }
    }

    std::shared_ptr<void> convert_frame_for_port(const core::video_format_desc& channel_format_desc,
                                                 const core::video_format_desc& decklink_format_desc,
                                                 const port_configuration&      config,
                                                 const core::const_frame&       frame1,
                                                 const core::const_frame&       frame2,
                                                 BMDFieldDominance              field_dominance)
    {
        std::shared_ptr<void> image_data = allocate_frame_data(decklink_format_desc);

        if (field_dominance != bmdProgressiveFrame) {
            convert_frame(channel_format_desc,
                          decklink_format_desc,
                          config,
                          image_data,
                          field_dominance == bmdUpperFieldFirst,
                          frame1);

            convert_frame(channel_format_desc,
                          decklink_format_desc,
                          config,
                          image_data,
                          field_dominance != bmdUpperFieldFirst,
                          frame2);

        } else {
            convert_frame(channel_format_desc, decklink_format_desc, config, image_data, true, frame1);
        }

        if (config.key_only) {
            // TODO: Add support for hdr frames
        }

        return image_data;
    }
};

hdr_v210_strategy::hdr_v210_strategy()
    : impl_(new impl())
{
}

hdr_v210_strategy::~hdr_v210_strategy() {}

BMDPixelFormat hdr_v210_strategy::get_pixel_format() { return impl_->get_pixel_format(); }
int            hdr_v210_strategy::get_row_bytes(int width) { return impl_->get_row_bytes(width); }

std::shared_ptr<void> hdr_v210_strategy::allocate_frame_data(const core::video_format_desc& format_desc)
{
    return impl_->allocate_frame_data(format_desc);
}
std::shared_ptr<void> hdr_v210_strategy::convert_frame_for_port(const core::video_format_desc& channel_format_desc,
                                                                const core::video_format_desc& decklink_format_desc,
                                                                const port_configuration&      config,
                                                                const core::const_frame&       frame1,
                                                                const core::const_frame&       frame2,
                                                                BMDFieldDominance              field_dominance)
{
    return impl_->convert_frame_for_port(
        channel_format_desc, decklink_format_desc, config, frame1, frame2, field_dominance);
}

}} // namespace caspar::decklink
