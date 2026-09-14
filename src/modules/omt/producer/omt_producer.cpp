/*
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
#include "omt_producer.h"

#include "../util/omt_util.h"

#include <common/diagnostics/graph.h>
#include <common/log.h>
#include <common/os/thread.h>
#include <common/param.h>
#include <common/timer.h>
#include <common/utf.h>

#include <core/frame/draw_frame.h>
#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>
#include <core/producer/frame_producer.h>
#include <core/video_format.h>

#include <boost/algorithm/string.hpp>

#pragma warning(push)
#pragma warning(disable : 4244)
extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}
#include "../../ffmpeg/util/av_util.h"
#pragma warning(pop)

#include <algorithm>
#include <atomic>
#include <cstring>
#include <deque>
#include <mutex>
#include <queue>
#include <thread>

namespace caspar { namespace omt {

struct omt_producer : public core::frame_producer
{
    static std::atomic<int> instances_;
    const int                instance_no_;
    const std::wstring       name_;
    const std::string        name_utf8_;

    spl::shared_ptr<core::frame_factory> frame_factory_;
    core::video_format_desc              format_desc_;

    omt_lib*        lib_  = nullptr;
    omt_receive_t*  recv_ = nullptr;

    std::atomic<bool> running_{false};
    std::thread       worker_thread_;

    std::queue<core::draw_frame> frames_;
    mutable std::mutex           frames_mutex_;
    core::draw_frame             last_frame_;

    struct swr_deleter
    {
        void operator()(SwrContext* p) { swr_free(&p); }
    };
    std::unique_ptr<SwrContext, swr_deleter> swr_;
    int                                       recv_channels_    = 0;
    int                                       recv_sample_rate_ = 0;

    std::deque<int32_t> audio_queue_;
    std::mutex           audio_mutex_;
    int                  cadence_counter_ = 0;
    const int            cadence_length_;

    spl::shared_ptr<diagnostics::graph> graph_;
    caspar::timer                       frame_timer_;

  public:
    explicit omt_producer(spl::shared_ptr<core::frame_factory> frame_factory,
                          core::video_format_desc              format_desc,
                          std::wstring                          name)
        : instance_no_(instances_++)
        , name_(name)
        , name_utf8_(u8(name))
        , frame_factory_(frame_factory)
        , format_desc_(format_desc)
        , cadence_length_(static_cast<int>(format_desc.audio_cadence.size()))
    {
        lib_ = omt::load_library();

        graph_ = spl::make_shared<diagnostics::graph>();
        graph_->set_text(print());
        graph_->set_color("frame-time", diagnostics::color(0.5f, 1.0f, 0.2f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        diagnostics::register_graph(graph_);

        running_       = true;
        worker_thread_ = std::thread([this] { run(); });
    }

    ~omt_producer()
    {
        running_ = false;
        if (worker_thread_.joinable())
            worker_thread_.join();
    }

    std::wstring print() const override { return L"omt[" + std::to_wstring(instance_no_) + L"|" + name_ + L"]"; }

    std::wstring name() const override { return L"omt"; }

    void run()
    {
        set_thread_name(L"OMT-RECV: " + name_);

        {
            auto lock = omt::serialize_create_call();
            recv_     = lib_->receive_create(name_utf8_.c_str(),
                                          static_cast<OMTFrameType>(OMTFrameType_Video | OMTFrameType_Audio),
                                          OMTPreferredVideoFormat_BGRA,
                                          OMTReceiveFlags_None);
        }

        if (!recv_) {
            CASPAR_LOG(error) << print() << L" Failed to create OMT receiver.";
            return;
        }

        while (running_) {
            OMTMediaFrame* frame =
                lib_->receive(recv_, static_cast<OMTFrameType>(OMTFrameType_Video | OMTFrameType_Audio), 100);

            if (!frame)
                continue;

            if (frame->Type == OMTFrameType_Video) {
                handle_video(*frame);
            } else if (frame->Type == OMTFrameType_Audio) {
                handle_audio(*frame);
            }
        }

        {
            auto lock = omt::serialize_create_call();
            lib_->receive_destroy(recv_);
        }
        recv_ = nullptr;
    }

    void ensure_swr(int channels, int sample_rate)
    {
        if (swr_ && recv_channels_ == channels && recv_sample_rate_ == sample_rate)
            return;

        recv_channels_    = channels;
        recv_sample_rate_ = sample_rate;

        AVChannelLayout layout;
        av_channel_layout_default(&layout, channels);

        SwrContext* raw = nullptr;
        swr_alloc_set_opts2(&raw,
                            &layout,
                            AV_SAMPLE_FMT_S32,
                            format_desc_.audio_sample_rate,
                            &layout,
                            AV_SAMPLE_FMT_FLTP,
                            sample_rate,
                            0,
                            nullptr);
        swr_.reset(raw);
        swr_init(raw);
    }

    void handle_audio(const OMTMediaFrame& frame)
    {
        if (frame.Channels <= 0 || frame.SamplesPerChannel <= 0 || !frame.Data)
            return;

        ensure_swr(frame.Channels, frame.SampleRate);

        std::vector<const uint8_t*> in_planes(frame.Channels);
        for (int ch = 0; ch < frame.Channels; ++ch) {
            in_planes[ch] = reinterpret_cast<const uint8_t*>(frame.Data) +
                            static_cast<size_t>(ch) * frame.SamplesPerChannel * sizeof(float);
        }

        int64_t max_out = av_rescale_rnd(swr_get_delay(swr_.get(), recv_sample_rate_) + frame.SamplesPerChannel,
                                         format_desc_.audio_sample_rate,
                                         recv_sample_rate_,
                                         AV_ROUND_UP);
        if (max_out <= 0)
            return;

        std::vector<int32_t> out_buf(static_cast<size_t>(max_out) * frame.Channels);
        uint8_t*              out_ptr = reinterpret_cast<uint8_t*>(out_buf.data());

        int converted =
            swr_convert(swr_.get(), &out_ptr, static_cast<int>(max_out), in_planes.data(), frame.SamplesPerChannel);
        if (converted <= 0)
            return;

        std::lock_guard<std::mutex> lock(audio_mutex_);
        audio_queue_.insert(
            audio_queue_.end(), out_buf.begin(), out_buf.begin() + static_cast<size_t>(converted) * frame.Channels);

        // Keep at most ~2 seconds of audio buffered in case no video frames arrive to drain it.
        size_t max_queue = static_cast<size_t>(format_desc_.audio_sample_rate) * frame.Channels * 2;
        while (audio_queue_.size() > max_queue)
            audio_queue_.pop_front();
    }

    void handle_video(const OMTMediaFrame& frame)
    {
        if (frame.Width <= 0 || frame.Height <= 0 || !frame.Data)
            return;

        frame_timer_.restart();

        std::shared_ptr<AVFrame> video_frame(av_frame_alloc(), [](AVFrame* f) { av_frame_free(&f); });
        video_frame->format = AV_PIX_FMT_BGRA;
        video_frame->width  = frame.Width;
        video_frame->height = frame.Height;

        if (av_frame_get_buffer(video_frame.get(), 32) < 0)
            return;

        auto src = reinterpret_cast<const uint8_t*>(frame.Data);
        for (int y = 0; y < frame.Height; ++y) {
            std::memcpy(video_frame->data[0] + static_cast<size_t>(y) * video_frame->linesize[0],
                       src + static_cast<size_t>(y) * frame.Stride,
                       static_cast<size_t>(frame.Width) * 4);
        }

        int channels        = recv_channels_ > 0 ? recv_channels_ : format_desc_.audio_channels;
        int wanted_samples  = format_desc_.audio_cadence[cadence_counter_ % cadence_length_];
        cadence_counter_    = (cadence_counter_ + 1) % cadence_length_;

        std::vector<int32_t> audio_samples(static_cast<size_t>(wanted_samples) * channels, 0);
        {
            std::lock_guard<std::mutex> lock(audio_mutex_);
            int have = channels > 0 ? static_cast<int>(audio_queue_.size() / channels) : 0;
            int take = std::min(have, wanted_samples);
            for (int i = 0; i < take * channels; ++i) {
                audio_samples[i] = audio_queue_.front();
                audio_queue_.pop_front();
            }
        }

        std::shared_ptr<AVFrame> audio_frame(av_frame_alloc(), [](AVFrame* f) { av_frame_free(&f); });
        av_channel_layout_default(&audio_frame->ch_layout, channels);
        audio_frame->format      = AV_SAMPLE_FMT_S32;
        audio_frame->sample_rate = format_desc_.audio_sample_rate;
        audio_frame->nb_samples  = wanted_samples;

        if (wanted_samples > 0 && av_frame_get_buffer(audio_frame.get(), 32) == 0) {
            std::memcpy(audio_frame->data[0], audio_samples.data(), audio_samples.size() * sizeof(int32_t));
        }

        // Honor whatever alpha convention the sender declared (CasparCG's own compositing pipeline
        // expects premultiplied by default; make_frame's is_straight_alpha flags the exception).
        bool is_straight_alpha = (frame.Flags & OMTVideoFlags_Alpha) && !(frame.Flags & OMTVideoFlags_PreMultiplied);

        auto mframe = ffmpeg::make_frame(this,
                                         *frame_factory_,
                                         std::move(video_frame),
                                         std::move(audio_frame),
                                         core::color_space::unknown,
                                         core::frame_geometry::scale_mode::stretch,
                                         is_straight_alpha);
        auto dframe = core::draw_frame(std::move(mframe));

        {
            std::lock_guard<std::mutex> lock(frames_mutex_);
            frames_.push(dframe);
            while (frames_.size() > 3) {
                frames_.pop();
                graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
            }
        }

        graph_->set_value("frame-time", frame_timer_.elapsed() * format_desc_.fps * 0.5);
    }

    core::draw_frame receive_impl(const core::video_field /*field*/, int /*nb_samples*/) override
    {
        std::lock_guard<std::mutex> lock(frames_mutex_);
        if (!frames_.empty()) {
            last_frame_ = frames_.front();
            frames_.pop();
        }
        return last_frame_;
    }

    core::draw_frame last_frame(const core::video_field field) override
    {
        if (!last_frame_) {
            last_frame_ = receive_impl(field, 0);
        }
        return core::draw_frame::still(last_frame_);
    }

    bool is_ready() override
    {
        std::lock_guard<std::mutex> lock(frames_mutex_);
        return !frames_.empty() || static_cast<bool>(last_frame_);
    }

    core::monitor::state state() const override
    {
        core::monitor::state state;
        state["omt/name"] = u8(name_);
        return state;
    }
};

std::atomic<int> omt_producer::instances_(0);

spl::shared_ptr<core::frame_producer> create_omt_producer(const core::frame_producer_dependencies& dependencies,
                                                           const std::vector<std::wstring>&         params)
{
    if (params.empty())
        return core::frame_producer::empty();

    std::wstring name_or_address;
    bool         match = false;

    if (boost::iequals(params.at(0), L"[OMT]")) {
        match = true;
        if (params.size() > 1)
            name_or_address = params.at(1);
    } else if (boost::istarts_with(params.at(0), L"omt://")) {
        match           = true;
        name_or_address = params.at(0).substr(6);
    } else if (boost::iequals(params.at(0), L"OMT")) {
        match = true;
        if (params.size() > 1)
            name_or_address = params.at(1);
    }

    if (!match || name_or_address.empty())
        return core::frame_producer::empty();

    return spl::make_shared<omt_producer>(dependencies.frame_factory, dependencies.format_desc, name_or_address);
}

}} // namespace caspar::omt
