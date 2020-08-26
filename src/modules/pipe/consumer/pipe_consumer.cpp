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
#include <errhandlingapi.h>
#endif


namespace caspar { namespace pipe {

struct configuration
{
    std::wstring    name            = L"Pipe consumer";
    std::wstring    video_pipe_name = L"";
    std::wstring    audio_pipe_name = L"";
    bool            use_video_pipe  = false;
    bool            use_audio_pipe  = false;
    bool            single_pipe     = false;
    double          buffer_size     = 3.0;
    int             pipe_index      = 0;
};

struct pipe_consumer
{
    const configuration     config_;
    core::video_format_desc format_desc_;
    int                     channel_index_;

    spl::shared_ptr<diagnostics::graph> graph_video_;
    spl::shared_ptr<diagnostics::graph> graph_audio_;
    caspar::timer                       frame_timer_video_;
    caspar::timer                       frame_timer_audio_;

    tbb::concurrent_bounded_queue<core::const_frame> frame_buffer_video_;
    tbb::concurrent_bounded_queue<core::const_frame> frame_buffer_audio_;

    std::atomic<bool> is_running_{true};
    std::atomic<bool> ready_for_frames_{false};
    std::thread       thread_video_;
    std::thread       thread_audio_;

    pipe_consumer(const pipe_consumer&) = delete;
    pipe_consumer& operator=(const pipe_consumer&) = delete;

  public:
    pipe_consumer(const configuration& config, const core::video_format_desc& format_desc, int channel_index)
        : config_(config)
        , format_desc_(format_desc)
        , channel_index_(channel_index)
    {
        // log pipe names
        if (config_.use_video_pipe) {
            CASPAR_LOG(debug) << print() << L": using video pipe name: " << config_.video_pipe_name;
            if (config_.single_pipe) {
                CASPAR_LOG(debug) << print()
                                  << L": Single pipe enabled. Both video and audio (in that order) will be sent down "
                                     L"the above pipe).";
            }
        }
        if (config_.use_audio_pipe) {
            CASPAR_LOG(debug) << print() << L": using audio pipe name: " << config_.audio_pipe_name;
            if (config_.single_pipe) {
                CASPAR_LOG(debug) << print()
                                  << L": Single pipe enabled. Both audio and video (in that order) will be sent down "
                                     L"the above pipe).";
            }
        }

        // If no pipes were specified, don't start the consumer and log an error.
        if (!config_.use_audio_pipe && !config_.use_video_pipe)
        {
            CASPAR_LOG(error) << print()
                              << L": No pipe name specified. You must specify one or both of VIDEO_PIPE and AUDIO_PIPE "
                                 L"parameters followed by the pipe name (e.g. VIDEO_PIPE \\\\.\\pipe\\CasparCGVideo)";
            is_running_ = false;
            return;
        }

        // set buffer size
        frame_buffer_video_.set_capacity((int)std::round(config_.buffer_size * format_desc_.fps));
        frame_buffer_audio_.set_capacity((int)std::round(config_.buffer_size * format_desc_.fps));

        // Start the thread(s)
        if (config_.use_video_pipe) {
            graph_video_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
            graph_video_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
            graph_video_->set_color("input", diagnostics::color(0.7f, 0.4f, 0.4f));
            graph_video_->set_text(print(L"video"));
            diagnostics::register_graph(graph_video_);

            thread_video_ = std::thread([this] {
                // open the pipe (this will block until the receiver opens the pipe)
                HANDLE pipe_h;
                if (!openPipe(L"video", config_.video_pipe_name, pipe_h)) {
                    return;
                }

                try {
                    while (is_running_) {
                        core::const_frame in_frame;

                        while (!frame_buffer_video_.try_pop(in_frame) && is_running_) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(2));
                        }

                        graph_video_->set_value("input",
                                                static_cast<double>(frame_buffer_video_.size() + 0.001) /
                                                    frame_buffer_video_.capacity());

                        if (!in_frame) {
                            is_running_ = false;
                            break;
                        }

                        bool successV = writeVideo(L"video", in_frame, pipe_h);
                        bool successA = true;
                        if (config_.single_pipe && successV) {
                            // Note: avType is set as "video" here even though we are calling writeAudio.
                            //       This is so the error message to internal calls to print() are correct.
                            successA = writeAudio(L"video", in_frame, pipe_h);
                        }

                        if (!successA || !successV) {
                            // Log the dropped frame
                            graph_video_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
                            // Then terminate the pipe consumer because there is no recovery from a failed/partial write
                            // when the write to the pipe is synchronous and should thus block until completion
                            is_running_ = false;
                            break;
                        }
                        
                        graph_video_->set_value("frame-time", frame_timer_video_.elapsed() * format_desc_.fps * 0.5);
                        frame_timer_video_.restart();
                    }
                } catch (tbb::user_abort&) {
                    // we are ending
                } catch (...) {
                    CASPAR_LOG_CURRENT_EXCEPTION();
                    is_running_ = false;
                }
                // close handle
                closePipe(L"video", pipe_h);
            });
        }
        if (config_.use_audio_pipe) {
            graph_audio_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
            graph_audio_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
            graph_audio_->set_color("input", diagnostics::color(0.7f, 0.4f, 0.4f));
            graph_audio_->set_text(print(L"audio"));
            diagnostics::register_graph(graph_audio_);

            thread_audio_ = std::thread([this] {
                // open the pipe (this will block until the receiver opens the pipe)
                HANDLE pipe_h;
                if (!openPipe(L"audio", config_.audio_pipe_name, pipe_h)) {
                    return;
                }

                try {
                    while (is_running_) {
                        core::const_frame in_frame;

                        while (!frame_buffer_audio_.try_pop(in_frame) && is_running_) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(2));
                        }
                        graph_audio_->set_value("input",
                                                static_cast<double>(frame_buffer_audio_.size() + 0.001) /
                                                    frame_buffer_audio_.capacity());

                        if (!in_frame) {
                            is_running_ = false;
                            break;
                        }

                        bool successA = writeAudio(L"audio", in_frame, pipe_h);
                        bool successV = true;
                        if (config_.single_pipe && successA) {
                            // Note: avType is set as "audio" here even though we are calling writeVideo.
                            //       This is so the error message to internal calls to print() are correct.
                            successV = writeVideo(L"audio", in_frame, pipe_h);
                        }

                        if (!successA || !successV) {
                            // Log the dropped frame
                            graph_audio_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
                            // Then terminate the pipe consumer because there is no recovery from a failed/partial write
                            // when the write to the pipe is synchronous and should thus block until completion
                            is_running_ = false;
                            break;
                        }
                        graph_audio_->set_value("frame-time", frame_timer_audio_.elapsed() * format_desc_.fps * 0.5);
                        frame_timer_audio_.restart();
                    }
                } catch (tbb::user_abort&) {
                    // we are ending
                } catch (...) {
                    CASPAR_LOG_CURRENT_EXCEPTION();
                    is_running_ = false;
                }
                // close handle
                closePipe(L"audio", pipe_h);
            });
        }
    }

    ~pipe_consumer()
    {
        is_running_ = false;
        frame_buffer_video_.abort();
        frame_buffer_audio_.abort();
        // Some pipe functions will block - This should unblock them so that join() succeeds
        if (config_.use_video_pipe) {
            if (!CancelSynchronousIo(thread_video_.native_handle())) {
                DWORD lastError = GetLastError();
                if (lastError != ERROR_NOT_FOUND) {
                    CASPAR_LOG(error) << print(L"video")
                                      << L" Failed to cancel synchronous IO (CasparCG may now lock up). Error code: "
                                      << std::to_wstring(lastError);
                }
            }
        }
        if (config_.use_audio_pipe) {
            if (!CancelSynchronousIo(thread_audio_.native_handle())) {
                DWORD lastError = GetLastError();
                if (lastError != ERROR_NOT_FOUND) {
                    CASPAR_LOG(error) << print(L"audio")
                                      << L" Failed to cancel synchronous IO (CasparCG may now lock up). Error code: "
                                      << std::to_wstring(lastError);
                }
            }
        }
        if (config_.use_video_pipe) {
            thread_video_.join();
        }
        if (config_.use_audio_pipe) {
            thread_audio_.join();
        }
        CASPAR_LOG(debug) << print() << L" PIPE consumer destructed";
    }

    bool openPipe(std::wstring avType, std::wstring pipeName, HANDLE& pipe_h)
    {
        int max_retries = 20;
        int retries     = 0;
        DWORD lastError = 0;
        // Create pipe
        CASPAR_LOG(info) << print(avType) << L" Creating pipe...";
        // If the pipe creation fails because of ERROR_PIPE_BUSY, loop for a couple of seconds to
        // see if it becomes free.
        do {
            pipe_h = CreateNamedPipeW(pipeName.c_str(),
                                      PIPE_ACCESS_DUPLEX,
                                      PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                      1,
                                      0,
                                      0,
                                      0,
                                      NULL);
            if (pipe_h == INVALID_HANDLE_VALUE) {
                lastError = GetLastError();
                if (lastError == ERROR_PIPE_BUSY) {
                    CASPAR_LOG(debug) << print(avType) << L" Pipe creation failed (PIPE_BUSY). Will retry in 100ms.";
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    retries++;
                }
            }
            
        } while (lastError == ERROR_PIPE_BUSY && retries <= max_retries);

        // Handle error on pipe creation
        if (pipe_h == INVALID_HANDLE_VALUE) {
            std::wstring msg = L"Error code: " + std::to_wstring(lastError) + L".";
            // print message for the most common errors
            if (lastError == ERROR_INVALID_NAME) {
                msg += L" Error message: The pipe name is invalid.";
            } else if (lastError == ERROR_BAD_PATHNAME) {
                msg += L" Error message: The pipe path is invalid";
            }
            CASPAR_LOG(error) << print(avType) << L" Failed to create named pipe " << pipeName << L" . " << msg;
            // close handle
            closePipe(avType, pipe_h);
            is_running_ = false;
            return false;
        }

        // Connect pipe
        CASPAR_LOG(info) << print(avType) << L" Pipe created. Connecting to pipe...";
        if (!ConnectNamedPipe(pipe_h, NULL)) {
            lastError = GetLastError();
            CASPAR_LOG(error) << print(avType) << L" Failed to connect to named pipe " << pipeName << L" . Error code: "
                              << std::to_wstring(lastError);
            // close handle
            closePipe(avType, pipe_h);
            is_running_ = false;
            return false;
        }
        CASPAR_LOG(info) << print(avType) << L" Pipe connected";
        ready_for_frames_ = true;
        return true;
    }

    bool writeVideo(std::wstring avType, const core::const_frame& in_frame, HANDLE& pipe_h)
    {
        DWORD numBytesWritten = 0;
        DWORD numBytesToWrite = format_desc_.width * format_desc_.height * 4; // assumes RGBA

        // Check video frame format is supported
        if (core::pixel_format::bgra != in_frame.pixel_format_desc().format) {
            CASPAR_LOG(error) << print(avType) << L" Unsupported pixel format "
                              << std::to_string((int)in_frame.pixel_format_desc().format);
            is_running_ = false;
            return false;
        }

        if (in_frame.image_data(0).size() != numBytesToWrite) {
            CASPAR_LOG(error) << print(avType) << L" Unsupported frame video array format";
            is_running_ = false;
            return false;
        }

        // send frame data
        bool writeSuccess = WriteFile(pipe_h, in_frame.image_data(0).data(), numBytesToWrite, &numBytesWritten, NULL);
        if (!writeSuccess || numBytesToWrite != numBytesWritten) {
            DWORD lastError = GetLastError();
            CASPAR_LOG(error) << print(avType) << L" Video frame dropped (failed to write frame to pipe). Error code: "
                              << std::to_wstring(lastError);
            return false;
        }
        return true;
    }

    bool writeAudio(std::wstring avType, const core::const_frame& in_frame, HANDLE& pipe_h)
    {
        DWORD numBytesWritten = 0;
        DWORD numBytesToWrite = 0;

        // check there is audio data to send
        if (in_frame.audio_data().size() == 0) {
            CASPAR_LOG(warning) << print(avType) << L" No audio data in frame. Skipping writing.";
            return true;
        }

        // send frame data
        numBytesToWrite = static_cast<DWORD>(in_frame.audio_data().size() * sizeof(in_frame.audio_data().data()[0]));
        bool writeSuccess = WriteFile(pipe_h, in_frame.audio_data().data(), numBytesToWrite, &numBytesWritten, NULL);
        if (!writeSuccess || numBytesToWrite != numBytesWritten) {
            DWORD lastError = GetLastError();
            CASPAR_LOG(error) << print(avType) << L" Audio frame dropped (failed to write frame to pipe). Error code: "
                              << std::to_wstring(lastError);
            return false;
        }
        return true;
    }

    bool closePipe(std::wstring avType, HANDLE& handle)
    {
        if (!CloseHandle(handle)) {
            DWORD lastError = GetLastError();
            CASPAR_LOG(error) << print(avType) << L" Failed to close named pipe. Error code: "
                              << std::to_wstring(lastError);
            return false;
        }
        return true;
    }

    std::future<bool> send(const core::const_frame& frame)
    {
        bool vPushResult = true;
        bool aPushResult = true;
        // silently drop frames until the pipe is connected
        if (ready_for_frames_) {
            if (config_.use_video_pipe) {
                vPushResult = frame_buffer_video_.try_push(frame);
                if (!vPushResult) {
                    graph_video_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
                }
            }
            if (config_.use_audio_pipe) {
                aPushResult = frame_buffer_audio_.try_push(frame);
                if (!aPushResult) {
                    graph_audio_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
                }
            }
            // If only one dropped a frame, warn in the log that the A/V sync will be out
            if (!(!vPushResult && !aPushResult)) {
                if (!vPushResult) {
                    CASPAR_LOG(warning) << print()
                                        << L" A video frame was dropped but an audio frame was not. The audio-video "
                                            L"sync will be out.";
                }
                else if (!aPushResult) {
                    CASPAR_LOG(warning) << print()
                                        << L" An audio frame was dropped but a video frame was not. The audio-video "
                                            L"sync will be out.";
                }
            }
        }
        return make_ready_future(is_running_.load());
    }

    std::wstring channel_and_format() const
    {
        return L"[" + std::to_wstring(channel_index_) + L"|" + format_desc_.name + L"]";
    }

    std::wstring print(std::wstring avType) const
    {
        std::wstring msg = config_.name + L" " + channel_and_format();
        if (!boost::iequals(avType, L"")) {
            msg += L"[" + avType;
            if (config_.single_pipe) {
                if (boost::iequals(avType, L"video")) {
                    msg += L"+audio";
                } else {
                    msg += L"+video";
                }
            }
            msg += L"]";
        }
        return msg;
    }

    std::wstring print() { return print(L""); }
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

    config.single_pipe = contains_param(L"SINGLE_PIPE", params);

    // Read in pipe names
    if (contains_param(L"VIDEO_PIPE", params)) {
        config.video_pipe_name = get_param(L"VIDEO_PIPE", params);
        config.use_video_pipe   = true;
    }
    if (contains_param(L"AUDIO_PIPE", params)) {
        config.audio_pipe_name = get_param(L"AUDIO_PIPE", params);
        config.use_audio_pipe   = true;
    }

    // If separate pipes were specified for A/V but a single pipe was also requested, ignore the AUDIO_PIPE
    // parameter
    if (config.single_pipe && config.use_audio_pipe && config.use_video_pipe) {
        CASPAR_LOG(warning)
            << config.name
            << L": Ignoring AUDIO_PIPE parameter (conflicts with SINGLE_PIPE when VIDEO_PIPE also specified).";
        config.use_audio_pipe = false;
    }
    
    if (contains_param(L"BUFFER_SIZE", params)) {
        try{
            config.buffer_size = std::stod(get_param(L"BUFFER_SIZE", params));
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
    config.use_video_pipe  = boost::iequals(config.video_pipe_name, L"") ? false : true;
    config.use_audio_pipe  = boost::iequals(config.audio_pipe_name, L"") ? false : true;
    config.single_pipe     = ptree.get(L"single-pipe", config.single_pipe);
    config.buffer_size     = ptree.get(L"buffer-size", config.buffer_size);

    // If separate pipes were specified for A/V but a single pipe was also requested, ignore the AUDIO_PIPE
    // parameter
    if (config.single_pipe && config.use_audio_pipe && config.use_video_pipe) {
        CASPAR_LOG(warning)
            << config.name
            << L": Ignoring AUDIO_PIPE parameter (conflicts with SINGLE_PIPE when VIDEO_PIPE also specified).";
        config.use_audio_pipe = false;
    }

    return spl::make_shared<pipe_consumer_proxy>(config);
}

}} // namespace caspar::pipe
