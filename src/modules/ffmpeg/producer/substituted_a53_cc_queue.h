#pragma once

#include "common/log.h"
#include "core/frame/frame_side_data.h"
#include "modules/ffmpeg/util/av_assert.h"

#include <boost/log/utility/manipulators/dump.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <utility>
#include <vector>

extern "C" {
#include <libavutil/frame.h>
}

namespace caspar { namespace ffmpeg {

/// Queue for A53 CC side data, needed to avoid EIA-608 and CEA-708 side data getting out of sync or split up in ffmpeg
/// filters.
/// ffmpeg likes to reformat A53 CC side data when it's passed through ffmpeg filters, so to help CasparCG properly
/// generate closed captions side data that's as close as possible to the input, this class is used to pass the
/// A53 CC side data around ffmpeg's filters so we can avoid unnecessarily changing the side data.
/// This works by replacing the A53 CC side data by a counter formatted to look like valid A53 CC side data in
/// `SubstitutedA53CCQueue::insert_and_replace_with_key`, and then the user can then run those `AVFrame`s through their
/// ffmpeg filter pipeline, and then finally use `SubstitutedA53CCQueue::extract_by_key` to substitute back in the
/// A53 CC side data at the correct frames (which may have been duplicated/deleted/replaced by ffmpeg's filters).
class SubstitutedA53CCQueue final
{
  public:
    void insert_and_replace_with_key(AVFrame* frame)
    {
        if (auto* side_data = av_frame_get_side_data(frame, AV_FRAME_DATA_A53_CC)) {
            auto buf = std::vector(side_data->data, side_data->data + side_data->size);

            auto pos = queue_.add_frame(
                std::vector{core::const_frame_side_data(core::frame_side_data_type::a53_cc, std::move(buf))});
            key k(pos);
            CASPAR_LOG(trace) << "ffmpeg producer: SubstitutedA53CCQueue::insert_and_replace_with_key: key: "
                              << boost::log::dump(k.data, sizeof(k.data))
                              << "  input: " << boost::log::dump(side_data->data, side_data->size, 128);
            av_frame_remove_side_data(frame, AV_FRAME_DATA_A53_CC);
            side_data = FFMEM(av_frame_new_side_data(frame, AV_FRAME_DATA_A53_CC, sizeof(k.data)));
            std::memcpy(side_data->data, k.data, sizeof(k.data));
        }
    }
    void extract_by_key(AVFrame* frame)
    {
        if (auto* side_data = av_frame_get_side_data(frame, AV_FRAME_DATA_A53_CC)) {
            CASPAR_LOG(trace) << "ffmpeg producer: SubstitutedA53CCQueue::extract_by_key: input: "
                              << boost::log::dump(side_data->data, side_data->size, 128);

            auto valid_position_range = queue_.valid_position_range();

            std::vector<std::uint8_t> side_data_from_queue;

            auto*       p    = side_data->data;
            std::size_t size = side_data->size;
            while (size > 0) {
                if (size >= A53_CC_CHUNK_SIZE) {
                    auto cc_valid = (p[0] & 0x04) >> 2;
                    auto cc_type  = p[0] & 0x03;
                    if (cc_type == 0x00 || cc_type == 0x01) {
                        if (p[1] == 0x80 && p[2] == 0x80) {
                            // skip EIA-608 padding
                            p += A53_CC_CHUNK_SIZE;
                            size -= A53_CC_CHUNK_SIZE;
                            continue;
                        }
                    } else if (!cc_valid) {
                        // skip CEA-708 padding
                        p += A53_CC_CHUNK_SIZE;
                        size -= A53_CC_CHUNK_SIZE;
                        continue;
                    }
                }
                key k;
                if (size >= sizeof(k.data)) {
                    std::memcpy(k.data, p, sizeof(k.data));
                    if (auto pos = k.pos(valid_position_range)) {
                        if (auto cur_side_data = queue_.get(*pos)) {
                            auto& cur_side_data_buf = (*cur_side_data)[0].data();
                            side_data_from_queue.insert(
                                side_data_from_queue.end(), cur_side_data_buf.begin(), cur_side_data_buf.end());
                        }
                        p += sizeof(k.data);
                        size -= sizeof(k.data);
                        continue;
                    }
                }
                if (!corrupted_.exchange(true, std::memory_order_relaxed)) {
                    CASPAR_LOG(error) << "ffmpeg producer: SubstitutedA53CCQueue: closed captions corrupted by ffmpeg, "
                                         "removing them.";
                }
                break;
            }
            av_frame_remove_side_data(frame, AV_FRAME_DATA_A53_CC);
            if (!side_data_from_queue.empty() && !corrupted_.load(std::memory_order_relaxed)) {
                side_data = FFMEM(av_frame_new_side_data(frame, AV_FRAME_DATA_A53_CC, side_data_from_queue.size()));
                std::memcpy(side_data->data, side_data_from_queue.data(), side_data_from_queue.size());
                CASPAR_LOG(trace) << "ffmpeg producer: SubstitutedA53CCQueue::extract_by_key: output: "
                                  << boost::log::dump(side_data->data, side_data->size, 128);
            }
        }
    }

  private:
    using position = core::frame_side_data_queue::position;

    static constexpr std::uint8_t to_hex_digit(std::uint8_t v)
    {
        v &= 0xF;
        if (v < 0xA)
            return v + '0';
        else
            return v - 0xA + 'A';
    }
    static constexpr std::optional<std::uint8_t> from_hex_digit(std::uint8_t v)
    {
        if (v >= '0' && v <= '9') {
            return v - '0';
        } else if (v >= 'A' && v <= 'F') {
            return v - 'A' + 0xA;
        } else {
            return std::nullopt;
        }
    }

    static inline constexpr std::size_t A53_CC_CHUNK_SIZE = 3;

    struct key final
    {
        // values less than 0x20 are the shift to get a hex digit from,
        // otherwise they are literal values.
        // small enough that it shouldn't be split up by ffmpeg's ccfifo
        static constexpr inline std::uint8_t data_template[][A53_CC_CHUNK_SIZE] = {
            {0xFF, 'C', 'C'},
            {0xFE, 'Q', ':'},
            {0xFE, 0x0C, 0x08},
            {0xFE, 0x04, 0x00},
        };

        std::uint8_t data[sizeof(data_template)];

        constexpr explicit key(position pos) noexcept
            : data{}
        {
            const std::uint8_t* tmpl = &data_template[0][0];
            for (std::size_t i = 0; i < sizeof(data_template) / sizeof(data_template[0][0]); i++) {
                if (tmpl[i] < 0x20) {
                    data[i] = to_hex_digit(pos >> tmpl[i]);
                } else {
                    data[i] = tmpl[i];
                }
            }
        }
        key() noexcept = default;
        constexpr std::optional<position> pos(std::pair<position, position> valid_position_range) const noexcept
        {
            position pos = valid_position_range.second;

            const std::uint8_t* tmpl = &data_template[0][0];
            for (std::size_t i = 0; i < sizeof(data_template) / sizeof(data_template[0][0]); i++) {
                if (tmpl[i] < 0x20) {
                    pos &= ~(static_cast<position>(0xF) << tmpl[i]);
                    if (auto digit = from_hex_digit(data[i])) {
                        pos |= static_cast<position>(*digit) << tmpl[i];
                    } else {
                        return std::nullopt;
                    }
                } else if (data[i] != tmpl[i]) {
                    return std::nullopt;
                }
            }
            // we store only the lower 16-bits in the key,
            // so reconstruct the upper 16 bits by checking which one is in range.
            for (position offset : {-1 << 16, 0, 1 << 16}) {
                position retval = pos + offset;
                if (retval >= valid_position_range.first && retval < valid_position_range.second)
                    return retval;
            }
            // none in range
            return std::nullopt;
        }
    };

    core::frame_side_data_queue queue_;
    std::atomic_bool            corrupted_ = false;
};

}} // namespace caspar::ffmpeg
