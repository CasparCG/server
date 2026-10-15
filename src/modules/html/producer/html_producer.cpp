/*
 * Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include "html_producer.h"

#include <core/video_format.h>

#include <core/frame/draw_frame.h>
#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/frame_transform.h>
#include <core/frame/geometry.h>
#include <core/frame/pixel_format.h>
#include <core/monitor/monitor.h>
#include <core/producer/frame_producer.h>

#include <common/assert.h>
#include <common/diagnostics/graph.h>
#include <common/env.h>
#include <common/future.h>
#include <common/os/filesystem.h>
#include <common/timer.h>

#if defined(__GNUC__) && __GNUC__ == 14
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/regex.hpp>
#if defined(__GNUC__) && __GNUC__ == 14
#pragma GCC diagnostic pop
#endif

#include <tbb/concurrent_queue.h>
#include <tbb/parallel_for.h>

#include <mutex>

#include <include/cef_app.h>
#include <include/cef_client.h>
#include <include/cef_render_handler.h>

#include <chrono>
#include <deque>
#include <optional>
#include <queue>
#include <utility>

#include <ffmpeg/util/audio_resampler.h>

#include "../html.h"
#include "../util.h"

#ifdef WIN32
#include <accelerator/d3d/d3d_device.h>
#include <accelerator/d3d/d3d_device_context.h>
#include <accelerator/d3d/d3d_texture2d.h>
#endif

namespace caspar { namespace html {

// steady_clock, not system_clock/high_resolution_clock: CEF's audio pts is on Chromium's internal
// monotonic clock (base::TimeTicks), not the Unix epoch its own header documents - the two are
// comparable only via an explicitly monotonic clock on our side, calibrated once against pts
// (see OnAudioStreamPacket).
inline std::int64_t steady_now_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

// Matches ffmpeg::AudioResampler's fixed AV_CHANNEL_LAYOUT_HEXADECAGONAL output.
constexpr size_t kAudioChannels = 16;

// How many CEF audio packets' worth of backlog to allow before dropping the oldest.
constexpr size_t kAudioBacklogPackets = 4;

class html_client
    : public CefClient
    , public CefRenderHandler
    , public CefAudioHandler
    , public CefLifeSpanHandler
    , public CefLoadHandler
    , public CefDisplayHandler
{
    std::wstring                        url_;
    spl::shared_ptr<diagnostics::graph> graph_;
    core::monitor::state                state_;
    mutable std::mutex                  state_mutex_;
    caspar::timer                       tick_timer_;
    caspar::timer                       frame_timer_;
    caspar::timer                       paint_timer_;
    caspar::timer                       test_timer_;

    spl::shared_ptr<core::frame_factory> frame_factory_;
    core::video_format_desc              format_desc_;
    bool                                 gpu_enabled_;
    bool                                 shared_texture_enable_;
    tbb::concurrent_queue<std::wstring>  javascript_before_load_;
    std::atomic<bool>                    loaded_;
    std::atomic<bool>                    not_found_;
    std::atomic<bool>                    closing_;

    std::unique_ptr<ffmpeg::AudioResampler> audioResampler_;

    // Video and audio arrive on separate CEF threads (OnPaint on the UI thread, OnAudioStreamPacket
    // on CEF's own audio thread) with no shared timeline of their own, so - mirroring how
    // decklink_producer/av_producer bundle their separately-arriving video+audio into one atomic
    // unit before it ever reaches a bounded buffer - every pairing attempt below (from either
    // thread) combines whatever audio has become due with the latest video image into a single
    // draw_frame, under pairing_mutex_, before it's pushed to buffer_. This guarantees video and
    // audio are never split apart, and the fixed-capacity buffer_ (drop-oldest on overflow, exactly
    // like decklink_producer's buffer_capacity_) gives deterministic, bounded producer latency
    // instead of today's variable "whatever's queued" latency.
    mutable std::mutex      pairing_mutex_;
    core::draw_frame        latest_video_frame_;
    bool                    has_video_frame_ = false;
    std::int64_t            last_pair_time_ms_ = 0;

    // CEF's audio pts is documented as "milliseconds since the Unix Epoch" but is actually measured
    // on Chromium's internal monotonic clock (base::TimeTicks, an arbitrary reference) - see
    // OnAudioStreamPacket for the one-time calibration this requires before pts is comparable to
    // steady_now_ms(). audio_buffer_ is a continuous PCM backlog (not discrete CEF packets), so a
    // pairing attempt can slice out exactly the audio whose real timestamp precedes "now", rather
    // than being quantized to whole CEF packets.
    std::vector<std::int32_t> audio_buffer_;
    bool                      audio_clock_calibrated_ = false;
    std::int64_t              audio_clock_offset_ms_  = 0; // translates CEF pts -> steady_now_ms()
    double                    audio_buffer_start_ms_   = 0; // steady-clock timestamp of the oldest buffered sample

    const size_t             buffer_capacity_ = 4;
    struct paired_frame
    {
        core::draw_frame frame;
        std::int64_t     timestamp_ms;
    };
    std::deque<paired_frame>     buffer_;
    mutable std::mutex           buffer_mutex_;
    core::draw_frame             last_frame_;
    std::int64_t                 last_pop_timestamp_ms_ = 0;

    CefRefPtr<CefBrowser> browser_;

#ifdef WIN32
    std::shared_ptr<accelerator::d3d::d3d_device> const d3d_device_;
    std::shared_ptr<accelerator::d3d::d3d_texture2d>    d3d_shared_buffer_;
#endif

  public:
    html_client(spl::shared_ptr<core::frame_factory>       frame_factory,
                const spl::shared_ptr<diagnostics::graph>& graph,
                core::video_format_desc                    format_desc,
                bool                                       gpu_enabled,
                bool                                       shared_texture_enable,
                std::wstring                               url)
        : url_(std::move(url))
        , graph_(graph)
        , frame_factory_(std::move(frame_factory))
        , format_desc_(std::move(format_desc))
        , gpu_enabled_(gpu_enabled)
        , shared_texture_enable_(shared_texture_enable)
#ifdef WIN32
        , d3d_device_(accelerator::d3d::d3d_device::get_device())
#endif
    {
        graph_->set_color("browser-tick-time", diagnostics::color(0.1f, 1.0f, 0.1f));
        graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        graph_->set_color("late-frame", diagnostics::color(0.6f, 0.1f, 0.1f));
        graph_->set_color("overload", diagnostics::color(0.6f, 0.6f, 0.3f));
        graph_->set_color("buffered-frames", diagnostics::color(0.2f, 0.9f, 0.9f));
        graph_->set_text(print());
        diagnostics::register_graph(graph_);

        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            state_["file/path"] = u8(url_);
        }

        loaded_    = false;
        not_found_ = false;
        closing_   = false;
    }

    void reload()
    {
        html::begin_invoke([this] {
            if (browser_ != nullptr)
                browser_->Reload();
        });
    }

    void close()
    {
        closing_ = true;

        html::invoke([this] {
            if (browser_ != nullptr) {
                browser_->GetHost()->CloseBrowser(true);
            }
        });
    }

    bool try_pop(const core::video_field field)
    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);

        if (buffer_.empty()) {
            return false;
        }

        /*
         * CEF in gpu-enabled mode only sends frames when something changes, and interlaced channels
         * consume two frames in a short time span.
         * This can interact poorly and cause the second
         * field of an animation repeat the first.
         * If there is a single paired frame in the buffer, it may
         * want delaying to avoid this stutter.
         * The hazard here is that sometimes animations will
         * start a field later than intended.
         */
        if (field == core::video_field::a && buffer_.size() == 1) {
            auto now_time = steady_now_ms();

            // Make sure there has been a gap before this pop, of at least a couple of frames
            auto follows_gap_in_frames = (now_time - last_pop_timestamp_ms_) > 100;

            // Check if the sole buffered frame is too young to have a partner field generated (with a tolerance)
            auto time_per_frame           = (1000 * 1.5) / format_desc_.fps;
            auto front_frame_is_too_young = (now_time - buffer_.front().timestamp_ms) < time_per_frame;

            if (follows_gap_in_frames && front_frame_is_too_young) {
                return false;
            }
        }

        last_pop_timestamp_ms_ = buffer_.front().timestamp_ms;
        last_frame_            = std::move(buffer_.front().frame);
        buffer_.pop_front();

        graph_->set_value("buffered-frames", (double)buffer_.size() / buffer_capacity_);

        return true;
    }

    core::draw_frame receive(const core::video_field field)
    {
        if (!try_pop(field)) {
            graph_->set_tag(diagnostics::tag_severity::SILENT, "late-frame");
            return core::draw_frame::still(last_frame_);
        } else {
            return last_frame_;
        }
    }

    core::draw_frame last_frame() const { return core::draw_frame::still(last_frame_); }

    bool is_ready() const
    {
        std::lock_guard<std::mutex> lock(buffer_mutex_);
        return !buffer_.empty() || last_frame_;
    }

    // Must be called with pairing_mutex_ held. Combines the current video image with whatever
    // audio has become due (by real elapsed time) into one atomic unit, then pushes it to buffer_
    // - mirroring decklink_producer's "bundle video+audio before it ever reaches the buffer, drop
    // whole units on overflow" pattern. Called from both OnPaint (video changed) and
    // OnAudioStreamPacket (audio arrived), so audio keeps flowing even while the page is visually
    // static; audio-triggered calls are throttled to roughly tick-rate so a fast-changing page
    // doesn't get its audio pre-sliced into many tiny buffer_ entries.
    void try_pair_and_enqueue_locked(bool from_audio)
    {
        if (!has_video_frame_) {
            return;
        }

        auto now_ms = steady_now_ms();

        const auto min_interval_ms = static_cast<std::int64_t>(500.0 / format_desc_.fps);
        if (from_audio && last_pair_time_ms_ != 0 && (now_ms - last_pair_time_ms_) < min_interval_ms) {
            return;
        }

        core::draw_frame audio_slice;
        if (audio_clock_calibrated_) {
            auto due_ms = now_ms - audio_buffer_start_ms_;
            if (due_ms > 0.0) {
                auto available_samples = audio_buffer_.size() / kAudioChannels;
                auto wanted_samples    = static_cast<size_t>(due_ms * format_desc_.audio_sample_rate / 1000.0);
                auto take_samples      = std::min(available_samples, wanted_samples);
                if (take_samples > 0) {
                    auto                       take_elements = take_samples * kAudioChannels;
                    std::vector<std::int32_t> taken(audio_buffer_.begin(), audio_buffer_.begin() + take_elements);
                    audio_buffer_.erase(audio_buffer_.begin(), audio_buffer_.begin() + take_elements);
                    audio_buffer_start_ms_ += take_samples * 1000.0 / format_desc_.audio_sample_rate;

                    audio_slice = core::draw_frame(core::mutable_frame(
                        this, {}, array<std::int32_t>(std::move(taken)), core::pixel_format_desc()));
                }
            }
        }

        // Skip audio-only "nothing happened" pairings; a genuine video change always gets its own
        // entry (CEF only calls OnPaint on real changes).
        if (from_audio && !audio_slice) {
            return;
        }

        auto paired = core::draw_frame::over(latest_video_frame_, audio_slice);

        {
            std::lock_guard<std::mutex> buf_lock(buffer_mutex_);
            buffer_.push_back({std::move(paired), now_ms});
            if (buffer_.size() > buffer_capacity_) {
                buffer_.pop_front();
                graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
            }
            graph_->set_value("buffered-frames", (double)buffer_.size() / buffer_capacity_);
        }

        last_pair_time_ms_ = now_ms;
    }

    void execute_javascript(const std::wstring& javascript)
    {
        if (!loaded_) {
            javascript_before_load_.push(javascript);
        } else {
            execute_queued_javascript();
            do_execute_javascript(javascript);
        }
    }

    bool OnBeforePopup(CefRefPtr<CefBrowser>          browser,
                       CefRefPtr<CefFrame>            frame,
                       int                            popup_id,
                       const CefString&               target_url,
                       const CefString&               target_frame_name,
                       WindowOpenDisposition          target_disposition,
                       bool                           user_gesture,
                       const CefPopupFeatures&        popupFeatures,
                       CefWindowInfo&                 windowInfo,
                       CefRefPtr<CefClient>&          client,
                       CefBrowserSettings&            settings,
                       CefRefPtr<CefDictionaryValue>& dict,
                       bool*                          no_javascript_access) override
    {
        // This blocks popup windows from opening, as they dont make sense and hit an exception in get_browser_host upon
        // closing
        return true;
    }

    CefRefPtr<CefBrowserHost> get_browser_host() const
    {
        if (browser_ != nullptr)
            return browser_->GetHost();
        return nullptr;
    }

    core::monitor::state state() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return state_;
    }

  private:
    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override
    {
        CASPAR_ASSERT(CefCurrentlyOn(TID_UI));

        rect = CefRect(0, 0, format_desc_.square_width, format_desc_.square_height);
    }

    void OnPaint(CefRefPtr<CefBrowser> browser,
                 PaintElementType      type,
                 const RectList&       dirtyRects,
                 const void*           buffer,
                 int                   width,
                 int                   height) override
    {
        if (shared_texture_enable_ || closing_ || not_found_)
            return;

        graph_->set_value("browser-tick-time", paint_timer_.elapsed() * format_desc_.fps * 0.5);
        paint_timer_.restart();
        CASPAR_ASSERT(CefCurrentlyOn(TID_UI));

        if (type != PET_VIEW)
            return;

        core::pixel_format_desc pixel_desc(core::pixel_format::bgra);
        pixel_desc.planes.emplace_back(width, height, 4);

        core::mutable_frame frame = frame_factory_->create_frame(this, pixel_desc);
        char*               src   = (char*)buffer;
        char*               dst   = reinterpret_cast<char*>(frame.image_data(0).begin());
        test_timer_.restart();

#ifdef WIN32
        if (gpu_enabled_) {
            int chunksize = height * width;
            tbb::parallel_for(0, 4, [&](int y) { std::memcpy(dst + y * chunksize, src + y * chunksize, chunksize); });
        } else {
            std::memcpy(dst, src, width * height * 4);
        }
#else
        // On my one test linux machine, doing a single memcpy doesn't have the same cost as windows,
        // making using tbb excessive
        std::memcpy(dst, src, width * height * 4);
#endif

        graph_->set_value("memcpy", test_timer_.elapsed() * format_desc_.fps * 0.5 * 5);

        {
            core::draw_frame new_frame = core::draw_frame(std::move(frame));

            std::lock_guard<std::mutex> lock(pairing_mutex_);
            latest_video_frame_ = std::move(new_frame);
            has_video_frame_    = true;
            try_pair_and_enqueue_locked(/*from_audio=*/false);
        }
    }

#ifdef WIN32
    void OnAcceleratedPaint(CefRefPtr<CefBrowser>          browser,
                            PaintElementType               type,
                            const RectList&                dirtyRects,
                            const CefAcceleratedPaintInfo& info) override
    {
        try {
            if (!shared_texture_enable_ || closing_ || not_found_)
                return;

            graph_->set_value("browser-tick-time", paint_timer_.elapsed() * format_desc_.fps * 0.5);
            paint_timer_.restart();
            CASPAR_ASSERT(CefCurrentlyOn(TID_UI));

            if (type != PET_VIEW)
                return;

            if (d3d_shared_buffer_) {
                if (info.shared_texture_handle != d3d_shared_buffer_->share_handle())
                    d3d_shared_buffer_.reset();
            }

            if (!d3d_shared_buffer_) {
                d3d_shared_buffer_ = d3d_device_->open_shared_texture(info.shared_texture_handle);
                if (!d3d_shared_buffer_)
                    CASPAR_LOG(error) << print() << L" could not open shared texture!";
            }

            if (d3d_shared_buffer_) {
                core::pixel_format format = core::pixel_format::invalid;
                if (d3d_shared_buffer_->format() == DXGI_FORMAT_B8G8R8A8_UNORM) {
                    format = core::pixel_format::bgra;
                } else if (d3d_shared_buffer_->format() == DXGI_FORMAT_R8G8B8A8_UNORM) {
                    format = core::pixel_format::rgba;
                }

                if (format != core::pixel_format::invalid) {
                    auto frame =
                        frame_factory_->import_d3d_texture(this, d3d_shared_buffer_, format, common::bit_depth::bit8);
                    core::draw_frame dframe(std::move(frame));

                    {
                        std::lock_guard<std::mutex> lock(pairing_mutex_);
                        latest_video_frame_ = std::move(dframe);
                        has_video_frame_    = true;
                        try_pair_and_enqueue_locked(/*from_audio=*/false);
                    }
                }
            }
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
        }
    }
#endif

    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override
    {
        CASPAR_ASSERT(CefCurrentlyOn(TID_UI));

        browser_ = std::move(browser);
    }

    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override
    {
        CASPAR_ASSERT(CefCurrentlyOn(TID_UI));

        browser_ = nullptr;
    }

    bool DoClose(CefRefPtr<CefBrowser> browser) override
    {
        CASPAR_ASSERT(CefCurrentlyOn(TID_UI));

        return false;
    }

    bool OnConsoleMessage(CefRefPtr<CefBrowser> browser,
                          cef_log_severity_t    level,
                          const CefString&      message,
                          const CefString&      source,
                          int                   line) override
    {
        auto msg = log::replace_nonprintable_copy(message.ToWString(), L'?');
        if (level == cef_log_severity_t::LOGSEVERITY_DEBUG)
            CASPAR_LOG(debug) << print() << L" Log: " << msg;
        else if (level == cef_log_severity_t::LOGSEVERITY_WARNING)
            CASPAR_LOG(warning) << print() << L" Log: " << msg;
        else if (level == cef_log_severity_t::LOGSEVERITY_ERROR)
            CASPAR_LOG(error) << print() << L" Log: " << msg;
        else if (level == cef_log_severity_t::LOGSEVERITY_FATAL)
            CASPAR_LOG(fatal) << print() << L" Log: " << msg;
        else
            CASPAR_LOG(info) << print() << L" Log: " << msg;
        return true;
    }

    CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }

    CefRefPtr<CefAudioHandler> GetAudioHandler() override { return this; }

    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }

    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }

    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }

    void OnLoadError(CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefFrame>   frame,
                     ErrorCode             errorCode,
                     const CefString&      errorText,
                     const CefString&      failedUrl) override
    {
        // Replacing a template normally cancels its in-flight navigation.
        // Neither cancellation nor a failed child iframe invalidates the
        // parent renderer and its independent on-air graphics.
        if (errorCode == ERR_ABORTED)
            return;
        CASPAR_LOG(warning) << "[html_producer] " << errorText.ToString() << " while loading url: \""
                            << failedUrl.ToString() << "\"";

        if (!frame->IsMain())
            return;
        not_found_ = true;
        // Stop producing if the page fails to load
        {
            std::lock_guard<std::mutex> lock(buffer_mutex_);
            buffer_.push_back({core::draw_frame{}, steady_now_ms()});
        }

        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            state_ = {};
        }
    }

    void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override
    {
        if (not_found_)
            return;

        loaded_ = true;
        execute_queued_javascript();
    }

    bool OnProcessMessageReceived(CefRefPtr<CefBrowser>        browser,
                                  CefRefPtr<CefFrame>          frame,
                                  CefProcessId                 source_process,
                                  CefRefPtr<CefProcessMessage> message) override
    {
        auto name = message->GetName().ToString();

        if (name == REMOVE_MESSAGE_NAME) {
            // TODO fully remove producer
            this->close();

            {
                std::lock_guard<std::mutex> lock(buffer_mutex_);
                buffer_.push_back({core::draw_frame{}, steady_now_ms()});
            }

            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                state_ = {};
            }

            return true;
        }
        if (name == LOG_MESSAGE_NAME) {
            auto args     = message->GetArgumentList();
            auto severity = static_cast<boost::log::trivial::severity_level>(args->GetInt(0));
            auto msg      = log::replace_nonprintable_copy(args->GetString(1).ToWString(), L'?');

            BOOST_LOG_SEV(log::logger::get(), severity) << print() << L" [renderer_process] " << msg;
        }

        return false;
    }

    bool GetAudioParameters(CefRefPtr<CefBrowser> browser, CefAudioParameters& params) override
    {
        params.channel_layout    = CEF_CHANNEL_LAYOUT_7_1;
        params.sample_rate       = format_desc_.audio_sample_rate;
        params.frames_per_buffer = format_desc_.audio_cadence[0];
        return true; // true keeps audio routed into the mixer; false would leak it to system audio instead
    }

    void OnAudioStreamStarted(CefRefPtr<CefBrowser> browser, const CefAudioParameters& params, int channels) override
    {
        audioResampler_ = std::make_unique<ffmpeg::AudioResampler>(params.sample_rate, AV_SAMPLE_FMT_FLTP);

        // A restarted stream's pts isn't guaranteed to continue from where the last one left off -
        // force recalibration (see OnAudioStreamPacket) rather than reuse a possibly-stale offset.
        std::lock_guard<std::mutex> lock(pairing_mutex_);
        audio_clock_calibrated_ = false;
        audio_buffer_.clear();
    }
    void OnAudioStreamPacket(CefRefPtr<CefBrowser> browser, const float** data, int samples, int64_t pts) override
    {
        if (!audioResampler_)
            return;

        auto audio = audioResampler_->convert(samples, reinterpret_cast<const void**>(data));

        std::lock_guard<std::mutex> lock(pairing_mutex_);

        auto now_ms          = steady_now_ms();
        auto expected_offset = now_ms - pts;

        if (!audio_clock_calibrated_) {
            audio_clock_offset_ms_  = expected_offset;
            audio_clock_calibrated_ = true;
        } else {
            auto offset_delta = expected_offset > audio_clock_offset_ms_ ? expected_offset - audio_clock_offset_ms_
                                                                         : audio_clock_offset_ms_ - expected_offset;
            if (offset_delta > 1000) {
                // CEF's audio clock reference has shifted (e.g. a stalled/backgrounded tab resuming)
                // - hard resync rather than let stale buffered samples/position tracking stay wrong.
                audio_buffer_.clear();
                audio_clock_offset_ms_ = expected_offset;
                graph_->set_tag(diagnostics::tag_severity::WARNING, "audio-clock-resync");
            }
        }

        if (audio_buffer_.empty()) {
            audio_buffer_start_ms_ = static_cast<double>(pts + audio_clock_offset_ms_);
        }

        audio_buffer_.insert(audio_buffer_.end(), audio.data(), audio.data() + audio.size());

        // Cap backlog to ~kAudioBacklogPackets packets' worth, dropping the oldest on overflow.
        const auto max_buffered_samples = kAudioBacklogPackets * audio.size();
        if (audio_buffer_.size() > max_buffered_samples) {
            auto dropped_elements = audio_buffer_.size() - max_buffered_samples;
            audio_buffer_.erase(audio_buffer_.begin(), audio_buffer_.begin() + dropped_elements);
            audio_buffer_start_ms_ += dropped_elements / double(kAudioChannels) * 1000.0 / format_desc_.audio_sample_rate;
        }

        try_pair_and_enqueue_locked(/*from_audio=*/true);
    }
    void OnAudioStreamStopped(CefRefPtr<CefBrowser> browser) override { audioResampler_ = nullptr; }
    void OnAudioStreamError(CefRefPtr<CefBrowser> browser, const CefString& message) override
    {
        CASPAR_LOG(info) << "[html_producer] OnAudioStreamError: \"" << message.ToString() << "\"";
        audioResampler_ = nullptr;
    }

    void do_execute_javascript(const std::wstring& javascript)
    {
        html::begin_invoke([this, javascript] {
            if (browser_ != nullptr)
                browser_->GetMainFrame()->ExecuteJavaScript(
                    u8(javascript).c_str(), browser_->GetMainFrame()->GetURL(), 0);
        });
    }

    void execute_queued_javascript()
    {
        std::wstring javascript;

        while (javascript_before_load_.try_pop(javascript))
            do_execute_javascript(javascript);
    }

    std::wstring print() const
    {
        return L"html[" + url_ + L"]" + L" " + std::to_wstring(format_desc_.square_width) + L" " +
               std::to_wstring(format_desc_.square_height) + L" " + std::to_wstring(format_desc_.fps);
    }

    IMPLEMENT_REFCOUNTING(html_client);
};

class html_producer : public core::frame_producer
{
    core::video_format_desc             format_desc_;
    const std::wstring                  url_;
    spl::shared_ptr<diagnostics::graph> graph_;

    CefRefPtr<html_client> client_;

  public:
    html_producer(const spl::shared_ptr<core::frame_factory>& frame_factory,
                  const core::video_format_desc&              format_desc,
                  const std::wstring&                         url)
        : format_desc_(format_desc)
        , url_(url)
    {
        html::invoke([&] {
            auto gpu = is_gpu_shared_texture_enabled();

            client_ = new html_client(frame_factory, graph_, format_desc, gpu.first, gpu.second, url_);

            CefWindowInfo window_info;
            window_info.bounds.width                 = format_desc.square_width;
            window_info.bounds.height                = format_desc.square_height;
            window_info.windowless_rendering_enabled = true;
            window_info.shared_texture_enabled       = gpu.second;

            CefBrowserSettings browser_settings;
            browser_settings.webgl = gpu.first ? cef_state_t::STATE_ENABLED : cef_state_t::STATE_DISABLED;
            double fps             = format_desc.fps;
            browser_settings.windowless_frame_rate = int(ceil(fps));
            CefBrowserHost::CreateBrowser(window_info, client_.get(), url, browser_settings, nullptr, nullptr);
        });
    }

    ~html_producer() override
    {
        if (client_ != nullptr)
            client_->close();
    }

    // frame_producer

    std::wstring name() const override { return L"html"; }

    core::draw_frame receive_impl(const core::video_field field, int nb_samples) override
    {
        if (client_ != nullptr) {
            return client_->receive(field);
        }

        return core::draw_frame::empty();
    }

    core::draw_frame first_frame(const core::video_field field) override { return receive_impl(field, 0); }

    bool is_ready() override
    {
        if (client_ != nullptr) {
            return client_->is_ready();
        }
        return false;
    }

    core::draw_frame last_frame(const core::video_field field) override
    {
        if (client_ != nullptr) {
            return client_->last_frame();
        }

        return core::draw_frame::empty();
    }

    std::future<std::wstring> call(const std::vector<std::wstring>& params) override
    {
        if (client_ == nullptr)
            return make_ready_future(std::wstring());

        auto javascript = params.at(0);

        if (javascript == L"RELOAD") {
            client_->reload();
        } else {
            client_->execute_javascript(javascript);
        }

        return make_ready_future(std::wstring());
    }

    std::wstring print() const override { return L"html[" + url_ + L"]"; }

    core::monitor::state state() const override
    {
        if (client_ != nullptr) {
            return client_->state();
        }

        static const core::monitor::state empty;
        return empty;
    }
};

spl::shared_ptr<core::frame_producer> create_cg_producer(const core::frame_producer_dependencies& dependencies,
                                                         const std::vector<std::wstring>&         params)
{
    const auto html_prefix    = boost::iequals(params.at(0), L"[HTML]");
    const auto param_url      = html_prefix ? params.at(1) : params.at(0);
    const auto filename       = env::template_folder() + param_url + L".html";
    const auto found_filename = find_case_insensitive(filename);
    const auto http_prefix =
        boost::algorithm::istarts_with(param_url, L"http:") || boost::algorithm::istarts_with(param_url, L"https:");

    if (!found_filename && !http_prefix && !html_prefix)
        return core::frame_producer::empty();

    const auto url = found_filename ? L"file://" + *found_filename : param_url;

    std::optional<int> width;
    std::optional<int> height;
    {
        auto u8_url = u8(url);

        boost::smatch what;
        if (boost::regex_search(u8_url, what, boost::regex("width=([0-9]+)"))) {
            width = std::stoi(what[1].str());
        }

        if (boost::regex_search(u8_url, what, boost::regex("height=([0-9]+)"))) {
            height = std::stoi(what[1].str());
        }
    }

    auto format_desc = dependencies.format_desc;
    if (width && height) {
        format_desc.width         = *width;
        format_desc.square_width  = *width;
        format_desc.height        = *height;
        format_desc.square_height = *height;
    }

    return spl::make_shared<html_producer>(dependencies.frame_factory, format_desc, url);
}

spl::shared_ptr<core::frame_producer> create_producer(const core::frame_producer_dependencies& dependencies,
                                                      const std::vector<std::wstring>&         params)
{
    return create_cg_producer(dependencies, params);
}

}} // namespace caspar::html
