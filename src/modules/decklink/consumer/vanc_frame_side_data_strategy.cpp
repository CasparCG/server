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
#include "common/scope_exit.h"
#include "core/frame/frame_side_data.h"
#include "ffmpeg/util/av_assert.h"
#include "ffmpeg/util/av_util.h"
#include "vanc.h"
#include <boost/format.hpp>
#include <boost/log/utility/manipulators/dump.hpp>
#include <boost/rational.hpp>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <libavcodec/codec_id.h>
#include <libavcodec/packet.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavutil/rational.h>
#include <memory>
#include <mutex>
#include <string>

extern "C" {
#include <libavcodec/version.h>
}

#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(62, 10, 100)
#define DECKLINK_USE_FFMPEG_VANC 1
#endif

#ifdef DECKLINK_USE_FFMPEG_VANC
extern "C" {
#include <libavcodec/bsf.h>
#include <libavcodec/smpte_436m.h>
#include <libavutil/opt.h>
}
#endif

namespace caspar::decklink {

static const std::wstring a53_cc_name = L"ATSC A/53 Closed Captions";

#ifdef DECKLINK_USE_FFMPEG_VANC
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
        , packet_(ffmpeg::alloc_packet())
    {
        auto bsf = av_bsf_get_by_name("eia608_to_smpte436m");
        CASPAR_ENSURE(bsf);
        AVBSFContext* ctx = nullptr;
        FF(av_bsf_alloc(bsf, &ctx));
        eia608_to_smpte436m_context_ = std::shared_ptr<AVBSFContext>(ctx, [](AVBSFContext* ctx) { av_bsf_free(&ctx); });

        eia608_to_smpte436m_context_->time_base_in       = AVRational{frame_rate.numerator(), frame_rate.denominator()};
        eia608_to_smpte436m_context_->par_in->codec_type = AVMEDIA_TYPE_SUBTITLE;
        eia608_to_smpte436m_context_->par_in->codec_id   = AV_CODEC_ID_EIA_608;

        AVDictionary* options = nullptr;
        CASPAR_SCOPE_EXIT { av_dict_free(&options); };
        FF(av_dict_set_int(&options, "line_number", config.a53_cc_line, 0));
        FF(av_dict_set(&options, "wrapping_type", "vanc_frame", 0));
        FF(av_dict_set(&options, "sample_coding", "8bit_luma", 0));
        FF(av_dict_set_int(&options, "initial_cdp_sequence_cntr", config.a53_cc_initial_sequence_number, 0));
        FF(av_dict_set(
            &options,
            "cdp_frame_rate",
            (boost::format("%d/%d") % cdp_frame_rate_.numerator() % cdp_frame_rate_.denominator()).str().c_str(),
            0));
        FF(av_opt_set_dict(eia608_to_smpte436m_context_->priv_data, &options));

        FF(av_bsf_init(eia608_to_smpte436m_context_.get()));

        CASPAR_ENSURE(eia608_to_smpte436m_context_->par_out->codec_id == AV_CODEC_ID_SMPTE_436M_ANC);
    }
    virtual bool        has_data() const override { return true; }
    virtual vanc_packet pop_packet(bool field2) override
    {
        if (field2)
            return {};
        auto _lock = std::unique_lock(mutex_);

        auto cc_data    = a53_cc_queue_.lock().pop_frame();
        FF(av_new_packet(packet_.get(), cc_data.size()));
        memcpy(packet_->data, cc_data.data(), cc_data.size());
        int ret = av_bsf_send_packet(eia608_to_smpte436m_context_.get(), packet_.get());
        if (ret < 0) {
            av_packet_unref(packet_.get());
        }
        FF_RET(ret, "av_bsf_send_packet");
        std::vector<vanc_packet> retval_packets;
        // make sure to call av_bsf_receive_packet until it stops returning packets as required by ffmpeg
        for (;;) {
            ret = av_bsf_receive_packet(eia608_to_smpte436m_context_.get(), packet_.get());
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
                break;
            }
            FF_RET(ret, "av_bsf_receive_packet");
            CASPAR_SCOPE_EXIT { av_packet_unref(packet_.get()); };
            AVSmpte436mAncIterator iter;
            FF(av_smpte_436m_anc_iter_init(&iter, packet_->data, packet_->size));
            for (;;) {
                AVSmpte436mCodedAnc anc_436m;
                ret = av_smpte_436m_anc_iter_next(&iter, &anc_436m);
                if (ret == AVERROR_EOF) {
                    break;
                }
                FF_RET(ret, "av_smpte_436m_anc_iter_next");
                AVSmpte291mAnc8bit anc_291m;
                FF(av_smpte_291m_anc_8bit_decode(&anc_291m,
                                                 anc_436m.payload_sample_coding,
                                                 anc_436m.payload_sample_count,
                                                 anc_436m.payload,
                                                 nullptr));
                retval_packets.push_back(
                    {.did         = anc_291m.did,
                     .sdid        = anc_291m.sdid_or_dbn,
                     .line_number = line_number_,
                     .data        = std::vector(anc_291m.payload, anc_291m.payload + anc_291m.data_count)});
            }
        }
        CASPAR_ENSURE(retval_packets.size() == 1);
        auto retval = std::move(retval_packets[0]);
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
    std::shared_ptr<AVBSFContext>  eia608_to_smpte436m_context_;
    std::shared_ptr<AVPacket>      packet_;
};
#endif

std::shared_ptr<decklink_frame_side_data_vanc_strategy>
decklink_frame_side_data_vanc_strategy::try_create(core::frame_side_data_type     type,
                                                   const vanc_configuration&      config,
                                                   const core::video_format_desc& format)
{
    switch (type) {
        case core::frame_side_data_type::a53_cc:
#ifdef DECKLINK_USE_FFMPEG_VANC
            return std::make_shared<decklink_side_data_strategy_a53_cc>(format.framerate, config, format);
#else
            CASPAR_LOG(warning) << "decklink consumer: ffmpeg >= 8.0 is required for " << a53_cc_name
                                << " -- disabling closed captions for output";
#endif
            return nullptr;
    }
    CASPAR_THROW_EXCEPTION(programming_error() << msg_info("unknown frame side-data type"));
}

} // namespace caspar::decklink