/*
 * portaudio_consumer.cpp - Multi-Channel ASIO Audio Output with Video-Scheduled Sync
 * Based on OAL consumer video-scheduled architecture, adapted for PortAudio callback model
 */

#include "portaudio_consumer.h"

#include <common/diagnostics/graph.h>
#include <common/env.h>
#include <common/except.h>
#include <common/executor.h>
#include <common/future.h>
#include <common/log.h>
#include <common/param.h>
#include <common/timer.h>
#include <common/utf.h>

#include <core/consumer/channel_info.h>
#include <core/consumer/frame_consumer.h>
#include <core/frame/frame.h>
#include <core/video_format.h>

#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/property_tree/ptree.hpp>

#include <portaudio.h>

#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <numeric>
#include <queue>
#include <thread>
#include <vector>

namespace caspar { namespace portaudio {

// Audio packet for video-scheduled playback
struct audio_packet
{
    int64_t                                        video_frame_number;
    std::chrono::high_resolution_clock::time_point target_time;
    std::vector<float>                             samples; // Already converted to float, interleaved
    int                                            sample_rate;
    int                                            channels;

    bool operator<(const audio_packet& other) const
    {
        return target_time > other.target_time; // For priority queue (earliest first)
    }
};

// Lock-free ring buffer for float samples (single producer, single consumer)
class float_fifo
{
  public:
    explicit float_fifo(size_t capacity_samples = 0) { reset(capacity_samples); }

    void reset(size_t capacity_samples)
    {
        // Force power-of-two capacity for efficient wrapping
        size_t cap = 1;
        while (cap < capacity_samples) {
            cap <<= 1;
        }

        buffer_.assign(cap, 0.0f);
        mask_ = cap - 1;

        write_idx_.store(0, std::memory_order_relaxed);
        read_idx_.store(0, std::memory_order_relaxed);
    }

    size_t capacity() const { return buffer_.size(); }

    size_t available_to_read() const
    {
        const size_t w = write_idx_.load(std::memory_order_acquire);
        const size_t r = read_idx_.load(std::memory_order_acquire);
        return w - r;
    }

    size_t available_to_write() const { return capacity() - available_to_read(); }

    // Producer: write as many samples as will fit. Returns samples written.
    size_t write(const float* src, size_t count)
    {
        if (count == 0 || buffer_.empty()) {
            return 0;
        }

        const size_t r    = read_idx_.load(std::memory_order_acquire);
        const size_t w    = write_idx_.load(std::memory_order_relaxed);
        const size_t used = w - r;
        const size_t free = buffer_.size() - used;

        const size_t to_write = (count <= free) ? count : free;

        // Write in up to two chunks (wrap)
        for (size_t i = 0; i < to_write; ++i) {
            buffer_[(w + i) & mask_] = src[i];
        }

        write_idx_.store(w + to_write, std::memory_order_release);
        return to_write;
    }

    // Consumer: read up to count samples. Returns samples read.
    size_t read(float* dst, size_t count)
    {
        if (count == 0 || buffer_.empty()) {
            return 0;
        }

        const size_t w     = write_idx_.load(std::memory_order_acquire);
        const size_t r     = read_idx_.load(std::memory_order_relaxed);
        const size_t avail = w - r;

        const size_t to_read = (count <= avail) ? count : avail;

        for (size_t i = 0; i < to_read; ++i) {
            dst[i] = buffer_[(r + i) & mask_];
        }

        read_idx_.store(r + to_read, std::memory_order_release);
        return to_read;
    }

  private:
    std::vector<float>  buffer_;
    size_t              mask_ = 0;
    std::atomic<size_t> write_idx_{0};
    std::atomic<size_t> read_idx_{0};
};

// Find the ASIO host API
int find_asio_host_api()
{
    int numApis = Pa_GetHostApiCount();
    for (int i = 0; i < numApis; i++) {
        const PaHostApiInfo* apiInfo = Pa_GetHostApiInfo(i);
        if (apiInfo && apiInfo->type == paASIO) {
            return i;
        }
    }
    return -1;
}

// Device name matching helper - ASIO-specific
int find_device_by_name(const std::wstring& device_name)
{
    // First, find the ASIO host API
    int asio_api_index = find_asio_host_api();

    if (asio_api_index < 0) {
        CASPAR_LOG(error) << L"ASIO host API not found!";
        CASPAR_LOG(error) << L"Make sure you have ASIO drivers installed.";
        return -1;
    }

    const PaHostApiInfo* asioApi = Pa_GetHostApiInfo(asio_api_index);
    CASPAR_LOG(info) << L"Found ASIO host API with " << asioApi->deviceCount << L" devices";

    // If no device name specified, use ASIO default
    if (device_name.empty()) {
        int default_device = asioApi->defaultOutputDevice;
        if (default_device != paNoDevice) {
            const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(default_device);
            CASPAR_LOG(info) << L"Using default ASIO device: \"" << deviceInfo->name << L"\"";
            return default_device;
        }
        return -1;
    }

    // Prepare search string
    std::string search_name           = u8(device_name);
    std::string search_name_no_spaces = search_name;
    boost::algorithm::erase_all(search_name_no_spaces, " ");
    boost::algorithm::to_lower(search_name_no_spaces);

    CASPAR_LOG(info) << L"------- ASIO Device List -----";
    CASPAR_LOG(info) << L"Searching for: \"" << device_name << L"\"";

    int found_device = -1;

    // Only enumerate ASIO devices
    for (int i = 0; i < asioApi->deviceCount; i++) {
        int device_index = Pa_HostApiDeviceIndexToDeviceIndex(asio_api_index, i);
        if (device_index < 0) {
            continue;
        }

        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(device_index);
        if (!deviceInfo || deviceInfo->maxOutputChannels <= 0) {
            continue;
        }

        CASPAR_LOG(info) << L"ASIO Device " << i << L": \"" << deviceInfo->name << L"\" - "
                         << deviceInfo->maxOutputChannels << L" output channels";

        // Fuzzy match
        std::string dev_name           = deviceInfo->name;
        std::string dev_name_no_spaces = dev_name;
        boost::algorithm::erase_all(dev_name_no_spaces, " ");
        boost::algorithm::to_lower(dev_name_no_spaces);

        // First try exact match (with spaces)
        if (boost::iequals(search_name, dev_name)) {
            found_device = device_index;
            CASPAR_LOG(info) << L"  ** EXACT MATCH **";
        }
        // Then try fuzzy match (without spaces)
        else if (boost::iequals(search_name_no_spaces, dev_name_no_spaces)) {
            found_device = device_index;
            CASPAR_LOG(info) << L"  ** FUZZY MATCH **";
        }
    }

    CASPAR_LOG(info) << L"------ ASIO Device List Done -----";

    if (found_device >= 0) {
        const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(found_device);
        CASPAR_LOG(info) << L"Selected ASIO device: \"" << deviceInfo->name << L"\"";
    } else {
        CASPAR_LOG(warning) << L"ASIO device \"" << device_name << L"\" not found";
        // Try to use default ASIO device as fallback
        int default_device = asioApi->defaultOutputDevice;
        if (default_device != paNoDevice) {
            const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(default_device);
            CASPAR_LOG(warning) << L"Using default ASIO device instead: \"" << deviceInfo->name << L"\"";
            found_device = default_device;
        }
    }

    return found_device;
}

// PortAudio initialization helper
class portaudio_initializer
{
  public:
    portaudio_initializer()
    {
        PaError err = Pa_Initialize();
        if (err != paNoError) {
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(std::string("PortAudio initialization failed: ") +
                                                                  Pa_GetErrorText(err)));
        }
        CASPAR_LOG(info) << L"PortAudio initialized successfully";
        CASPAR_LOG(info) << L"PortAudio version: " << Pa_GetVersionText();
    }

    ~portaudio_initializer()
    {
        Pa_Terminate();
        CASPAR_LOG(info) << L"PortAudio terminated";
    }
};

void init_portaudio()
{
    static std::unique_ptr<portaudio_initializer> instance;
    static std::once_flag                         f;

    std::call_once(f, [&] { instance = std::make_unique<portaudio_initializer>(); });
}

// Video-scheduled PortAudio consumer
class portaudio_consumer : public core::frame_consumer
{
  private:
    // Diagnostics and basic info
    spl::shared_ptr<diagnostics::graph> graph_;
    caspar::timer                       perf_timer_;
    int                                 channel_index_ = -1;
    core::video_format_desc             format_desc_;

    // Video-scheduled audio system
    std::priority_queue<audio_packet> audio_schedule_;
    std::mutex                        schedule_mutex_;
    std::condition_variable           schedule_cv_;
    std::thread                       audio_dispatch_thread_;
    std::atomic<bool>                 stop_audio_dispatch_{false};

    // Timing and synchronization
    std::chrono::high_resolution_clock::time_point channel_start_time_;
    std::atomic<int64_t>                           frame_counter_{0};
    double                                         frame_duration_seconds_ = 0.0;
    std::atomic<int>                               audio_latency_compensation_ms_{40};

    // PortAudio resources
    PaStream*     stream_          = nullptr;
    int           output_channels_ = 2;
    std::wstring  device_name_;
    int           device_index_       = -1;
    unsigned long buffer_size_frames_ = 128;

    // FIFO for PortAudio callback
    float_fifo            audio_fifo_{0};
    std::atomic<uint64_t> overrun_count_{0};
    std::atomic<uint64_t> underrun_count_{0};
    std::atomic<uint64_t> callback_count_{0};
    std::atomic<uint64_t> packets_dispatched_{0};

    // Auto-tuning members
    std::vector<int64_t>                  timing_samples_;
    bool                                  auto_tune_enabled_ = false;
    std::chrono::steady_clock::time_point last_auto_tune_;
    int                                   auto_tune_sample_count_ = 0;

    executor executor_{L"portaudio_consumer"};

    void perform_auto_tune()
    {
        if (timing_samples_.empty())
            return;

        int64_t sum          = std::accumulate(timing_samples_.begin(), timing_samples_.end(), 0LL);
        int64_t avg_error_us = sum / static_cast<int64_t>(timing_samples_.size());
        int     avg_error_ms = static_cast<int>(avg_error_us / 1000);

        int64_t variance_sum = 0;
        for (auto sample : timing_samples_) {
            int64_t diff = sample - avg_error_us;
            variance_sum += diff * diff;
        }
        int stddev_ms = static_cast<int>(std::sqrt(variance_sum / timing_samples_.size()) / 1000);

        CASPAR_LOG(info) << L"PortAudio auto-tune: Avg=" << avg_error_ms << L"ms, StdDev=" << stddev_ms << L"ms";
        CASPAR_LOG(info) << L"  Current compensation: " << audio_latency_compensation_ms_.load() << L"ms";

        // Be more aggressive - adjust even for smaller errors
        if (std::abs(avg_error_ms) >= 2 && stddev_ms < 30) {
            int old_compensation = audio_latency_compensation_ms_.load();

            // Full correction (not 50% or 75%)
            int adjustment       = avg_error_ms;
            int new_compensation = old_compensation + adjustment;

            // Clamp
            if (new_compensation < 0)
                new_compensation = 0;
            if (new_compensation > 1000)
                new_compensation = 1000;

            audio_latency_compensation_ms_.store(new_compensation);

            CASPAR_LOG(info) << L"Auto-tuned latency: " << old_compensation << L"ms â†’ " << new_compensation
                             << L"ms (adjusted by " << adjustment << L"ms)";
        } else if (stddev_ms >= 30) {
            CASPAR_LOG(warning) << L"Auto-tune skipped: timing inconsistent (StdDev=" << stddev_ms << L"ms)";
        } else {
            CASPAR_LOG(info) << L"Auto-tune: timing good (Â±" << avg_error_ms << L"ms)";
        }

        timing_samples_.clear();
    }

    void start_audio_dispatch_thread()
    {
        audio_dispatch_thread_ = std::thread([this] {
            try {
                set_thread_realtime_priority();
                set_thread_name(L"portaudio-dispatch");

                CASPAR_LOG(info) << L"PortAudio dispatch thread started";

                while (!stop_audio_dispatch_) {
                    audio_packet packet;

                    // Wait for next scheduled audio packet
                    {
                        std::unique_lock<std::mutex> lock(schedule_mutex_);
                        schedule_cv_.wait(lock, [this] { return !audio_schedule_.empty() || stop_audio_dispatch_; });

                        if (stop_audio_dispatch_)
                            break;

                        packet = audio_schedule_.top();
                        audio_schedule_.pop();
                    }

                    // ALWAYS respect target_time - this is what applies the compensation
                    auto now = std::chrono::high_resolution_clock::now();
                    if (packet.target_time > now) {
                        auto sleep_us =
                            std::chrono::duration_cast<std::chrono::microseconds>(packet.target_time - now).count();

                        // Log timing for first 10 packets
                        static std::atomic<int> timing_log{0};
                        if (timing_log.fetch_add(1) < 10) {
                            auto frame_time_us =
                                std::chrono::duration_cast<std::chrono::microseconds>(
                                    std::chrono::duration<double>(packet.video_frame_number * frame_duration_seconds_))
                                    .count();
                            auto compensation_us =
                                frame_time_us - std::chrono::duration_cast<std::chrono::microseconds>(
                                                    packet.target_time - channel_start_time_)
                                                    .count();

                            CASPAR_LOG(debug)
                                << L"Frame " << packet.video_frame_number << L": sleeping " << sleep_us << L"us"
                                << L", compensation applied: " << (compensation_us / 1000) << L"ms";
                        }

                        std::this_thread::sleep_until(packet.target_time);
                    }

                    dispatch_audio_packet(packet);
                }

                CASPAR_LOG(info) << L"PortAudio dispatch thread stopped";

            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
            }
        });
    }

    void dispatch_audio_packet(const audio_packet& packet)
    {
        if (packet.samples.empty()) {
            return;
        }

        static std::atomic<int> dispatch_count{0};
        int                     count = dispatch_count.fetch_add(1);

        try {
            const float* src = packet.samples.data();
            size_t       cnt = packet.samples.size();

            // Remove this line:
            // size_t fifo_before = audio_fifo_.available_to_read();

            const size_t written    = audio_fifo_.write(src, cnt);
            size_t       fifo_after = audio_fifo_.available_to_read();

            // Log every 100 dispatches
            if (count % 100 == 0) {
                double fifo_ms = (fifo_after / static_cast<double>(output_channels_) /
                                  static_cast<double>(format_desc_.audio_sample_rate)) *
                                 1000.0;

                CASPAR_LOG(debug) << L"Dispatch #" << count << L": FIFO " << static_cast<int>(fifo_ms) << L"ms ("
                                  << (fifo_after * 100 / audio_fifo_.capacity()) << L"%), " << L"comp="
                                  << audio_latency_compensation_ms_.load() << L"ms, " << L"underruns="
                                  << underrun_count_.load();
            }

            if (written < cnt) {
                overrun_count_++;
                if (overrun_count_.load() % 100 == 1) {
                    CASPAR_LOG(warning) << L"FIFO overrun #" << overrun_count_.load();
                }
            }

            packets_dispatched_++;

        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
        }
    }

    // PortAudio callback - called from audio thread
    static int pa_callback_wrapper(const void*                     input,
                                   void*                           output,
                                   unsigned long                   frameCount,
                                   const PaStreamCallbackTimeInfo* timeInfo,
                                   PaStreamCallbackFlags           statusFlags,
                                   void*                           userData)
    {
        auto* consumer = static_cast<portaudio_consumer*>(userData);
        return consumer->audio_callback(output, frameCount, statusFlags);
    }

    int audio_callback(void* output, unsigned long frameCount, PaStreamCallbackFlags statusFlags)
    {
        uint64_t count = callback_count_.fetch_add(1);

        if (count < 20) {
            CASPAR_LOG(debug) << L"PortAudio callback #" << count << L": frameCount=" << frameCount << L", need="
                              << (frameCount * output_channels_) << L" samples";
        }

        float* out = static_cast<float*>(output);

        const size_t need = static_cast<size_t>(frameCount) * static_cast<size_t>(output_channels_);

        // Clear output first (safe default)
        std::memset(out, 0, need * sizeof(float));

        if (statusFlags & paOutputUnderflow) {
            underrun_count_++;
            if (count < 20) {
                CASPAR_LOG(debug) << L"  Callback #" << count << L": UNDERFLOW flagged by PortAudio";
            }
        }

        if (count < 20) {
            CASPAR_LOG(debug) << L"  FIFO available: " << audio_fifo_.available_to_read();
        }

        const size_t got = audio_fifo_.read(out, need);

        if (count < 20) {
            CASPAR_LOG(debug) << L"  Read " << got << L" samples from FIFO";
            if (got > 0) {
                CASPAR_LOG(debug) << L"  First sample values: [0]=" << out[0] << L", [1]=" << out[1];
            }
        }

        if (got < need) {
            underrun_count_++;
            if (count < 20) {
                CASPAR_LOG(debug) << L"  Callback #" << count << L": Underrun - got " << got << L" but needed "
                                  << need;
            }
        }

        return paContinue;
    }

    void set_thread_realtime_priority()
    {
#ifdef _WIN32
        SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
#else
        struct sched_param sp;
        sp.sched_priority = 50;
        pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp);
#endif
    }

    void set_thread_name(const std::wstring& name)
    {
#ifdef _WIN32
        SetThreadDescription(GetCurrentThread(), name.c_str());
#else
        std::string narrow_name = u8(name);
        pthread_setname_np(pthread_self(), narrow_name.c_str());
#endif
    }

  public:
    explicit portaudio_consumer()
    {
        graph_ = spl::make_shared<diagnostics::graph>();
        init_portaudio();

        graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("fifo-fill", diagnostics::color(0.9f, 0.6f, 0.0f));
        graph_->set_color("underruns", diagnostics::color(0.6f, 0.3f, 0.3f));
        graph_->set_color("overruns", diagnostics::color(0.3f, 0.6f, 0.3f));
        diagnostics::register_graph(graph_);

        // Get configuration
        audio_latency_compensation_ms_.store(
            static_cast<int>(env::properties().get(L"configuration.portaudio.latency-compensation-ms", 40)));

        auto_tune_enabled_ = env::properties().get(L"configuration.portaudio.auto-tune-latency", false);

        output_channels_ = static_cast<int>(env::properties().get(L"configuration.portaudio.output-channels", 2));

        device_name_ = env::properties().get(L"configuration.portaudio.device-name", L"");

        buffer_size_frames_ =
            static_cast<unsigned long>(env::properties().get(L"configuration.portaudio.buffer-size-frames", 128));

        if (auto_tune_enabled_) {
            CASPAR_LOG(info) << L"PortAudio: Auto-tuning enabled";
            timing_samples_.reserve(1000);
            last_auto_tune_ = std::chrono::steady_clock::now();
        }

        CASPAR_LOG(info) << L"PortAudio Consumer: " << output_channels_ << L" channels, " << buffer_size_frames_
                         << L" frames/buffer, " << audio_latency_compensation_ms_.load() << L"ms latency compensation";
    }

    ~portaudio_consumer() override
    {
        // Stop audio dispatch thread
        stop_audio_dispatch_ = true;
        schedule_cv_.notify_all();

        if (audio_dispatch_thread_.joinable()) {
            audio_dispatch_thread_.join();
        }

        // Clean up PortAudio
        executor_.invoke([=] {
            if (stream_ != nullptr) {
                Pa_StopStream(stream_);
                Pa_CloseStream(stream_);
                stream_ = nullptr;
            }
        });
    }

    void schedule_audio_for_frame(int64_t frame_number, const std::vector<int32_t>& samples, int sample_rate, int channels)
    {
        if (samples.empty()) {
            return;
        }

        // Log first few calls
        static std::atomic<int> schedule_count{0};
        int                     count = schedule_count.fetch_add(1);
        if (count < 10) {
            CASPAR_LOG(debug) << L"schedule_audio_for_frame #" << count << L": frame=" << frame_number
                              << L", samples=" << samples.size() << L", rate=" << sample_rate << L", channels="
                              << channels;
        }

        // Determine number of frames in the incoming interleaved audio
        const size_t in_channels = static_cast<size_t>(channels);
        if (in_channels == 0) {
            CASPAR_LOG(warning) << L"schedule_audio_for_frame: zero channels!";
            return;
        }

        const size_t in_frames = samples.size() / in_channels;
        if (in_frames == 0) {
            CASPAR_LOG(warning) << L"schedule_audio_for_frame: zero frames!";
            return;
        }

        audio_packet packet;
        packet.video_frame_number = frame_number;
        packet.sample_rate        = sample_rate;
        packet.channels           = output_channels_;

        // Always generate output in the stream's channel count (interleaved)
        packet.samples.resize(in_frames * static_cast<size_t>(output_channels_));

        // Caspar audio is 16.16 fixed-point (see OAL path using >> 16)
        // Convert to float in [-1, 1)
        for (size_t f = 0; f < in_frames; ++f) {
            const size_t in_base  = f * in_channels;
            const size_t out_base = f * static_cast<size_t>(output_channels_);

            for (int ch = 0; ch < output_channels_; ++ch) {
                int32_t s32 = 0;

                if (static_cast<size_t>(ch) < in_channels) {
                    s32 = samples[in_base + static_cast<size_t>(ch)];
                } else if (in_channels == 1) {
                    // Duplicate mono to all outputs
                    s32 = samples[in_base];
                } else {
                    s32 = 0;
                }

                // Clamp to 16-bit range in 16.16 domain, then convert to int16 then float
                const int32_t max_16_16 = static_cast<int32_t>(INT16_MAX) * 65536;
                const int32_t min_16_16 = static_cast<int32_t>(INT16_MIN) * 65536;

                if (s32 > max_16_16)
                    s32 = max_16_16;
                if (s32 < min_16_16)
                    s32 = min_16_16;

                const int16_t s16 = static_cast<int16_t>(s32 >> 16);
                const float   sf  = static_cast<float>(s16) * (1.0f / 32768.0f);

                packet.samples[out_base + static_cast<size_t>(ch)] = sf;
            }
        }

        if (count < 5) {
            // Log first few sample values to verify conversion
            CASPAR_LOG(debug) << L"  Converted packet: " << packet.samples.size() << L" float samples";
            if (packet.samples.size() >= 2) {
                CASPAR_LOG(debug) << L"  First samples: [0]=" << packet.samples[0] << L", [1]=" << packet.samples[1];
            }
        }

        // Calculate target time
        auto frame_time_offset =
            std::chrono::duration<double>(static_cast<double>(frame_number) * frame_duration_seconds_);
        auto latency_compensation = std::chrono::milliseconds(audio_latency_compensation_ms_.load());
        packet.target_time        = channel_start_time_ +
                             std::chrono::duration_cast<std::chrono::nanoseconds>(frame_time_offset) -
                             latency_compensation;

        // Schedule the packet
        {
            std::lock_guard<std::mutex> lock(schedule_mutex_);
            audio_schedule_.push(packet);
        }
        schedule_cv_.notify_one();

        if (count < 10) {
            CASPAR_LOG(debug) << L"  Packet scheduled, queue size now: " << audio_schedule_.size();
        }
    }

    void initialize(const core::video_format_desc& format_desc,
                    const core::channel_info&      channel_info,
                    int                            port_index) override
    {
        format_desc_            = format_desc;
        channel_index_          = channel_info.index;
        frame_duration_seconds_ = 1.0 / format_desc.fps;
        channel_start_time_     = std::chrono::high_resolution_clock::now();

        graph_->set_text(print());

        // CRITICAL: Do PortAudio initialization synchronously
        executor_.invoke([=] {
            // Find ASIO device by name
            device_index_ = find_device_by_name(device_name_);

            if (device_index_ < 0) {
                CASPAR_LOG(error) << L"========================================";
                CASPAR_LOG(error) << L"NO ASIO DEVICE AVAILABLE";
                CASPAR_LOG(error) << L"========================================";
                CASPAR_LOG(error) << L"PortAudio consumer requires ASIO for multi-channel output.";
                CASPAR_LOG(error) << L"";
                CASPAR_LOG(error) << L"Please install an ASIO driver:";
                CASPAR_LOG(error) << L"  - Hardware ASIO (from your audio interface manufacturer)";
                CASPAR_LOG(error) << L"  - ASIO4ALL (universal ASIO driver for any audio device)";
                CASPAR_LOG(error) << L"  - FlexASIO (alternative universal ASIO driver)";
                CASPAR_LOG(error) << L"========================================";

                CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("No ASIO device available"));
            }

            // Setup PortAudio stream
            PaStreamParameters outputParameters;
            std::memset(&outputParameters, 0, sizeof(outputParameters));
            outputParameters.device = device_index_;

            const PaDeviceInfo*  deviceInfo = Pa_GetDeviceInfo(outputParameters.device);
            const PaHostApiInfo* hostInfo   = Pa_GetHostApiInfo(deviceInfo->hostApi);

            CASPAR_LOG(info) << L"Using PortAudio device: \"" << deviceInfo->name << L"\" ("
                             << (hostInfo ? hostInfo->name : "Unknown") << L")";

            // Verify it's actually ASIO
            if (hostInfo && hostInfo->type != paASIO) {
                CASPAR_LOG(warning) << L"WARNING: Selected device is not ASIO! (Host API: " << hostInfo->name << L")";
                CASPAR_LOG(warning) << L"Multi-channel output may not work correctly.";
            }

            if (output_channels_ > deviceInfo->maxOutputChannels) {
                CASPAR_LOG(warning) << L"Requested " << output_channels_ << L" channels but device only has "
                                    << deviceInfo->maxOutputChannels << L". Adjusting.";
                output_channels_ = deviceInfo->maxOutputChannels;
            }

            outputParameters.channelCount              = output_channels_;
            outputParameters.sampleFormat              = paFloat32;
            outputParameters.suggestedLatency          = deviceInfo->defaultLowOutputLatency;
            outputParameters.hostApiSpecificStreamInfo = nullptr;

            // Check if format is supported
            double  desired_sample_rate = static_cast<double>(format_desc_.audio_sample_rate);
            PaError sup                 = Pa_IsFormatSupported(nullptr, &outputParameters, desired_sample_rate);

            if (sup != paNoError) {
                CASPAR_LOG(error) << L"========================================";
                CASPAR_LOG(error) << L"ASIO SAMPLE RATE MISMATCH!";
                CASPAR_LOG(error) << L"========================================";
                CASPAR_LOG(error) << L"CasparCG wants: " << format_desc_.audio_sample_rate << L" Hz";
                CASPAR_LOG(error) << L"ASIO driver error: " << Pa_GetErrorText(sup);
                CASPAR_LOG(error) << L"";
                CASPAR_LOG(error) << L"SOLUTION:";
                CASPAR_LOG(error) << L"1. Open ASIO control panel for: " << deviceInfo->name;
                CASPAR_LOG(error) << L"2. Set sample rate to: " << format_desc_.audio_sample_rate << L" Hz";
                CASPAR_LOG(error) << L"3. Apply settings and restart CasparCG";
                CASPAR_LOG(error) << L"========================================";

                CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("ASIO sample rate mismatch"));
            }

            CASPAR_LOG(info) << L"âœ“ ASIO format check passed: " << format_desc_.audio_sample_rate << L" Hz";

            // Open stream
            PaError err = Pa_OpenStream(&stream_,
                                        nullptr,
                                        &outputParameters,
                                        desired_sample_rate,
                                        buffer_size_frames_,
                                        paClipOff,
                                        &portaudio_consumer::pa_callback_wrapper,
                                        this);

            if (err != paNoError) {
                CASPAR_LOG(error) << L"Failed to open PortAudio stream: " << Pa_GetErrorText(err);
                CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(std::string("Failed to open PortAudio stream: ") +
                                                                      Pa_GetErrorText(err)));
            }

            // Get stream info
            const PaStreamInfo* streamInfo    = Pa_GetStreamInfo(stream_);
            double              pa_latency_ms = streamInfo->outputLatency * 1000.0;

            CASPAR_LOG(info) << L"PortAudio stream opened:";
            CASPAR_LOG(info) << L"  Sample rate: " << streamInfo->sampleRate << L" Hz";
            CASPAR_LOG(info) << L"  ASIO reported latency: " << pa_latency_ms << L" ms";
            CASPAR_LOG(info) << L"  Buffer size: " << buffer_size_frames_ << L" frames ("
                             << (buffer_size_frames_ * 1000.0 / format_desc_.audio_sample_rate) << L" ms)";

            // Calculate FIFO size - REDUCE to 50ms for lower latency
            const size_t fifo_ms = static_cast<size_t>(
                env::properties().get(L"configuration.portaudio.fifo-ms", 50)); // Reduced from 100ms

            const size_t fifo_samples = static_cast<size_t>(
                (static_cast<double>(format_desc_.audio_sample_rate) * (static_cast<double>(fifo_ms) / 1000.0)) *
                static_cast<double>(output_channels_));

            const size_t min_samples = buffer_size_frames_ * output_channels_ * 4;
            audio_fifo_.reset(std::max<size_t>(fifo_samples, min_samples));

            double fifo_latency_ms = (audio_fifo_.capacity() / static_cast<double>(output_channels_) /
                                      static_cast<double>(format_desc_.audio_sample_rate)) *
                                     1000.0;

            CASPAR_LOG(info) << L"PortAudio FIFO: " << audio_fifo_.capacity() << L" samples (" << fifo_latency_ms
                             << L" ms capacity)";

            // Pre-fill FIFO to 75% to ensure callbacks never starve initially
            size_t             prefill_samples = (audio_fifo_.capacity() * 3) / 4;
            std::vector<float> silence(prefill_samples, 0.0f);
            size_t             prefilled = audio_fifo_.write(silence.data(), prefill_samples);

            CASPAR_LOG(info) << L"Pre-filled FIFO with " << prefilled << L" silent samples ("
                             << (prefilled / output_channels_ / format_desc_.audio_sample_rate * 1000.0) << L" ms) = "
                             << (prefilled * 100 / audio_fifo_.capacity()) << L"% full";

            // Start stream
            err = Pa_StartStream(stream_);
            if (err != paNoError) {
                CASPAR_LOG(error) << L"Failed to start PortAudio stream: " << Pa_GetErrorText(err);
                Pa_CloseStream(stream_);
                stream_ = nullptr;
                CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(
                                           std::string("Failed to start PortAudio stream: ") + Pa_GetErrorText(err)));
            }

            CASPAR_LOG(info) << L"âœ“ PortAudio ASIO stream started successfully";
        });

        // Start audio dispatch thread
        start_audio_dispatch_thread();
    }

    std::future<bool> send(core::video_field field, core::const_frame frame) override
    {
        auto current_frame = frame_counter_.fetch_add(1) + 1;

        // Extract audio FIRST, before any sleeping
        std::vector<int32_t> audio_data;
        bool                 has_audio = false;

        if (frame.audio_data() && frame.audio_data().size() > 0) {
            auto audio_array = frame.audio_data();
            audio_data.assign(audio_array.begin(), audio_array.end());
            has_audio = true;
        }

        // Schedule audio IMMEDIATELY (before video sleep)
        // This gives dispatch thread time to process it ahead of video display
        if (has_audio && !audio_data.empty()) {
            executor_.begin_invoke([=] {
                schedule_audio_for_frame(
                    current_frame, audio_data, format_desc_.audio_sample_rate, format_desc_.audio_channels);
            });
        }

        // NOW sleep for video frame pacing
        auto target_time =
            channel_start_time_ + std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(
                                      static_cast<double>(current_frame) * frame_duration_seconds_));

        auto now = std::chrono::high_resolution_clock::now();
        if (target_time > now) {
            std::this_thread::sleep_until(target_time);
        }

        // Update diagnostics
        executor_.begin_invoke([=] {
            const float fifo_fill = (audio_fifo_.capacity() > 0) ? static_cast<float>(audio_fifo_.available_to_read()) /
                                                                       static_cast<float>(audio_fifo_.capacity())
                                                                 : 0.0f;

            graph_->set_value("fifo-fill", fifo_fill);
            graph_->set_value("tick-time", perf_timer_.elapsed() * format_desc_.fps * 0.5);
            perf_timer_.restart();
        });

        return make_ready_future(true);
    }

    std::wstring print() const override
    {
        return L"portaudio[" + std::to_wstring(channel_index_) + L"|" + format_desc_.name + L"|" +
               std::to_wstring(output_channels_) + L"ch]";
    }

    std::wstring name() const override { return L"portaudio"; }

    bool has_synchronization_clock() const override { return true; }

    int index() const override { return 501; }

    core::monitor::state state() const override
    {
        core::monitor::state state;
        state["sync-mode"]               = std::string("video-scheduled");
        state["frame-counter"]           = static_cast<int>(frame_counter_.load());
        state["latency-compensation-ms"] = audio_latency_compensation_ms_.load();
        state["output-channels"]         = output_channels_;
        state["callback-count"]          = static_cast<int>(callback_count_.load());
        state["packets-dispatched"]      = static_cast<int>(packets_dispatched_.load());
        state["fifo-overruns"]           = static_cast<int>(overrun_count_.load());
        state["fifo-underruns"]          = static_cast<int>(underrun_count_.load());
        state["fifo-fill-samples"]       = static_cast<int>(audio_fifo_.available_to_read());
        state["fifo-capacity"]           = static_cast<int>(audio_fifo_.capacity());
        return state;
    }
};

// Factory functions
spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>&     params,
                                                      const core::video_format_repository& format_repository,
                                                      const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                                                      const core::channel_info& channel_info)
{
    if (params.empty() || !boost::iequals(params.at(0), L"PORTAUDIO"))
        return core::frame_consumer::empty();

    return spl::make_shared<portaudio_consumer>();
}

spl::shared_ptr<core::frame_consumer>
create_preconfigured_consumer(const boost::property_tree::wptree&                      ptree,
                              const core::video_format_repository&                     format_repository,
                              const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                              const core::channel_info&                                channel_info)
{
    return spl::make_shared<portaudio_consumer>();
}

}} // namespace caspar::portaudio
