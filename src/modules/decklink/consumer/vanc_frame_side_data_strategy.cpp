/*
 * Copyright (c) 2025 Jacob Lifshay <programmerjake@gmail.com>
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
 */

/*
 * Partially based on FFmpeg libavfilter/ccfifo.c:
 *
 * CEA-708 Closed Captioning FIFO
 * Copyright (c) 2023 LTN Global Communications
 *
 * Author: Devin Heitmueller <dheitmueller@ltnglobal.com>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * Partially based on FFmpeg libavdevice/decklink_enc.cpp:
 *
 * Blackmagic DeckLink output
 * Copyright (c) 2013-2014 Ramiro Polla
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "common/assert.h"
#include "core/frame/frame_side_data.h"
#include "vanc.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

#include <boost/log/utility/manipulators/dump.hpp>

#ifdef DECKLINK_USE_LIBKLVANC
#include "vanc-eia_708b.h"
#endif

namespace caspar::decklink {

static const std::wstring a53_cc_name = L"ATSC A/53 Closed Captions";

#ifdef DECKLINK_USE_LIBKLVANC
struct libklvanc_error_t : virtual caspar_exception
{
};

using libklvanc_errn_info = boost::error_info<struct tag_libklvanc_errn_info, int>;

#define THROW_ON_ERROR_STR_(call) #call
#define THROW_ON_ERROR_STR(call) THROW_ON_ERROR_STR_(call)

#define LKV_RET_ERR(ret, func)                                                                                         \
    if (ret < 0) {                                                                                                     \
        CASPAR_THROW_EXCEPTION(libklvanc_error_t() << boost::errinfo_api_function(func) << libklvanc_errn_info(ret)    \
                                                   << boost::errinfo_errno(-(ret)));                                   \
    }

#define LKV_RET_FLAG(ret, func)                                                                                        \
    if (ret < 0) {                                                                                                     \
        CASPAR_THROW_EXCEPTION(libklvanc_error_t() << boost::errinfo_api_function(func));                              \
    }

#define LKV_HANDLE_ERR(call)                                                                                           \
    [&] {                                                                                                              \
        auto ret = call;                                                                                               \
        LKV_RET_ERR(ret, THROW_ON_ERROR_STR(call));                                                                    \
    }()

#define LKV_HANDLE_FLAG(call)                                                                                          \
    [&] {                                                                                                              \
        auto ret = call;                                                                                               \
        LKV_RET_FLAG(ret, THROW_ON_ERROR_STR(call));                                                                   \
    }()

static std::shared_ptr<klvanc_packet_eia_708b_s> create_eia708_cdp()
{
    klvanc_packet_eia_708b_s* retval = nullptr;
    LKV_HANDLE_ERR(klvanc_create_eia708_cdp(&retval));
    return std::shared_ptr<klvanc_packet_eia_708b_s>(retval,
                                                     [](klvanc_packet_eia_708b_s* p) { klvanc_destroy_eia708_cdp(p); });
}

class decklink_side_data_strategy_a53_cc final : public decklink_frame_side_data_vanc_strategy
{
    // name from https://en.wikipedia.org/wiki/CTA-708#Closed_Caption_Data_Packet_(cc_data_pkt)
    typedef std::array<std::uint8_t, 3> cc_data_pkt;
    static_assert(sizeof(cc_data_pkt) == cc_data_pkt{}.size(), "cc_data_pkt must not have any padding");

  public:
    static constexpr inline core::frame_side_data_type type = core::frame_side_data_type::a53_cc;
    struct known_framerate final
    {
        int                    framerate_num;
        int                    framerate_denom;
        std::uint8_t           total_cc_data_pkts_per_frame;
        std::uint8_t           eia_608_cc_data_pkts_per_frame;
        static known_framerate try_get(int num, int denom) noexcept
        {
            static constexpr known_framerate known_framerates[] = {
                // from Table 3 of https://pub.smpte.org/latest/st334-2/st0334-2-2015.pdf
                {24000, 1001, 25, 3}, // cdp_frame_rate 0001
                {24, 1, 25, 3},       // cdp_frame_rate 0010
                {25, 1, 24, 2},       // cdp_frame_rate 0011
                {30000, 1001, 20, 2}, // cdp_frame_rate 0100
                {30, 1, 20, 2},       // cdp_frame_rate 0101
                {50, 1, 12, 1},       // cdp_frame_rate 0110
                {60000, 1001, 10, 1}, // cdp_frame_rate 0111
                {60, 1, 10, 1},       // cdp_frame_rate 1000
            };
            static_assert(
                [] {
                    for (auto i : known_framerates) {
                        if (i.total_cc_data_pkts_per_frame > KLVANC_MAX_CC_COUNT)
                            throw "known_framerate total_cc_data_pkts_per_frame exceeds KLVANC_MAX_CC_COUNT";
                    }
                    return true;
                }(),
                "known_framerate total_cc_data_pkts_per_frame exceeds KLVANC_MAX_CC_COUNT");
            for (auto retval : known_framerates) {
                if (retval.framerate_num == num && retval.framerate_denom == denom) {
                    return retval;
                }
            }
            return {num, denom};
        }
    };

    explicit decklink_side_data_strategy_a53_cc(known_framerate known_framerate_)
        : decklink_frame_side_data_vanc_strategy(type)
        , known_framerate_(known_framerate_)
    {
        CASPAR_ENSURE(known_framerate_.total_cc_data_pkts_per_frame > known_framerate_.eia_608_cc_data_pkts_per_frame);
        CASPAR_ENSURE(known_framerate_.eia_608_cc_data_pkts_per_frame >= 1);
    }
    virtual bool        has_data() const override { return true; }
    virtual vanc_packet pop_packet(bool field2) override
    {
        if (field2)
            return {};
        auto _lock = std::unique_lock(mutex_);

        std::vector<cc_data_pkt> cc_data_pkts_for_frame;
        cc_data_pkts_for_frame.reserve(known_framerate_.total_cc_data_pkts_per_frame);
        for (std::size_t i = 0; i < known_framerate_.eia_608_cc_data_pkts_per_frame; i++) {
            if (eia_608_cc_data_pkts_.empty()) {
                std::uint8_t first = field2 ? 0xFD : 0xFC;
                cc_data_pkts_for_frame.push_back({first, 0x80, 0x80});
            } else {
                cc_data_pkts_for_frame.push_back(eia_608_cc_data_pkts_.front());
                eia_608_cc_data_pkts_.pop();
            }
        }
        while (cc_data_pkts_for_frame.size() < known_framerate_.total_cc_data_pkts_per_frame) {
            if (cta_708_cc_data_pkts_.empty()) {
                cc_data_pkts_for_frame.push_back({0xFA, 0x00, 0x00});
            } else {
                cc_data_pkts_for_frame.push_back(cta_708_cc_data_pkts_.front());
                cta_708_cc_data_pkts_.pop();
            }
        }
        auto eia708_cdp = create_eia708_cdp();
        // num/denom intentionally swapped since casparcg and ffmpeg uses the reciprocal of libklvanc
        LKV_HANDLE_FLAG(klvanc_set_framerate_EIA_708B(
            eia708_cdp.get(), known_framerate_.framerate_denom, known_framerate_.framerate_num));

        eia708_cdp->header.ccdata_present         = 1;
        eia708_cdp->header.caption_service_active = 1;
        eia708_cdp->ccdata.cc_count               = static_cast<std::uint8_t>(cc_data_pkts_for_frame.size());
        for (std::size_t i = 0; i < cc_data_pkts_for_frame.size(); i++) {
            if (cc_data_pkts_for_frame[i][0] & 0x04)
                eia708_cdp->ccdata.cc[i].cc_valid = 1;
            eia708_cdp->ccdata.cc[i].cc_type    = cc_data_pkts_for_frame[i][0] & 0x03;
            eia708_cdp->ccdata.cc[i].cc_data[0] = cc_data_pkts_for_frame[i][1];
            eia708_cdp->ccdata.cc[i].cc_data[1] = cc_data_pkts_for_frame[i][2];
        }

        // intentionally wrap around at 0xFFFF
        klvanc_finalize_EIA_708B(eia708_cdp.get(), sequence_number++);

        std::uint8_t* bytes_p    = nullptr;
        std::uint16_t byte_count = 0;

        LKV_HANDLE_ERR(klvanc_convert_EIA_708B_to_packetBytes(eia708_cdp.get(), &bytes_p, &byte_count));

        struct bytes_deleter final
        {
            void operator()(std::uint8_t* bytes) const noexcept { free(bytes); }
        };

        std::unique_ptr<std::uint8_t[], bytes_deleter> bytes(bytes_p);

        vanc_packet retval = {0x61, 0x01, 9, std::vector(bytes.get(), bytes.get() + byte_count)};
        CASPAR_LOG(trace) << L"decklink consumer: generated VANC packet from A53_CC side data: "
                          << boost::log::dump(retval.data.data(), retval.data.size(), 128);
        CASPAR_LOG(trace) << L"decklink consumer: eia-608 queue size = " << eia_608_cc_data_pkts_.size()
                          << " cta-708 queue size = " << cta_708_cc_data_pkts_.size();

        return retval;
    }
    virtual const std::wstring& get_name() const override { return a53_cc_name; }
    virtual void push_frame_side_data(const core::frame_side_data_in_queue& side_data, bool field2) override
    {
        if (!side_data.queue)
            return;

        auto _lock = std::unique_lock(mutex_);

        auto last_position = side_data.pos;
        if (last_frame_side_data_in_queue.queue == side_data.queue) {
            last_position = last_frame_side_data_in_queue.pos;
        }
        last_frame_side_data_in_queue = side_data;

        for (auto pos = last_position + 1; pos <= side_data.pos; pos++) {
            auto side_data_opt = side_data.queue->get(pos);
            if (!side_data_opt)
                continue;
            for (auto& side_data_ : *side_data_opt) {
                if (side_data_.type() != type)
                    continue;
                CASPAR_LOG(trace) << L"decklink consumer: got A53_CC side data: "
                                  << boost::log::dump(side_data_.data().data(), side_data_.data().size(), 16);
                auto& data = side_data_.data();
                // intentionally ignore any remainder to avoid out-of-bounds access
                std::size_t pkt_count = data.size() / cc_data_pkt{}.size();
                for (std::size_t i = 0; i < pkt_count; i++) {
                    cc_data_pkt pkt{};
                    std::memcpy(pkt.data(), &data[i * cc_data_pkt{}.size()], cc_data_pkt{}.size());

                    // TODO: is this correct? FFmpeg doesn't agree with Wikipedia:
                    // https://en.wikipedia.org/wiki/CTA-708#Closed_Caption_Data_Packet_(cc_data_pkt)

                    /* See ANSI/CTA-708-E Sec 4.3, Table 3 */
                    auto cc_valid = (pkt[0] & 0x04) >> 2;
                    auto cc_type  = pkt[0] & 0x03;
                    if (cc_type == 0x00 || cc_type == 0x01) {
                        if (pkt[1] == 0x80 && pkt[2] == 0x80) {
                            // ignore padding, since we end up with too much when reducing frame rate
                            continue;
                        }
                        eia_608_cc_data_pkts_.push(pkt);
                    } else if (cc_valid && (cc_type == 0x02 || cc_type == 0x03)) {
                        cta_708_cc_data_pkts_.push(pkt);
                    }
                }
            }
        }
    }

  private:
    mutable std::mutex             mutex_;
    std::queue<cc_data_pkt>        eia_608_cc_data_pkts_;
    std::queue<cc_data_pkt>        cta_708_cc_data_pkts_;
    const known_framerate          known_framerate_;
    std::uint16_t                  sequence_number = 0;
    core::frame_side_data_in_queue last_frame_side_data_in_queue{};
};
#endif

std::shared_ptr<decklink_frame_side_data_vanc_strategy>
decklink_frame_side_data_vanc_strategy::try_create(core::frame_side_data_type     type,
                                                   const core::video_format_desc& format)
{
    switch (type) {
        case core::frame_side_data_type::a53_cc:
#ifdef DECKLINK_USE_LIBKLVANC
            auto known_framerate = decklink_side_data_strategy_a53_cc::known_framerate::try_get(
                format.framerate.numerator(), format.framerate.denominator());
            if (known_framerate.total_cc_data_pkts_per_frame != 0) {
                return std::make_shared<decklink_side_data_strategy_a53_cc>(known_framerate);
            }
            CASPAR_LOG(error) << "decklink consumer: unknown framerate for " << a53_cc_name << ": "
                              << known_framerate.framerate_num << "/" << known_framerate.framerate_denom
                              << " -- disabling closed captions for output";
#else
            CASPAR_LOG(warning)
                << "decklink consumer: libklvanc is required for " << a53_cc_name
                << " but libklvanc was not compiled into casparcg -- disabling closed captions for output";
#endif
            return nullptr;
    }
    CASPAR_THROW_EXCEPTION(programming_error() << msg_info("unknown frame side-data type"));
}

} // namespace caspar::decklink