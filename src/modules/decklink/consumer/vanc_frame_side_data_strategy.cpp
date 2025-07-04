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
#include <boost/rational.hpp>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iomanip>
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
struct libklvanc_error_t : public caspar_exception
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

struct closed_captions_dont_fit : public libklvanc_error_t
{
};

class decklink_side_data_strategy_a53_cc final : public decklink_frame_side_data_vanc_strategy
{
  public:
    static constexpr inline core::frame_side_data_type type = core::frame_side_data_type::a53_cc;

    explicit decklink_side_data_strategy_a53_cc(boost::rational<int>           frame_rate,
                                                const vanc_configuration&      config,
                                                const core::video_format_desc& format)
        : decklink_frame_side_data_vanc_strategy(type)
        , a53_cc_queue_(frame_rate, format.field_count != 1)
        , line_number_(config.a53_cc_line)
        , cdp_frame_rate_(config.a53_cc_cdp_frame_rate == 0 ? frame_rate : config.a53_cc_cdp_frame_rate)
        , sequence_number_(config.a53_cc_initial_sequence_number)
    {
        if (a53_cc_queue_.max_frame_size() / core::a53_cc_queue::cc_data_pkt{}.size() > KLVANC_MAX_CC_COUNT) {
            CASPAR_THROW_EXCEPTION(closed_captions_dont_fit()
                                   << msg_info(u8(a53_cc_name) + " don't fit in libklvanc's structures"));
        }
    }
    virtual bool        has_data() const override { return true; }
    virtual vanc_packet pop_packet(bool field2) override
    {
        if (field2)
            return {};
        auto _lock = std::unique_lock(mutex_);

        auto cc_data    = a53_cc_queue_.lock().pop_frame();
        auto eia708_cdp = create_eia708_cdp();
        // num/denom intentionally swapped since casparcg and ffmpeg uses the reciprocal of libklvanc
        LKV_HANDLE_FLAG(klvanc_set_framerate_EIA_708B(
            eia708_cdp.get(), cdp_frame_rate_.denominator(), cdp_frame_rate_.numerator()));

        eia708_cdp->header.ccdata_present         = 1;
        eia708_cdp->header.caption_service_active = 1;
        eia708_cdp->ccdata.cc_count =
            static_cast<std::uint8_t>(cc_data.size() / core::a53_cc_queue::cc_data_pkt{}.size());
        for (std::size_t i = 0; i < eia708_cdp->ccdata.cc_count; i++) {
            std::size_t start = i * core::a53_cc_queue::cc_data_pkt{}.size();
            if (cc_data[start] & 0x04)
                eia708_cdp->ccdata.cc[i].cc_valid = 1;
            eia708_cdp->ccdata.cc[i].cc_type    = cc_data[start] & 0x03;
            eia708_cdp->ccdata.cc[i].cc_data[0] = cc_data[start + 1];
            eia708_cdp->ccdata.cc[i].cc_data[1] = cc_data[start + 2];
        }

        // intentionally wrap around at 0xFFFF
        klvanc_finalize_EIA_708B(eia708_cdp.get(), sequence_number_++);

        std::uint8_t* bytes_p    = nullptr;
        std::uint16_t byte_count = 0;

        LKV_HANDLE_ERR(klvanc_convert_EIA_708B_to_packetBytes(eia708_cdp.get(), &bytes_p, &byte_count));

        struct bytes_deleter final
        {
            void operator()(std::uint8_t* bytes) const noexcept { free(bytes); }
        };

        std::unique_ptr<std::uint8_t[], bytes_deleter> bytes(bytes_p);

        vanc_packet retval = {0x61, 0x01, line_number_, std::vector(bytes.get(), bytes.get() + byte_count)};
        // format starting from `Line:` matches decklink `VancCapture` example, making it easier to compare
        CASPAR_LOG(trace) << L"decklink consumer: generated VANC packet from A53_CC side data: Line "
                          << retval.line_number << L":   DID: " << std::hex << std::setfill(L'0') << std::setw(2)
                          << static_cast<unsigned>(retval.did) << L"; SDID: " << std::setw(2)
                          << static_cast<unsigned>(retval.sdid) << L"; Data: "
                          << boost::log::dump(retval.data.data(), retval.data.size(), 128);

        return retval;
    }
    virtual const std::wstring& get_name() const override { return a53_cc_name; }
    virtual void                push_frame_side_data(const core::frame_side_data_in_queue& field1_side_data,
                                                     const core::frame_side_data_in_queue& field2_side_data) override
    {
        if (!field1_side_data.queue && !field2_side_data.queue)
            return;

        auto _lock               = std::unique_lock(mutex_);
        auto locked_a53_cc_queue = a53_cc_queue_.lock();

        if (field1_side_data.queue == field2_side_data.queue) {
            push_fields_side_data_locked(
                std::max(field1_side_data,
                         field1_side_data,
                         [](const core::frame_side_data_in_queue& a, const core::frame_side_data_in_queue& b) {
                             return a.pos < b.pos;
                         }),
                locked_a53_cc_queue);
        } else {
            push_fields_side_data_locked(field1_side_data, locked_a53_cc_queue);
            push_fields_side_data_locked(field2_side_data, locked_a53_cc_queue);
        }
    }

  private:
    void push_fields_side_data_locked(const core::frame_side_data_in_queue& side_data,
                                      core::a53_cc_queue::locked&           locked_a53_cc_queue)
    {
        auto [start, end] = side_data.position_range_since_last(last_frame_side_data_in_queue_);
        for (auto pos = start; pos < end; pos++) {
            auto side_data_opt = side_data.queue->get(pos);
            if (!side_data_opt)
                continue;
            for (auto& side_data_ : *side_data_opt) {
                if (side_data_.type() != type)
                    continue;
                CASPAR_LOG(trace) << L"decklink consumer: got A53_CC side data: "
                                  << boost::log::dump(side_data_.data().data(), side_data_.data().size(), 16);
                locked_a53_cc_queue.push_field(side_data_.data());
            }
        }
    }

  private:
    std::mutex                     mutex_;
    core::a53_cc_queue             a53_cc_queue_;
    const std::uint8_t             line_number_;
    const boost::rational<int>     cdp_frame_rate_;
    std::uint16_t                  sequence_number_;
    core::frame_side_data_in_queue last_frame_side_data_in_queue_{};
};
#endif

std::shared_ptr<decklink_frame_side_data_vanc_strategy>
decklink_frame_side_data_vanc_strategy::try_create(core::frame_side_data_type     type,
                                                   const vanc_configuration&      config,
                                                   const core::video_format_desc& format)
{
    switch (type) {
        case core::frame_side_data_type::a53_cc:
#ifdef DECKLINK_USE_LIBKLVANC
            try {
                return std::make_shared<decklink_side_data_strategy_a53_cc>(format.framerate, config, format);
            } catch (closed_captions_dont_fit&) {
                CASPAR_LOG(error) << "decklink consumer: frame-rate is too weird for " << a53_cc_name
                                  << ", the closed captions don't fit in libklvanc's structures: " << format.framerate
                                  << " -- disabling closed captions for output";
            }
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