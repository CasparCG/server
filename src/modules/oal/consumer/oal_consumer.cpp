/*
 * Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
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

#include "oal_consumer.h"

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

#include <boost/property_tree/ptree.hpp>

#include <tbb/concurrent_queue.h>

extern "C" {
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

#include <algorithm>
#include <atomic>
#include <cctype>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include <AL/al.h>
#include <AL/alc.h>

namespace caspar::oal {

using namespace std::chrono_literals;

class device
{
    ALCdevice*  device_  = nullptr;
    ALCcontext* context_ = nullptr;

  public:
    explicit device(std::wstring device_name)
    {
        auto norm = [](std::string s) {
            s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            return s;
        };

        std::string found_name;

        if (device_name.size()) {
            if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT")) {

                auto match = norm(u8(device_name));

                auto enum_all_ext = alcIsExtensionPresent(nullptr, "ALC_ENUMERATE_ALL_EXT");
                auto names = alcGetString(nullptr, enum_all_ext ? ALC_ALL_DEVICES_SPECIFIER : ALC_DEVICE_SPECIFIER);

                CASPAR_LOG(info) << "-------- OpenAL Devices -------";

                // `names` contains multiple null-terminated device names
                for (auto p = names; *p; p += strlen(p) + 1) {
                    std::string found{p};
                    CASPAR_LOG(info) << found;

                    if (match == norm(found)) found_name = found;
                }

                CASPAR_LOG(info) << "-------- OpenAL Devices -------";

                if (found_name.size())
                    CASPAR_LOG(info) << "Using OpenAL device: " << found_name;
                else
                    CASPAR_LOG(warning) << "Specified OpenAL device not found - using default";
            } else {
                CASPAR_LOG(info) << "OpenAL device enumeration not supported - using default";
            }
        } else {
            CASPAR_LOG(info) << "Using default OpenAL device";
        }

        device_ = alcOpenDevice(found_name.size() ? found_name.data() : nullptr);
        if (!device_)
            CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("Failed to initialize audio device."));

        context_ = alcCreateContext(device_, nullptr);
        if (!context_)
            CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("Failed to create audio context."));

        if (!alcMakeContextCurrent(context_))
            CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("Failed to activate audio context."));
    }

    ~device()
    {
        alcMakeContextCurrent(nullptr);
        if (context_) alcDestroyContext(context_);
        if (device_) alcCloseDevice(device_);
    }

    ALCdevice* get() { return device_; }
};

struct oal_consumer : public core::frame_consumer
{
    spl::shared_ptr<diagnostics::graph> graph_;
    caspar::timer                       perf_timer_;
    int                                 channel_index_ = -1;

    core::video_format_desc format_desc_;

    device              device_;
    ALuint              source_ = 0;
    std::vector<ALuint> buffers_;
    std::queue<ALuint>  free_;

    std::mutex          mutex_;
    std::condition_variable free_cv_;
    std::atomic<bool>   stop_{false};

    struct swr_deleter { void operator()(SwrContext* p) { swr_free(&p); } };
    std::unique_ptr<SwrContext, swr_deleter> swr_;

    executor executor_{L"oal_consumer"};

  public:
    explicit oal_consumer(std::wstring device_name) : device_{std::move(device_name)}
    {
        graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
        diagnostics::register_graph(graph_);
    }

    ~oal_consumer() override
    {
        stop_ = true;
        free_cv_.notify_all();

        executor_.invoke([=] {
            if (source_) {
                std::lock_guard guard{mutex_};

                alSourceStop(source_);
                alDeleteSources(1, &source_);
                alDeleteBuffers(static_cast<ALsizei>(buffers_.size()), buffers_.data());
            }
        });
    }

    // frame consumer

    void initialize(const core::video_format_desc& format_desc,
                    const core::channel_info&      channel_info,
                    int                            port_index) override
    {
        format_desc_   = format_desc;
        channel_index_ = channel_info.index;
        graph_->set_text(print());

        executor_.begin_invoke([=] {
            AVChannelLayout from, to;
            av_channel_layout_default(&from, format_desc_.audio_channels);
            av_channel_layout_default(&to, 2); // stereo

            SwrContext* raw;
            swr_alloc_set_opts2(&raw,
                &to,   AV_SAMPLE_FMT_S16, format_desc_.audio_sample_rate,
                &from, AV_SAMPLE_FMT_S32, format_desc_.audio_sample_rate,
                0, nullptr
            );
            swr_.reset(raw);
            swr_init(raw);

            std::lock_guard guard{mutex_};

            buffers_.resize(16);
            alGenBuffers(static_cast<ALsizei>(buffers_.size()), buffers_.data());
            for (auto buf : buffers_) free_.push(buf);

            alGenSources(1, &source_);

            // queue one frame worth of silence to ensure there is something
            // in the OAL queue until we get the next frame
            std::vector<int16_t> silence(format_desc_.audio_cadence[0] * 2, 0); // stereo
            auto buf = free_.front();
            free_.pop();
            alBufferData(buf, AL_FORMAT_STEREO16, silence.data(),
                static_cast<ALsizei>(silence.size() * sizeof(std::int16_t)),
                format_desc_.audio_sample_rate
            );
            alSourceQueueBuffers(source_, 1, &buf);
        });
    }

    std::future<bool> send(core::video_field field, core::const_frame frame) override
    {
        executor_.begin_invoke([this, frame = std::move(frame)] {
            // resample and downmix
            auto&& audio_in = frame.audio_data();
            auto num_samples = static_cast<int>(audio_in.size() / format_desc_.audio_channels);

            std::vector<std::int16_t> audio_out(num_samples * 2); // stereo

            auto from = reinterpret_cast<const std::uint8_t*>(audio_in.data());
            auto to = reinterpret_cast<std::uint8_t*>(audio_out.data());
            swr_convert(swr_.get(), &to, num_samples, &from, num_samples);

            {
                // submit to OAL queue
                std::unique_lock lock{mutex_};
                free_cv_.wait(lock, [this] { return !free_.empty() || stop_; });

                auto buf = free_.front();
                free_.pop();
                alBufferData(buf, AL_FORMAT_STEREO16, audio_out.data(),
                    static_cast<ALsizei>(audio_out.size() * sizeof(std::int16_t)),
                    format_desc_.audio_sample_rate
                );
                alSourceQueueBuffers(source_, 1, &buf);

                ALenum state = 0;
                alGetSourcei(source_, AL_SOURCE_STATE, &state);
                if (state != AL_PLAYING) alSourcePlay(source_);
            }

            graph_->set_value("tick-time", perf_timer_.elapsed() * format_desc_.fps * 0.5);
            perf_timer_.restart();
        });

        return std::async(std::launch::deferred, [this] {
            // wait until one buffer is done playing
            while (!stop_) {
                ALint done = 0;
                {
                    std::lock_guard guard{mutex_};
                    alGetSourcei(source_, AL_BUFFERS_PROCESSED, &done);
                    if (done) {
                        ALuint buf;
                        alSourceUnqueueBuffers(source_, 1, &buf);
                        free_.push(buf);
                    }
                }
                if (done) {
                    free_cv_.notify_one();
                    return true;
                }
                std::this_thread::sleep_for(100us);
            }
            return false;
        });
    }

    std::wstring print() const override
    {
        return L"oal[" + std::to_wstring(channel_index_) + L"|" + format_desc_.name + L"]";
    }

    std::wstring name() const override { return L"system-audio"; }

    bool has_synchronization_clock() const override { return true; }

    int index() const override { return 500; }

    core::monitor::state state() const override
    {
        static const core::monitor::state empty;
        return empty;
    }
};

spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>&     params,
                                                      const core::video_format_repository& format_repository,
                                                      const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                                                      const core::channel_info& channel_info)
{
    if (params.empty() || !(boost::iequals(params.at(0), L"AUDIO") || boost::iequals(params.at(0), L"SYSTEM-AUDIO")))
        return core::frame_consumer::empty();

    return spl::make_shared<oal_consumer>(get_param(L"DEVICE_NAME", params, L""));
}

spl::shared_ptr<core::frame_consumer>
create_preconfigured_consumer(const boost::property_tree::wptree&                      ptree,
                              const core::video_format_repository&                     format_repository,
                              const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                              const core::channel_info&                                channel_info)
{
    return spl::make_shared<oal_consumer>(ptree.get(L"device-name", L""));
}

} // namespace caspar::oal
