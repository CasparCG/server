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
    bool            auto_reconnect  = false;
    bool            realtime        = false;
    bool            existing_pipe   = false;
    bool            from_config     = true;
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
    std::atomic<bool> audio_thread_loop_running_{false};
    std::atomic<bool> video_thread_loop_running_{false};
    std::atomic<int>  tail_av_sync{0};
    std::atomic<int>  head_av_sync{0};
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
        if (!config_.use_audio_pipe && !config_.use_video_pipe) {
            std::wstringstream str;
            if (config_.from_config) {
                str << config.name
                    << L": No pipe name specified. You must specify one or both of video-pipe-name and "
                       L"audio-pipe-name "
                       L"parameters and include a pipe name (e.g. "
                       L"<video-pipe-name>\\\\.\\pipe\\CasparCGVideo</video-pipe-name>)"
                    << std::endl;
            } else {
                str << print()
                    << L": No pipe name specified. You must specify one or both of VIDEO_PIPE and AUDIO_PIPE "
                       L"parameters followed by the pipe name (e.g. VIDEO_PIPE \\\\\\\\.\\\\pipe\\\\CasparCGVideo)"
                    << std::endl;
            }
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(str.str()));
        }
        
        // set buffer size
        frame_buffer_video_.set_capacity((int)std::round(config_.buffer_size * format_desc_.fps));
        frame_buffer_audio_.set_capacity((int)std::round(config_.buffer_size * format_desc_.fps));

        // Create graphs
        if (config_.use_video_pipe || config_.single_pipe) {
            graph_video_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
            graph_video_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
            graph_video_->set_color("input", diagnostics::color(0.7f, 0.4f, 0.4f));
            graph_video_->set_text(print(L"video"));
            diagnostics::register_graph(graph_video_);
        }
        if (config_.use_audio_pipe || config_.single_pipe) {
            graph_audio_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
            graph_audio_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
            graph_audio_->set_color("input", diagnostics::color(0.7f, 0.4f, 0.4f));
            graph_audio_->set_text(print(L"audio"));
            diagnostics::register_graph(graph_audio_);
        }

        // Start the thread(s)
        if (config_.use_video_pipe) {
            thread_video_ = std::thread([this] {
                do {
                    // open the pipe (this will block until the receiver opens the pipe)
                    HANDLE pipe_h;
                    if (!openPipe(L"video", config_.video_pipe_name, pipe_h)) {
                        if (!config_.auto_reconnect) {
                            return;
                        } else {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            continue;
                        }
                    }

                    video_thread_loop_running_ = true;
                    try {
                        while (is_running_ && ready_for_frames_) {
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
                                // Then terminate the pipe consumer because there is no recovery from a failed/partial write
                                // when the write to the pipe is synchronous and should thus block until completion
                                // (unless we should try reconnecting!)
                                if (!config_.auto_reconnect) {
                                    is_running_ = false;
                                }
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

                    // This will prevent frames for backing up in the queue if "realtime" was requested.
                    // It will also force the other thread to reconnect the pipe, which ensures that both pipes
                    // are synchronised.
                    if (config_.realtime) {
                        ready_for_frames_ = false;
                        head_av_sync      = 0;

                        // empty the queue
                        // We don't use clear() here because it is thread unsafe.
                        core::const_frame throwaway_frame;
                        while (frame_buffer_video_.try_pop(throwaway_frame)) {
                            graph_video_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
                            if (config_.single_pipe) {
                                graph_audio_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
                            }
                            graph_video_->set_value("input",
                                                    static_cast<double>(frame_buffer_video_.size() + 0.001) /
                                                        frame_buffer_video_.capacity());
                        }
                    }
                    video_thread_loop_running_ = false;

                    if (config_.realtime && config_.use_audio_pipe) {
                        // wait for the audio thread to stop and clear out old frames
                        while (audio_thread_loop_running_) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(2));
                        }
                    }
                } while (config_.auto_reconnect && is_running_);
            });
        }
        if (config_.use_audio_pipe) {
            thread_audio_ = std::thread([this] {
                do {
                    // open the pipe (this will block until the receiver opens the pipe)
                    HANDLE pipe_h;
                    if (!openPipe(L"audio", config_.audio_pipe_name, pipe_h)) {
                        if (!config_.auto_reconnect) {
                            return;
                        } else {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            continue;
                        }
                    }

                    audio_thread_loop_running_ = true;
                    try {
                        while (is_running_ && ready_for_frames_) {
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
                                // Then terminate the pipe consumer because there is no recovery from a failed/partial write
                                // when the write to the pipe is synchronous and should thus block until completion
                                // (unless we should try reconnecting!)
                                if (!config_.auto_reconnect) {
                                    is_running_ = false;
                                }
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

                    // This will prevent frames for backing up in the queue if "realtime" was requested.
                    // It will also force the other thread to reconnect the pipe, which ensures that both pipes
                    // are synchronised.
                    if (config_.realtime) {
                        ready_for_frames_ = false;
                        head_av_sync      = 0;

                        // empty the queue
                        // We don't use clear() here because it is thread unsafe.
                        core::const_frame throwaway_frame;
                        while (frame_buffer_audio_.try_pop(throwaway_frame)) {
                            graph_audio_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
                            if (config_.single_pipe) {
                                graph_video_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
                            }
                            // update the diagnostic graph
                            graph_audio_->set_value("input",
                                                    static_cast<double>(frame_buffer_audio_.size() + 0.001) /
                                                        frame_buffer_audio_.capacity());
                        }
                    }
                    audio_thread_loop_running_ = false;

                    if (config_.realtime && config_.use_video_pipe) {
                        // wait for the video thread to stop and clear out old frames
                        // This stops a race condition between each thread setting ready_for_frames_=false
                        // and pipe reconnection.
                        while (video_thread_loop_running_) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(2));
                        }
                    }
                } while (config_.auto_reconnect && is_running_);
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
        int   max_retries = 20;
        int   retries     = 0;
        DWORD lastError   = 0;
        DWORD dwMode      = PIPE_WAIT;
        
        // Create pipe
        CASPAR_LOG(info) << print(avType) << ((config_.existing_pipe) ? L" Opening" : L" Creating")
                         << L" named pipe...";
        // If the pipe creation fails because of ERROR_PIPE_BUSY, loop for a couple of seconds to
        // see if it becomes free.
        do {
            if (config_.existing_pipe) {
                pipe_h = CreateFile(pipeName.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
            } else {
                pipe_h = CreateNamedPipeW(pipeName.c_str(),
                                          PIPE_ACCESS_DUPLEX,
                                          dwMode,
                                          1,
                                          0,
                                          0,
                                          0,
                                          NULL);
            }

            if (pipe_h == INVALID_HANDLE_VALUE) {
                lastError = GetLastError();
                if (lastError == ERROR_PIPE_BUSY) {
                    CASPAR_LOG(debug) << print(avType) << L" Failed to "
                                      << ((config_.existing_pipe) ? L"open" : L"create")
                                      << L" named pipe (PIPE_BUSY). Will retry in 100ms.";
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    retries++;
                } else if (lastError == ERROR_FILE_NOT_FOUND && config_.existing_pipe) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                closePipe(avType, pipe_h);
            } else {
                lastError = 0;
            }
            
        } while (((lastError == ERROR_PIPE_BUSY && retries <= max_retries) ||
                  (lastError == ERROR_FILE_NOT_FOUND && config_.existing_pipe)) &&
                 is_running_);

        // Handle error on pipe creation
        if (pipe_h == INVALID_HANDLE_VALUE) {
            std::wstring msg = L"Error code: " + std::to_wstring(lastError) + L".";
            // print message for the most common errors
            if (lastError == ERROR_INVALID_NAME) {
                msg += L" Error message: The pipe name is invalid.";
            } else if (lastError == ERROR_BAD_PATHNAME) {
                msg += L" Error message: The pipe path is invalid";
            }
            CASPAR_LOG(error) << print(avType) << L" Failed to " << ((config_.existing_pipe) ? L"open" : L"create")
                              << L" named pipe \"" << pipeName << L"\". " << msg;
            // close handle
            closePipe(avType, pipe_h);
            // keep running if auto_reconnect = true and the pipe was busy
            if (!config_.auto_reconnect || lastError != ERROR_PIPE_BUSY) {
                is_running_ = false;
            }
            return false;
        }

        if (config_.existing_pipe) {
            // Set pipe mode
            CASPAR_LOG(info) << print(avType) << L" Pipe connected. Setting pipe mode...";
            if (!SetNamedPipeHandleState(pipe_h, &dwMode, NULL, NULL)) {
                lastError = GetLastError();
                CASPAR_LOG(error) << print(avType) << L" Failed to set pipe mode. Error code: " << std::to_wstring(lastError);
                // close handle
                closePipe(avType, pipe_h);
                if (!config_.auto_reconnect) {
                    is_running_ = false;
                }
                return false;
            }
            CASPAR_LOG(info) << print(avType) << L" Pipe mode set.";
        } else {
            // Connect pipe
            CASPAR_LOG(info) << print(avType) << L" Pipe created. Connecting to pipe...";
            if (!ConnectNamedPipe(pipe_h, NULL)) {
                lastError = GetLastError();
                CASPAR_LOG(error) << print(avType) << L" Failed to connect to named pipe \"" << pipeName
                                  << L"\". Error code: " << std::to_wstring(lastError);
                // close handle
                closePipe(avType, pipe_h);
                if (!config_.auto_reconnect) {
                    is_running_ = false;
                }
                return false;
            }
            CASPAR_LOG(info) << print(avType) << L" Pipe connected";
        }

        ready_for_frames_ = true;
        return true;
    }

    bool writeVideo(std::wstring avType, const core::const_frame& in_frame, HANDLE& pipe_h)
    {
        DWORD numBytesWritten = 0;
        DWORD numBytesToWrite = format_desc_.width * format_desc_.height * 4; // assumes RGBA

        if (head_av_sync < 0) {
            // Log the dropped frame
            graph_video_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
            head_av_sync += 1;
            return true;
        }

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

            // Update the av_sync for the head of the queue
            head_av_sync += 1;

            // Log the dropped frame
            graph_video_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
            return false;
        }
        return true;
    }

    bool writeAudio(std::wstring avType, const core::const_frame& in_frame, HANDLE& pipe_h)
    {
        DWORD numBytesWritten = 0;
        DWORD numBytesToWrite = 0;

        // if we need to drop an audio frame to resynchronise, then we will
        if (head_av_sync > 0) {
            // Log the dropped frame
            graph_audio_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
            head_av_sync -= 1;
            return true;
        }

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

            // update the av_sync for the head of the queue
            head_av_sync -= 1;

            // Log the dropped frame
            graph_audio_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
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
        handle = NULL;
        return true;
    }

    std::future<bool> send(const core::const_frame& frame)
    {
        bool vPushResult = true;
        bool aPushResult = true;
        // silently drop frames until the pipe is connected
        if (ready_for_frames_) {
            if (config_.use_video_pipe) {
                if (tail_av_sync >= 0) {
                    vPushResult = frame_buffer_video_.try_push(frame);
                    // update the diagnostic graph
                    graph_video_->set_value("input",
                                            static_cast<double>(frame_buffer_video_.size() + 0.001) /
                                                frame_buffer_video_.capacity());
                } else {
                    // compensate for previous audio frame drop
                    vPushResult = false;
                }

                if (!vPushResult) {
                    graph_video_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
                    // Only change sync counter if the other pipe is in use too
                    // Note that this will force an audio frame drop for the same frame
                    // as the missed video frame
                    if (config_.use_audio_pipe) {
                        tail_av_sync += 1;
                    }
                }
            }
            if (config_.use_audio_pipe) {
                if (tail_av_sync <= 0) {
                    aPushResult = frame_buffer_audio_.try_push(frame);
                    // update the diagnostic graph
                    graph_audio_->set_value("input",
                                            static_cast<double>(frame_buffer_audio_.size() + 0.001) /
                                                frame_buffer_audio_.capacity());
                } else {
                    // compensate for previous video frame drop
                    aPushResult = false;
                }

                if (!aPushResult) {
                    graph_audio_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
                    // Only change sync counter if the other pipe is in use too
                    // Note that this will for a video frame drop for the subsequent frame
                    // and so audio/video will be out of sync for at least 1 frame (with 1 frame also lost)
                    if (config_.use_video_pipe) {
                        tail_av_sync -= 1;
                    }
                }
            }
            // If only one dropped a frame, warn in the log that the A/V sync will be out
            if (config_.use_audio_pipe && config_.use_video_pipe && (vPushResult^aPushResult)) {
                if (!vPushResult) {
                    if (tail_av_sync != 0) {
                        CASPAR_LOG(warning)
                            << print()
                            << L" A video frame was dropped but an audio frame was not. The audio-video "
                               L"sync is out by "
                            << tail_av_sync.load() << L" frames";
                    }
                }
                else if (!aPushResult) {
                    if (tail_av_sync != 0) {
                        CASPAR_LOG(warning)
                            << print()
                            << L" An audio frame was dropped but a video frame was not. The audio-video "
                               L"sync is out by "
                            << tail_av_sync.load() << L" frames";
                    }
                }
            }
        } else {
            if (config_.use_audio_pipe) {
                // log dropped frame
                graph_audio_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
                // Update other lines as they won't be updated if the thread is not running
                graph_audio_->set_value(
                    "input", static_cast<double>(frame_buffer_audio_.size() + 0.001) / frame_buffer_audio_.capacity());
                graph_audio_->set_value("frame-time", 0);
            }
            if (config_.use_video_pipe) {
                // log dropped frame
                graph_video_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
                // Update other lines as they won't be updated if the thread is not running
                graph_video_->set_value(
                    "input", static_cast<double>(frame_buffer_video_.size() + 0.001) / frame_buffer_video_.capacity());
                graph_video_->set_value("frame-time", 0);
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
    config.from_config = false;

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
    config.auto_reconnect = contains_param(L"AUTO_RECONNECT", params);
    config.existing_pipe  = contains_param(L"EXISTING_PIPE", params);

    if (contains_param(L"REALTIME", params)) {
        if (!config.auto_reconnect) {
            CASPAR_LOG(warning) << config.name << L": Ignoring REALTIME parameter (only valid with AUTO_RECONNECT)";
            config.realtime = false;
        } else {
            try {
                if (boost::iequals(get_param(L"REALTIME", params), L"FALSE")) {
                    config.realtime = false;
                } else {
                    config.realtime = true;
                }
            } catch (...) {
                config.realtime = true;
            }
        }
    } else if (config.auto_reconnect) {
        config.realtime = true; // default to true if auto_reconnect is true
    }

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
    config.from_config     = true;
    config.name            = ptree.get(L"name", config.name);
    config.pipe_index      = ptree.get(L"index", config.pipe_index + 1) - 1;
    config.video_pipe_name = ptree.get(L"video-pipe-name", config.video_pipe_name);
    config.audio_pipe_name = ptree.get(L"audio-pipe-name", config.audio_pipe_name);
    config.use_video_pipe  = boost::iequals(config.video_pipe_name, L"") ? false : true;
    config.use_audio_pipe  = boost::iequals(config.audio_pipe_name, L"") ? false : true;
    config.single_pipe     = ptree.get(L"single-pipe", config.single_pipe);
    config.buffer_size     = ptree.get(L"buffer-size", config.buffer_size);
    config.auto_reconnect  = ptree.get(L"auto-reconnect", config.auto_reconnect);
    config.realtime        = ptree.get(L"realtime", config.auto_reconnect); // default to true if auto_reconnect is true
    config.existing_pipe   = ptree.get(L"existing-pipe", config.existing_pipe);

    // If separate pipes were specified for A/V but a single pipe was also requested, ignore the AUDIO_PIPE
    // parameter
    if (config.single_pipe && config.use_audio_pipe && config.use_video_pipe) {
        CASPAR_LOG(warning) << config.name
                            << L": Ignoring audio-pipe-name parameter (conflicts with single-pipe when video-pipe-name "
                               L"also specified).";
        config.use_audio_pipe = false;
    }

    if (config.realtime && !config.auto_reconnect) {
        config.realtime = false;
        CASPAR_LOG(warning) << config.name << L": Ignoring realtime parameter (only valid with auto-reconnect=true)";
    }

    return spl::make_shared<pipe_consumer_proxy>(config);
}

}} // namespace caspar::pipe
