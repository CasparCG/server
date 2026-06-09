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
 * Author: Mint de Wit mint@araminta.dev
 * Pretty verbatim copy of the decklink v210 implementation
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

#include "../StdAfx.h"

#include <tbb/scalable_allocator.h>

namespace caspar { namespace dmf_mxl {

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
                                                      offsets); // cbcr0 cbcr2 cbcr4 cbcr6   cbcr8 cbcr10 cbcr12
        // cbcr14
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
void pack_v210(const ARGBPixel<T>* src, const std::vector<int32_t>& color_matrix, uint32_t* dest, int num_pixels)
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
        uint32_t r, g, b;
        if constexpr (std::is_same<T, uint16_t>()) {
            r = src->R >> 6;
            g = src->G >> 6;
            b = src->B >> 6;
        } else if constexpr (std::is_same<T, uint8_t>()) {
            r = src->R << 2;
            g = src->G << 2;
            b = src->B << 2;
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

template <typename T>
void row_to_v210(ARGBPixel<T> const* src, int pixel_count, const std::vector<int32_t>& color_matrix, __m128i* dest)
{
    int fullspeed_batches = pixel_count / 48;
    for (int batch = 0; batch < fullspeed_batches; ++batch) {
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

            rgb_to_yuv_avx2(pixel_pairs, color_matrix, &luma[packet_index], &chroma[packet_index]);
        }

        pack_v210_avx2(luma, chroma, &dest);
        src += 48;
        pixel_count -= 48;
    }

    int full_6pixel_groups = pixel_count / 6;
    memset(dest, 0, sizeof(__m128i) * ((pixel_count + 5) / 6));

    if (full_6pixel_groups > 0) {
        int pixels_in_groups = full_6pixel_groups * 6;
        pack_v210(src, color_matrix, reinterpret_cast<uint32_t*>(dest), pixels_in_groups);
        dest += full_6pixel_groups;
        src += pixels_in_groups;
        pixel_count -= pixels_in_groups;
    }

    // Handle final partial packet (pad with black)
    if (pixel_count > 0) {
        ARGBPixel<T> pixels[6];
        memset(pixels, 0, sizeof(pixels));
        memcpy(pixels, src, pixel_count * sizeof(ARGBPixel<T>));
        pack_v210(pixels, color_matrix, reinterpret_cast<uint32_t*>(dest), 6);
        dest++;
        src += pixel_count;
    }
};

void do_row_to_v210(ARGBPixel<uint8_t> const*   src,
                    int                         pixel_count,
                    const std::vector<int32_t>& color_matrix,
                    __m128i*                    dest)
{
    return row_to_v210(src, pixel_count, color_matrix, dest);
}
void do_row_to_v210(ARGBPixel<uint16_t> const*  src,
                    int                         pixel_count,
                    const std::vector<int32_t>& color_matrix,
                    __m128i*                    dest)
{
    return row_to_v210(src, pixel_count, color_matrix, dest);
}

}} // namespace caspar::dmf_mxl