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

#include "proaudio_consumer.h"

#include "../util/ring_buffer.h"

#include <common/diagnostics/graph.h>
#include <common/except.h>
#include <common/executor.h>
#include <common/log.h>
#include <common/param.h>
#include <common/timer.h>
#include <common/timespan.h>
#include <common/utf.h>

#include <core/consumer/channel_info.h>
#include <core/consumer/frame_consumer.h>
#include <core/frame/frame.h>
#include <core/video_format.h>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>

#include <portaudio.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstring>
#include <future>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace caspar { namespace proaudio {

using namespace std::chrono_literals;

// PortAudio's Pa_Initialize/Pa_Terminate must be balanced process-wide. Refcounted so the first
// consumer initializes it and the last one tears it down, same lifetime idea as OAL's device
// singleton, but PortAudio needs no per-thread "current context" handling.
class library
{
    library()
    {
        auto err = Pa_Initialize();
        if (err != paNoError)
            CASPAR_THROW_EXCEPTION(invalid_operation()
                                   << msg_info(std::string("Failed to initialize PortAudio: ") + Pa_GetErrorText(err)));
    }

    inline static std::mutex           mutex_;
    inline static std::weak_ptr<library> instance_;

  public:
    ~library() { Pa_Terminate(); }

    static std::shared_ptr<library> acquire()
    {
        std::lock_guard guard{mutex_};

        auto shared = instance_.lock();
        if (!shared)
            instance_ = shared = std::shared_ptr<library>{new library()};

        return shared;
    }
};

namespace {

std::string clean_name(const std::string& s)
{
    std::string out;
    for (unsigned char c : s) {
        if (std::isgraph(c))
            out += static_cast<char>(std::tolower(c));
    }
    return out;
}

std::vector<int> parse_channel_map(const std::string& str, int default_channels)
{
    std::vector<int> map;

    if (str.empty()) {
        for (int i = 0; i < default_channels; ++i)
            map.push_back(i + 1);
        return map;
    }

    std::stringstream ss(str);
    std::string       item;
    while (std::getline(ss, item, ',')) {
        boost::trim(item);
        if (item.empty())
            continue;

        std::size_t pos  = 0;
        auto        value = std::stoi(item, &pos);
        if (pos != item.size())
            throw std::invalid_argument("CHANNEL_MAP entry is not a plain integer: '" + item + "'");

        map.push_back(value);
    }
    return map;
}

// Finds a PortAudio output device by (fuzzy, case/whitespace-insensitive) name and, optionally,
// host API name (e.g. "ASIO", "WASAPI", "ALSA", "JACK") to disambiguate identically-named devices
// exposed through more than one host API. Empty device_name + empty host_api_name means "default".
PaDeviceIndex find_device(const std::string& device_name, const std::string& host_api_name)
{
    if (device_name.empty() && host_api_name.empty())
        return Pa_GetDefaultOutputDevice();

    auto target_name = clean_name(device_name);
    auto target_api   = clean_name(host_api_name);

    std::vector<std::string> available;
    auto                     count = Pa_GetDeviceCount();

    for (PaDeviceIndex i = 0; i < count; ++i) {
        auto info = Pa_GetDeviceInfo(i);
        if (!info || info->maxOutputChannels <= 0)
            continue;

        auto api_info = Pa_GetHostApiInfo(info->hostApi);
        auto api_name  = api_info ? api_info->name : "?";

        available.push_back(std::string(info->name) + " (" + api_name + ")");

        if (!target_api.empty() && clean_name(api_name).find(target_api) == std::string::npos)
            continue;

        if (target_name.empty() || clean_name(info->name).find(target_name) != std::string::npos)
            return i;
    }

    CASPAR_LOG(info) << "-------- PortAudio Devices -------";
    for (auto&& name : available)
        CASPAR_LOG(info) << u16(name);
    CASPAR_LOG(info) << "-------- PortAudio Devices -------";

    CASPAR_THROW_EXCEPTION(invalid_operation()
                           << msg_info("Invalid PortAudio device/host-api: '" + device_name + "' '" + host_api_name +
                                       "'"));
}

} // namespace

struct proaudio_consumer : public core::frame_consumer
{
    spl::shared_ptr<diagnostics::graph> graph_;
    caspar::timer                       tick_timer_, frame_timer_;
    int                                 channel_index_ = -1;

    core::video_format_desc format_desc_;

    std::shared_ptr<library> library_;
    std::string               device_name_;
    std::string               host_api_name_;
    std::string               channel_map_param_;
    std::vector<int>           channel_map_; // 1-based destination device channel per source channel
    timespan                   delay_;

    PaStream* stream_          = nullptr;
    int       device_channels_ = 0; // width the stream was opened with

    static constexpr unsigned long max_frames_per_callback = 8192;
    std::vector<std::int16_t>      scratch_; // pre-sized once, only touched by the realtime callback

    std::unique_ptr<frame_ring_buffer> ring_;
    std::atomic<std::uint64_t>         frames_played_{0};
    std::atomic<bool>                  dropout_{false};
    std::atomic<bool>                  stop_{false};

    executor executor_{L"proaudio_consumer"};

  public:
    explicit proaudio_consumer(std::string device_name, std::string host_api_name, std::string channel_map, timespan delay)
        : library_(library::acquire())
        , device_name_(std::move(device_name))
        , host_api_name_(std::move(host_api_name))
        , channel_map_param_(std::move(channel_map))
        , delay_(delay)
    {
        using diagnostics::color;
        graph_->set_color("tick-time", color(0.0, 0.6, 0.9));
        graph_->set_color("queue-size", color(0.2, 0.9, 0.9));
        graph_->set_color("frame-time", color(0.1, 1.0, 0.1));
        graph_->set_color("drop-out", color(0.6, 0.3, 0.3));
        diagnostics::register_graph(graph_);
    }

    ~proaudio_consumer() override
    {
        stop_ = true;

        executor_.invoke([this] {
            if (stream_) {
                Pa_StopStream(stream_);
                Pa_CloseStream(stream_);
                stream_ = nullptr;
            }
        });
    }

    // frame_consumer

    void initialize(const core::video_format_desc& format_desc,
                    const core::channel_info&      channel_info,
                    int                            port_index) override
    {
        format_desc_   = format_desc;
        channel_index_ = channel_info.index;
        graph_->set_text(print());

        executor_.begin_invoke([this] {
            try {
                channel_map_ = parse_channel_map(channel_map_param_, format_desc_.audio_channels);
                if (channel_map_.empty())
                    CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("CHANNEL_MAP resolved to no channels"));

                auto device = find_device(device_name_, host_api_name_);
                auto info   = Pa_GetDeviceInfo(device);

                // Every entry must be a valid 1-based device channel - a stray 0/negative entry would
                // underflow to a huge index and corrupt memory in the realtime callback below.
                for (auto ch : channel_map_) {
                    if (ch < 1)
                        CASPAR_THROW_EXCEPTION(
                            invalid_operation() << msg_info("CHANNEL_MAP entries must be 1-based channel numbers, got " +
                                                            std::to_string(ch)));
                }

                auto max_dest_channel = *std::max_element(channel_map_.begin(), channel_map_.end());

                if (max_dest_channel > info->maxOutputChannels)
                    CASPAR_THROW_EXCEPTION(
                        invalid_operation() << msg_info("CHANNEL_MAP requires channel " +
                                                        std::to_string(max_dest_channel) + " but device '" +
                                                        info->name + "' only has " +
                                                        std::to_string(info->maxOutputChannels) + " output channels"));

                device_channels_ = max_dest_channel;

                auto num_silence = delay_.in_frames(format_desc_.fps);
                num_silence      = std::clamp<int>(num_silence, 1, format_desc_.fps);

                auto max_cadence =
                    *std::max_element(format_desc_.audio_cadence.begin(), format_desc_.audio_cadence.end());

                // At least double the largest block PortAudio's callback can ask for in one go, so an
                // oversized host-requested block can't drain the ring dry and report a false dropout.
                auto ring_capacity = std::max<std::size_t>(static_cast<std::size_t>(num_silence + 4) * max_cadence,
                                                           static_cast<std::size_t>(max_frames_per_callback) * 2);

                ring_ = std::make_unique<frame_ring_buffer>(ring_capacity, channel_map_.size());
                scratch_.resize(max_frames_per_callback * channel_map_.size());

                // Pre-roll silence so the callback has real data to consume from the first tick,
                // instead of waiting on data that hasn't been pushed by send() yet.
                std::vector<std::int16_t> silence(static_cast<std::size_t>(max_cadence) * channel_map_.size(), 0);
                for (auto n = 0; n < num_silence; ++n) {
                    auto cadence = format_desc_.audio_cadence[n % format_desc_.audio_cadence.size()];
                    ring_->push(silence.data(), cadence);
                }

                PaStreamParameters out_params{};
                out_params.device                    = device;
                out_params.channelCount               = device_channels_;
                out_params.sampleFormat               = paInt16;
                out_params.suggestedLatency           = info->defaultLowOutputLatency;
                out_params.hostApiSpecificStreamInfo = nullptr;

                auto err = Pa_OpenStream(&stream_,
                                         nullptr,
                                         &out_params,
                                         format_desc_.audio_sample_rate,
                                         paFramesPerBufferUnspecified,
                                         paNoFlag,
                                         &proaudio_consumer::pa_callback,
                                         this);
                if (err != paNoError)
                    CASPAR_THROW_EXCEPTION(invalid_operation()
                                           << msg_info(std::string("Failed to open PortAudio stream: ") +
                                                       Pa_GetErrorText(err)));

                err = Pa_StartStream(stream_);
                if (err != paNoError)
                    CASPAR_THROW_EXCEPTION(invalid_operation()
                                           << msg_info(std::string("Failed to start PortAudio stream: ") +
                                                       Pa_GetErrorText(err)));

                CASPAR_LOG(info) << print() << " Using device '" << u16(std::string(info->name)) << "'";
                CASPAR_LOG(info) << print() << " Latency: " << num_silence << " frames";
            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
            }
        });
    }

    std::future<bool> send(core::video_field field, core::const_frame frame) override
    {
        auto target_promise = std::make_shared<std::promise<std::uint64_t>>();
        auto target_future  = target_promise->get_future().share();

        executor_.begin_invoke([this, frame = std::move(frame), target_promise] {
            if (!stream_) {
                target_promise->set_value(frames_played_.load(std::memory_order_acquire));
                return;
            }

            auto&& audio_in     = frame.audio_data();
            auto    audio_ptr    = audio_in.data();
            auto    src_channels = format_desc_.audio_channels;
            auto    n_map        = channel_map_.size();
            int     num_samples  = src_channels > 0 ? static_cast<int>(audio_in.size()) / src_channels : 0;

            std::vector<std::int16_t> out(static_cast<std::size_t>(num_samples) * n_map);
            for (int s = 0; s < num_samples; ++s) {
                for (std::size_t c = 0; c < n_map; ++c) {
                    // Direct channel selection, not a downmix - caspar source channel `c` goes to
                    // whatever device channel CHANNEL_MAP[c] says, nothing is summed together.
                    std::int32_t v = static_cast<int>(c) < src_channels
                                        ? audio_ptr[static_cast<std::size_t>(s) * src_channels + c]
                                        : 0;
                    out[static_cast<std::size_t>(s) * n_map + c] = static_cast<std::int16_t>(v >> 16);
                }
            }

            auto written = ring_->push(out.data(), static_cast<std::size_t>(num_samples));
            if (written < static_cast<std::size_t>(num_samples))
                dropout_.store(true, std::memory_order_relaxed);

            target_promise->set_value(frames_played_.load(std::memory_order_acquire) +
                                      static_cast<std::uint64_t>(num_samples));

            graph_->set_value("tick-time", tick_timer_.elapsed() * format_desc_.fps * 0.5);
            tick_timer_.restart();
            if (dropout_.exchange(false, std::memory_order_relaxed))
                graph_->set_tag(diagnostics::tag_severity::WARNING, "drop-out");
        });

        return std::async(std::launch::deferred, [this, target_future] {
            // Block until the executor task above has actually computed the target - only then do
            // we know how many played-frames to wait for.
            auto target = target_future.get();

            while (!stop_) {
                if (frames_played_.load(std::memory_order_acquire) >= target) {
                    graph_->set_value("frame-time", frame_timer_.elapsed() * format_desc_.fps * 0.5);
                    frame_timer_.restart();
                    return true;
                }
                std::this_thread::sleep_for(100us);
            }
            return false;
        });
    }

    std::wstring print() const override
    {
        return L"proaudio[" + std::to_wstring(channel_index_) + L"|" + format_desc_.name + L"]";
    }

    std::wstring name() const override { return L"proaudio"; }

    bool has_synchronization_clock() const override { return true; }

    int index() const override { return 520; }

    core::monitor::state state() const override
    {
        static const core::monitor::state empty;
        return empty;
    }

  private:
    // Realtime audio thread - no locks, no allocation, no logging in here.
    static int pa_callback(const void* /*input*/,
                           void*                            output,
                           unsigned long                    frame_count,
                           const PaStreamCallbackTimeInfo* /*time_info*/,
                           PaStreamCallbackFlags /*status_flags*/,
                           void* user_data)
    {
        auto self = static_cast<proaudio_consumer*>(user_data);
        auto out  = static_cast<std::int16_t*>(output);

        std::memset(out, 0, static_cast<std::size_t>(frame_count) * self->device_channels_ * sizeof(std::int16_t));

        auto frames = std::min<unsigned long>(frame_count, max_frames_per_callback);
        if (frames < frame_count)
            self->dropout_.store(true, std::memory_order_relaxed);

        auto n_map   = self->channel_map_.size();
        auto played = self->ring_->pop(self->scratch_.data(), frames);

        for (unsigned long i = 0; i < frames; ++i) {
            for (std::size_t c = 0; c < n_map; ++c) {
                auto dest_ch = static_cast<std::size_t>(self->channel_map_[c] - 1); // 1-based -> 0-based
                out[i * self->device_channels_ + dest_ch] = self->scratch_[i * n_map + c];
            }
        }

        self->frames_played_.fetch_add(frames, std::memory_order_release);
        if (played < frames)
            self->dropout_.store(true, std::memory_order_relaxed);

        return paContinue;
    }
};

spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>&     params,
                                                      const core::video_format_repository& format_repository,
                                                      const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                                                      const core::channel_info& channel_info)
{
    if (params.empty() || !boost::iequals(params.at(0), L"PROAUDIO"))
        return core::frame_consumer::empty();

    auto device_name  = u8(get_param(L"DEVICE_NAME", params, L""));
    auto host_api     = u8(get_param(L"HOST_API", params, L""));
    auto channel_map  = u8(get_param(L"CHANNEL_MAP", params, L""));
    auto delay        = u8(get_param(L"DELAY", params, L"0"));

    return spl::make_shared<proaudio_consumer>(device_name, host_api, channel_map, timespan{delay});
}

spl::shared_ptr<core::frame_consumer>
create_preconfigured_consumer(const boost::property_tree::wptree&                      ptree,
                              const core::video_format_repository&                     format_repository,
                              const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                              const core::channel_info&                                channel_info)
{
    auto device_name  = u8(ptree.get(L"device-name", L""));
    auto host_api     = u8(ptree.get(L"host-api", L""));
    auto channel_map  = u8(ptree.get(L"channel-map", L""));
    auto delay        = u8(ptree.get(L"delay", L"0"));

    return spl::make_shared<proaudio_consumer>(device_name, host_api, channel_map, timespan{delay});
}

}} // namespace caspar::proaudio
