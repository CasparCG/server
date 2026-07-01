/*
 * Copyright (c) 2026 Sveriges Television AB <info@casparcg.com>
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
 * Original Author: Niklas Andersson, niklas@nxtedition.com
 * Moved to common on 26/6/26
 * @todo - option for v210 with straight alpha
 */

#if !defined(WIN32) && (defined(__x86_64__) || defined(__i386__))
// Force this file to compile with avx2, as it has been crafted with intrinsics that require it.
#pragma GCC target("avx2")
#endif

#ifdef USE_SIMDE
#define SIMDE_ENABLE_NATIVE_ALIASES
#include <simde/x86/avx2.h>
#endif

#include <type_traits>

#include "v210.h"

#include <common/log.h>
#include <common/memshfl.h>

#include <tbb/parallel_for.h>
#include <tbb/scalable_allocator.h>

namespace caspar { namespace v210 {

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

inline void rgb_to_yuv_avx2(__m256i                     pixel_pairs[4],
                            const std::vector<int32_t>& color_matrix,
                            __m256i*                    luma_out,
                            __m256i*                    chroma_out)
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
        *luma_out             = _mm256_srli_epi32(_mm256_add_epi32(y_sum01452367, y_offset),
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
        *chroma_out           = _mm256_srli_epi32(_mm256_add_epi32(cbcr_sum_0426, c_offset),
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
        chroma_16bit[i] = _mm256_permutevar8x32_epi32(cbcr16,
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

template <typename T = uint16_t>
struct ARGBPixel
{
    T R;
    T G;
    T B;
    T A;
};

template <typename T>
void pack_v210(const ARGBPixel<T>*         src,
               const std::vector<int32_t>& color_matrix,
               uint32_t*                   dest,
               int                         num_pixels,
               bool                        straight_alpha)
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
        uint32_t r, g, b, a;
        if constexpr (std::is_same<T, uint16_t>()) {
            r = src->R >> 6;
            g = src->G >> 6;
            b = src->B >> 6;
            a = src->A >> 6;
        } else if constexpr (std::is_same<T, uint8_t>()) {
            r = src->R << 2;
            g = src->G << 2;
            b = src->B << 2;
            a = src->A << 2;
        }

        // convert for straight alpha
        if (straight_alpha && a != 0) {
            int const factor = 1024 << 10;
            int const coeff  = factor / a;
            r                = (r * coeff) >> 10;
            g                = (g * coeff) >> 10;
            b                = (b * coeff) >> 10;
        }

        if (x % 2 == 0) {
            // Compute Cr
            uint32_t v = 1025 << 19;
            v += (int32_t)(color_matrix[6] * static_cast<int32_t>(r) + color_matrix[7] * static_cast<int32_t>(g) +
                           color_matrix[8] * static_cast<int32_t>(b));
            v >>= 20;
            write_v210(v);
        }

        // Compute Y
        uint32_t luma = 64 << 20;
        luma += (int32_t)(color_matrix[0] * static_cast<int32_t>(r) + color_matrix[1] * static_cast<int32_t>(g) +
                          color_matrix[2] * static_cast<int32_t>(b));
        luma >>= 20;
        write_v210(luma);

        if (x % 2 == 0) {
            // Compute Cb
            uint32_t u = 1025 << 19;
            u += (int32_t)(color_matrix[3] * static_cast<int32_t>(r) + color_matrix[4] * static_cast<int32_t>(g) +
                           color_matrix[5] * static_cast<int32_t>(b));
            u >>= 20;
            write_v210(u);
        }
    }
}

class v210_conv
    : public v210_output
    , std::enable_shared_from_this<v210_conv>
{
    std::vector<float> bt709{0.212639005871510,
                             0.715168678767756,
                             0.072192315360734,
                             -0.114592177555732,
                             -0.385407822444268,
                             0.5,
                             0.5,
                             -0.454155517037873,
                             -0.045844482962127};
    std::vector<float> bt2020{0.262700212011267,
                              0.677998071518871,
                              0.059301716469862,
                              -0.139630430187157,
                              -0.360369569812843,
                              0.5,
                              0.5,
                              -0.459784529009814,
                              -0.040215470990186};

    std::vector<int32_t> color_matrix;
    __m128i              black_batch;
    uint8_t              bpc;
    bool                 straight_alpha;

  public:
    explicit v210_conv(core::color_space color_space, uint8_t bpc, bool straight_alpha)
        : color_matrix(create_int_matrix(color_space == core::color_space::bt2020 ? bt2020 : bt709))
        , bpc(bpc)
        , straight_alpha(straight_alpha)
    {
        // setup black batch (6 pixels of black, encoded as v210)
        ARGBPixel<> black[6];
        memset(black, 0, sizeof(black));
        memset(&black_batch, 0, sizeof(__m128i));
        pack_v210(black, color_matrix, reinterpret_cast<uint32_t*>(&black_batch), 6, false);
    }

    void convert_frame(const core::video_format_desc& channel_format_desc,
                       const core::video_format_desc& output_format_desc,
                       const output_region&           region,
                       std::shared_ptr<void>          output,
                       const core::const_frame&       frame1,
                       const core::const_frame&       frame2,
                       uint8_t                        interlaced) override
    {
        if (interlaced != 0) {
            bpc == 1 ? do_convert_frame<uint8_t>(
                           channel_format_desc, output_format_desc, region, output, interlaced == 1, frame1)
                     : do_convert_frame<uint16_t>(
                           channel_format_desc, output_format_desc, region, output, interlaced == 1, frame1);

            bpc == 1 ? do_convert_frame<uint8_t>(
                           channel_format_desc, output_format_desc, region, output, interlaced == 2, frame2)
                     : do_convert_frame<uint16_t>(
                           channel_format_desc, output_format_desc, region, output, interlaced == 2, frame2);
        } else {
            bpc == 1
                ? do_convert_frame<uint8_t>(channel_format_desc, output_format_desc, region, output, true, frame1)
                : do_convert_frame<uint16_t>(channel_format_desc, output_format_desc, region, output, true, frame1);
        }
    }

  private:
    int get_row_bytes(int width) { return ((width + 47) / 48) * 128; }
    int get_row_bytes_alpha(int width) { return ((width + 2) / 3) * 4; }

    // Fill count 6-pixel groups with black
    inline void fill_black_groups(__m128i*& dest, int count) const
    {
        for (int i = 0; i < count; ++i) {
            _mm_storeu_si128(dest++, black_batch);
        }
    }

    // Fill count 24-pixel groups with transparency
    inline void fill_transparent_groups(uint32_t*& dest, int count) const
    {
        // 256bits is 32 bytes
        memset(dest, 0, count * 32);
        dest += 8;
    }

    // Convert 48 pixels using AVX2 SIMD
    template <typename T>
    inline void convert_48_pixels_avx2(const ARGBPixel<T>*& src, __m128i*& dest) const
    {
        const __m256i* pixeldata = reinterpret_cast<const __m256i*>(src);

        __m256i luma[6];
        __m256i chroma[6];
        __m256i zero = _mm256_setzero_si256();

        for (int packet_index = 0; packet_index < 6; packet_index++) {
            __m256i p0123, p4567;
            if constexpr (std::is_same<T, uint16_t>()) {
                p0123 = _mm256_loadu_si256(pixeldata + packet_index * 2);
                p4567 = _mm256_loadu_si256(pixeldata + packet_index * 2 + 1);

                p0123 = _mm256_srli_epi16(p0123, 6);
                p4567 = _mm256_srli_epi16(p4567, 6);
            } else if constexpr (std::is_same<T, uint8_t>()) {
                auto p01234567 = _mm256_loadu_si256(pixeldata + packet_index);
                auto p01452367 = _mm256_permute4x64_epi64(p01234567, 0b11011000);

                p0123 = _mm256_unpacklo_epi8(p01452367, zero);
                p4567 = _mm256_unpackhi_epi8(p01452367, zero);

                p0123 = _mm256_slli_epi16(p0123, 2);
                p4567 = _mm256_slli_epi16(p4567, 2);
            } else {
                static_assert(!std::is_same<T, T>(), "Unsupported template type for v210 conversion");
            }

            __m256i pixel_pairs[4];
            pixel_pairs[0] = _mm256_unpacklo_epi16(p0123, zero);
            pixel_pairs[1] = _mm256_unpackhi_epi16(p0123, zero);
            pixel_pairs[2] = _mm256_unpacklo_epi16(p4567, zero);
            pixel_pairs[3] = _mm256_unpackhi_epi16(p4567, zero);

            if (straight_alpha) {
                // convert to straight alpha
                int const factor = 1024 << 10;
                for (int i = 0; i < 4; i++) {
                    int const a1       = _mm256_extract_epi32(pixel_pairs[i], 3);
                    int const a2       = _mm256_extract_epi32(pixel_pairs[i], 7);
                    int const a1_coeff = a1 == 0 ? 1024 : factor / a1;
                    int const a2_coeff = a2 == 0 ? 1024 : factor / a2;

                    __m256i y_coeff = _mm256_set_epi32(
                        a1_coeff, a1_coeff, a1_coeff, a1_coeff, a2_coeff, a2_coeff, a2_coeff, a2_coeff);
                    __m256i res    = _mm256_mullo_epi32(pixel_pairs[i], y_coeff);
                    pixel_pairs[i] = _mm256_srli_epi32(res, 10);
                }
            }

            rgb_to_yuv_avx2(pixel_pairs, color_matrix, &luma[packet_index], &chroma[packet_index]);
        }

        pack_v210_avx2(luma, chroma, &dest);
        src += 48;
    }

    // Convert remaining pixels (less than 48) using scalar code
    template <typename T>
    inline void convert_remaining_pixels(const ARGBPixel<T>*& src, __m128i*& dest, int pixel_count) const
    {
        int full_6pixel_groups = pixel_count / 6;
        memset(dest, 0, sizeof(__m128i) * ((pixel_count + 5) / 6));

        if (full_6pixel_groups > 0) {
            int pixels_in_groups = full_6pixel_groups * 6;
            pack_v210(src, color_matrix, reinterpret_cast<uint32_t*>(dest), pixels_in_groups, straight_alpha);
            dest += full_6pixel_groups;
            src += pixels_in_groups;
            pixel_count -= pixels_in_groups;
        }

        // Handle final partial packet (pad with black)
        if (pixel_count > 0) {
            ARGBPixel<T> pixels[6];
            memset(pixels, 0, sizeof(pixels));
            memcpy(pixels, src, pixel_count * sizeof(ARGBPixel<T>));
            pack_v210(pixels, color_matrix, reinterpret_cast<uint32_t*>(dest), 6, straight_alpha);
            dest++;
            src += pixel_count;
        }
    }

    // Pack 48 pixels of alpha using AVX2 SIMD
    template <typename T>
    inline void pack_48_alpha_avx2(const ARGBPixel<T>*& src, __m256i*& dest) const
    {
        const __m256i* pixeldata = reinterpret_cast<const __m256i*>(src);

        if constexpr (std::is_same<T, uint16_t>()) {
            for (int j = 0; j < 2; j++) {
                auto result = _mm256_setzero_si256();
                for (int i = 0; i < 6; i++) {
                    auto padding = i > 2 ? 0 : 2;

                    auto group = _mm256_loadu_si256(pixeldata++);
                    auto alpha =
                        _mm256_slli_epi64(_mm256_srli_epi64(_mm256_slli_epi64(group, 6), 54), 2 + padding + i * 10);

                    result = _mm256_hadd_epi32(alpha, result);
                }

                // _mm256_store_si256(dest++, result);
                memcpy(dest, &result, 32);
                dest++;
            }
        } else if constexpr (std::is_same<T, uint8_t>()) {
            for (int j = 0; j < 2; j++) {
                auto result = _mm256_setzero_si256();
                for (int i = 0; i < 3; i++) {
                    auto group = _mm256_loadu_si256(pixeldata++);
                    auto alpha = _mm256_slli_epi32(_mm256_srli_epi32(group, 24), 2 + i * 10);

                    result = _mm256_add_epi32(alpha, result);
                }

                // _mm256_store_si256(dest++, result);
                memcpy(dest, &result, 32);
                dest++;
            }
        } else {
            static_assert(!std::is_same<T, T>(), "Unsupported template type for v210 conversion");
        }
    }

    // Convert remaining pixels (less than 48) using scalar code
    template <typename T>
    inline void pack_alpha(const ARGBPixel<T>* src, uint32_t*& dest, int pixel_count) const
    {
        auto write_alpha = [dest, index = 0, shift = 0](uint32_t val) mutable {
            dest[index] |= ((val & 0x3FF) << shift);

            shift += 10;
            if (shift >= 30) {
                index++;
                shift = 0;
            }
        };

        for (auto i = 0; i < pixel_count; i++) {
            uint32_t a;
            if constexpr (std::is_same<T, uint16_t>()) {
                a = (src + i)->A >> 6;
            } else if constexpr (std::is_same<T, uint8_t>()) {
                a = (src + i)->A << 2;
            }

            write_alpha(a);
        }

        dest += (pixel_count + 2) / 3;
    }

    template <typename T>
    void do_convert_frame(const core::video_format_desc& channel_format_desc,
                          const core::video_format_desc& output_format_desc,
                          const output_region&           region,
                          std::shared_ptr<void>&         image_data,
                          bool                           topField,
                          const core::const_frame&       frame)
    {
        if (!frame)
            return;

        int    firstLine                  = topField ? 0 : 1;
        size_t dest_line_bytes            = get_row_bytes(output_format_desc.width);
        size_t dest_alpha_line_bytes      = get_row_bytes_alpha(output_format_desc.width);
        int    black_groups_per_row       = static_cast<int>(dest_line_bytes / sizeof(__m128i));
        int    transparent_groups_per_row = static_cast<int>(dest_line_bytes / 32);

        // CASPAR_LOG(trace) << "[v210_conv] Line bytes=" << dest_line_bytes
        //                   << ", alpha line bytes=" << dest_alpha_line_bytes;

        // CASPAR_LOG(trace) << "[v210_conv] black_groups_per_row=" << black_groups_per_row
        //                   << ", transparent_groups_per_row=" << transparent_groups_per_row;

        // Calculate effective region dimensions
        int region_w = region.region_w > 0 ? region.region_w : channel_format_desc.width - region.src_x;
        int region_h = region.region_h > 0 ? region.region_h : channel_format_desc.height - region.src_y;

        // Clamp to available source pixels
        region_w = std::min(region_w, channel_format_desc.width - region.src_x);
        region_h = std::min(region_h, channel_format_desc.height - region.src_y);

        // Clamp to destination dimensions
        int pixels_to_copy = std::min(region_w, output_format_desc.width - region.dest_x);
        int lines_to_copy  = std::min(region_h, output_format_desc.height - region.dest_y);

        int max_y_content = region.dest_y + lines_to_copy;

        // Calculate dest_x alignment for v210 (6 pixels per group)
        int black_groups_start         = region.dest_x / 6;
        int partial_black_pixels       = region.dest_x - black_groups_start * 6;
        int transparent_groups_start   = region.dest_x / 24;
        int partial_transparent_pixels = region.dest_x - transparent_groups_start * 24;

        // CASPAR_LOG(trace) << "[v210_conv] black_groups_start= " << black_groups_start
        //                   << ", partial_black_pixels= " << partial_black_pixels
        //                   << ", transparent_groups_start= " << transparent_groups_start
        //                   << ", partial_transparent_pixels= " << partial_transparent_pixels;

        const int NUM_THREADS = 6;
        // const int NUM_THREADS     = 1;
        auto rows_per_thread = output_format_desc.height / NUM_THREADS;

        auto alpha_dest_plane =
            straight_alpha ? reinterpret_cast<uint8_t*>(image_data.get()) + output_format_desc.height * dest_line_bytes
                           : NULL;

        tbb::parallel_for(0, NUM_THREADS, [&](int thread_index) {
            auto start_y = firstLine + thread_index * rows_per_thread;
            auto end_y   = (thread_index + 1) * rows_per_thread;

            for (uint64_t y = start_y; y < end_y; y += output_format_desc.field_count) {
                auto     dest_row  = reinterpret_cast<uint8_t*>(image_data.get()) + y * dest_line_bytes;
                __m128i* v210_dest = reinterpret_cast<__m128i*>(dest_row);
                auto     alpha_dest =
                    straight_alpha ? reinterpret_cast<uint32_t*>(alpha_dest_plane + y * dest_alpha_line_bytes) : NULL;

                // Check if this row is outside the content region
                if (y < region.dest_y || y >= max_y_content) {
                    fill_black_groups(v210_dest, black_groups_per_row);
                    if (straight_alpha) {
                        // CASPAR_LOG(trace)
                        //     << "[v210_conv] y=" << y
                        //     << " fill_transparent_groups(outside content region)=" << transparent_groups_per_row;
                        fill_transparent_groups(alpha_dest, transparent_groups_per_row);
                    }
                    continue;
                }

                const uint64_t src_y = y - region.dest_y + region.src_y;

                // Fill the start of the row with black (complete 6-pixel groups)
                if (black_groups_start > 0) {
                    fill_black_groups(v210_dest, black_groups_start);
                    if (straight_alpha) {
                        // CASPAR_LOG(trace) << "[v210_conv] y=" << y
                        //                   << " fill_transparent_groups(fill start of row)=" <<
                        //                   transparent_groups_start;
                        fill_transparent_groups(alpha_dest, transparent_groups_start);
                    }
                }
                int content_pixels_written = 0;

                // Handle partial black group at start (if dest_x is not aligned to 6 pixels)
                if (partial_black_pixels > 0) {
                    ARGBPixel<T> pixels[6];
                    memset(pixels, 0, sizeof(pixels));
                    memset(v210_dest, 0, sizeof(__m128i));

                    int content_in_packet = std::min(6 - partial_black_pixels, pixels_to_copy);

                    auto src = reinterpret_cast<const ARGBPixel<T>*>(frame.image_data(0).data()) +
                               (src_y * channel_format_desc.width + region.src_x);

                    for (int i = 0; i < content_in_packet; ++i) {
                        pixels[partial_black_pixels + i] = src[i];
                    }

                    if (straight_alpha) {
                        // CASPAR_LOG(trace) << "[v210_conv] y=" << y << " pack_alpha(partial black at start)=" << 6;
                        pack_alpha(pixels, alpha_dest, 6);
                    }
                    pack_v210(pixels, color_matrix, reinterpret_cast<uint32_t*>(v210_dest), 6, straight_alpha);
                    v210_dest++;
                    content_pixels_written = content_in_packet;
                }

                // Pack the main content pixels
                int remaining_content = pixels_to_copy - content_pixels_written;
                if (remaining_content > 0) {
                    auto     src = reinterpret_cast<const ARGBPixel<T>*>(frame.image_data(0).data()) +
                                   (src_y * channel_format_desc.width + region.src_x + content_pixels_written);
                    __m256i* alpha_dest_avx2 = straight_alpha ? reinterpret_cast<__m256i*>(alpha_dest) : NULL;

                    // Process 48-pixel batches with AVX2
                    int fullspeed_batches = remaining_content / 48;

                    // if (straight_alpha) {
                    //     // CASPAR_LOG(trace) << "[v210_conv] y=" << y << " pack_alpha(main)=" << fullspeed_batches *
                    //     // 48;
                    //     pack_alpha(src, alpha_dest, fullspeed_batches * 48);
                    // }

                    for (int batch = 0; batch < fullspeed_batches; ++batch) {
                        if (straight_alpha) {
                            // CASPAR_LOG(trace) << "[v210_conv] y=" << y << " pack_48_alpha x=" << batch * 48;
                            pack_48_alpha_avx2(src, alpha_dest_avx2);
                            alpha_dest += 16;
                        }
                        convert_48_pixels_avx2(src, v210_dest);
                    }

                    // Process remaining content pixels (less than 48)
                    int rest_content = remaining_content - fullspeed_batches * 48;
                    if (rest_content > 0) {
                        if (straight_alpha) {
                            // CASPAR_LOG(trace) << "[v210_conv] y=" << y << " pack_alpha(rest_content)=" <<
                            // rest_content;
                            pack_alpha(src, alpha_dest, rest_content);
                        }
                        convert_remaining_pixels(src, v210_dest, rest_content);
                    }
                }

                // Fill the rest of the row with black
                auto bytes_written  = reinterpret_cast<uint8_t*>(v210_dest) - dest_row;
                auto padding_bytes  = dest_line_bytes - bytes_written;
                auto padding_groups = static_cast<int>(padding_bytes / sizeof(black_batch));
                fill_black_groups(v210_dest, padding_groups);
            }
        });
    }
};

spl::shared_ptr<v210_output> create_v210_output(core::color_space colorspace, uint8_t bpc, bool straight_alpha)
{
    return spl::make_shared<v210_conv>(colorspace, bpc, straight_alpha);
}

}} // namespace caspar::v210