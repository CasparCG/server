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
 * Author: Philip Starkey, https://github.com/philipstarkey
 * Author: Robert Nagy, ronag89@gmail.com
 */

#include "pipe_consumer.h"

#include <common/array.h>
#include <common/diagnostics/graph.h>
#include <common/future.h>
#include <common/log.h>
#include <common/param.h>
#include <common/timer.h>
#include <common/utf.h>

#include <core/consumer/frame_consumer.h>
#include <core/frame/frame.h>
#include <core/frame/pixel_format.h>
#include <core/video_format.h>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>

#include <tbb/concurrent_queue.h>

#include <memory>
#include <thread>
#include <utility>
#include <vector>

#if defined(_MSC_VER)
#include <windows.h>
#include <fileapi.h>
#include <winbase.h>
#include <namedpipeapi.h>
#include <handleapi.h>
#include <ioapiset.h>

//#pragma warning(push)
//#pragma warning(disable : 4244)
#endif


namespace caspar { namespace pipe {

struct configuration
{
    std::wstring    name            = L"Pipe consumer";
    std::wstring    video_pipe_name = L"\\\\.\\pipe\\CasparCGVideo";
    std::wstring    audio_pipe_name = L"\\\\.\\pipe\\CasparCGAudio";
    bool            include_video   = true;
    bool            include_audio   = false;
    int             buffer_size     = 3;
    int             pipe_index      = 0;
};

struct pipe_consumer
{
    const configuration     config_;
    core::video_format_desc format_desc_;
    int                     channel_index_;

    spl::shared_ptr<diagnostics::graph> graph_;
    caspar::timer                       tick_timer_;

    tbb::concurrent_bounded_queue<core::const_frame> frame_buffer_;

    std::atomic<bool> is_running_{true};
    std::atomic<bool> ready_for_frames_{false};
    std::thread       thread_;

    pipe_consumer(const pipe_consumer&) = delete;
    pipe_consumer& operator=(const pipe_consumer&) = delete;

  public:
    pipe_consumer(const configuration& config, const core::video_format_desc& format_desc, int channel_index)
        : config_(config)
        , format_desc_(format_desc)
        , channel_index_(channel_index)
    {

        frame_buffer_.set_capacity((int)(config_.buffer_size*format_desc_.fps));

        graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        graph_->set_text(print());
        diagnostics::register_graph(graph_);

        thread_ = std::thread([this] {
            // Create pipe (if requested)
            CASPAR_LOG(info) << print() << L" Creating pipe...";
            HANDLE pipe_h = CreateNamedPipeW(config_.video_pipe_name.c_str(),
                                             PIPE_ACCESS_DUPLEX,
                                             PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                             1,
                                             0,
                                             0,
                                             0,
                                             NULL);
            if (pipe_h == INVALID_HANDLE_VALUE) {
                CASPAR_LOG(error) << print() << L" Failed to create named pipe "
                                  << config_.video_pipe_name; // TODO: includeGetLastError
                // close handle
                closePipe(pipe_h);
                is_running_ = false;
                return;
            }

            // Connect pipe
            CASPAR_LOG(info) << print() << L" Pipe created. Connecting to pipe...";
            if (!ConnectNamedPipe(pipe_h, NULL)) {
                CASPAR_LOG(error) << print() << L" Failed to connect to named pipe "
                                  << config_.video_pipe_name; // TODO: includeGetLastError
                // close handle
                closePipe(pipe_h);
                is_running_ = false;
                return;
            }
            CASPAR_LOG(info) << print() << L" Pipe connected";
            ready_for_frames_ = true;

            try {
                while (is_running_) {
                    core::const_frame in_frame;
                    DWORD             numBytesWritten = 0;
                    DWORD             numBytesToWrite = format_desc_.width*format_desc_.height*4; //assumes RGBA

                    while (!frame_buffer_.try_pop(in_frame) && is_running_) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(2));
                    }

                    if (!in_frame) {
                        is_running_ = false;
                        break;
                    }

                    if (core::pixel_format::bgra != in_frame.pixel_format_desc().format) {
                        CASPAR_LOG(error) << print() << L" Unsupported pixel format " << std::to_string((int)in_frame.pixel_format_desc().format);
                        is_running_ = false;
                        break;
                    }

                    if (in_frame.image_data(0).size() != numBytesToWrite) {
                        CASPAR_LOG(error) << print() << L" Unsupported frame video array format";
                        is_running_ = false;
                        break;
                    }

                    // send frame data
                    bool writeSuccess =
                        WriteFile(pipe_h, in_frame.image_data(0).data(), numBytesToWrite, &numBytesWritten, NULL);
                    if (!writeSuccess || numBytesToWrite != numBytesWritten) {
                        CASPAR_LOG(warning) << print() << L" Frame dropped (failed to write frame to pipe)"; // TODO: Add GetLastError
                        is_running_ = false;
                        break;
                    }

                    graph_->set_value("tick-time", tick_timer_.elapsed() * format_desc_.fps * 0.5);
                    tick_timer_.restart();
                }
            } catch (tbb::user_abort&) {
                // we are ending
            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
                is_running_ = false;
            }
            // close handle
            closePipe(pipe_h);
        });
    }

    ~pipe_consumer()
    {
        is_running_ = false;
        frame_buffer_.abort();
        // Some pipe functions will block - This should unblock them so that join() succeeds
        if (!CancelSynchronousIo(thread_.native_handle())) {
            DWORD lastError = GetLastError();
            if (lastError != ERROR_NOT_FOUND) {
                CASPAR_LOG(error)
                    << print()
                    << L" Failed to cancel synchronous IO (CasparCG may now lock up)"; // TODO: Add GetLastError
            }
        }
        thread_.join();
    }

    bool closePipe(HANDLE handle)
    {
        if (!CloseHandle(handle)) {
            CASPAR_LOG(error) << print() << L" Failed to close named pipe";
            return false;
        }
        return true;
    }

    std::future<bool> send(const core::const_frame& frame)
    {
        // silently drop frames until the pipe is connected
        if (ready_for_frames_) {
            if (!frame_buffer_.try_push(frame)) {
                graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
            }
        }
        return make_ready_future(is_running_.load());
    }

    std::wstring channel_and_format() const
    {
        return L"[" + std::to_wstring(channel_index_) + L"|" + format_desc_.name + L"]";
    }

    std::wstring print() const { return config_.name + L" " + channel_and_format(); }
};

struct pipe_consumer_proxy : public core::frame_consumer
{
    const configuration              config_;
    std::unique_ptr<pipe_consumer> consumer_;

  public:
    explicit pipe_consumer_proxy(configuration config)
        : config_(std::move(config))
    {
    }

    // frame_consumer

    void initialize(const core::video_format_desc& format_desc, int channel_index) override
    {
        consumer_.reset();
        consumer_ = std::make_unique<pipe_consumer>(config_, format_desc, channel_index);
    }

    std::future<bool> send(core::const_frame frame) override { return consumer_->send(frame); }

    std::wstring print() const override { return consumer_ ? consumer_->print() : L"[pipe_consumer]"; }

    std::wstring name() const override { return L"pipe"; }

    bool has_synchronization_clock() const override { return false; }

    int index() const override { return 1100 + config_.pipe_index; }
};

spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>&                         params,
                                                      const std::vector<spl::shared_ptr<core::video_channel>>& channels)
{
    if (params.empty() || !boost::iequals(params.at(0), L"PIPE")) {
        return core::frame_consumer::empty();
    }

    configuration config;

    if (contains_param(L"NAME", params)) {
        config.name = get_param(L"NAME", params);
    }

    if (params.size() > 1) {
        try {
            config.pipe_index = std::stoi(params.at(1));
        } catch (...) {
            CASPAR_LOG(warning) << config.name << L": Pipe invalid pipe index specified, using 0.";
        }
    }

    // Default: include video only
    config.include_video = !contains_param(L"EXCLUDE_VIDEO", params); 
    config.include_audio = contains_param(L"INCLUDE_AUDIO", params);

    if (contains_param(L"VIDEO_PIPE_NAME", params)) {
        config.video_pipe_name = get_param(L"VIDEO_PIPE_NAME", params);
    } else if (config.include_video) {
        CASPAR_LOG(info) << config.name << L": using default video pipe name: " << config.video_pipe_name;
    }
    if (contains_param(L"AUDIO_PIPE_NAME", params)) {
        config.audio_pipe_name = get_param(L"AUDIO_PIPE_NAME", params);
    } else if (config.include_audio) {
        CASPAR_LOG(info) << config.name << L": using default audio pipe name: " << config.audio_pipe_name;
    }

    if (contains_param(L"BUFFER_SIZE", params)) {
        try{
            config.buffer_size = std::stoi(get_param(L"BUFFER_SIZE", params));
        }
        catch (...) {
            CASPAR_LOG(warning) << config.name << L": param BUFFER_SIZE ignored.";
        }
    }

    return spl::make_shared<pipe_consumer_proxy>(config);
}

spl::shared_ptr<core::frame_consumer>
create_preconfigured_consumer(const boost::property_tree::wptree&                      ptree,
                              const std::vector<spl::shared_ptr<core::video_channel>>& channels)
{
    configuration config;
    config.name            = ptree.get(L"name", config.name);
    config.pipe_index      = ptree.get(L"index", config.pipe_index + 1) - 1;
    config.video_pipe_name = ptree.get(L"video-pipe-name", config.video_pipe_name);
    config.audio_pipe_name = ptree.get(L"audio-pipe-name", config.audio_pipe_name);
    config.include_video   = ptree.get(L"include-video", config.include_video);
    config.include_audio   = ptree.get(L"include-audio", config.include_audio);
    config.buffer_size     = ptree.get(L"buffer-size", config.buffer_size);

    return spl::make_shared<pipe_consumer_proxy>(config);
}

}} // namespace caspar::pipe
