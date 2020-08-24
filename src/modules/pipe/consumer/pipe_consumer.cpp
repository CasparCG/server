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
#include <common/gl/gl_check.h>
#include <common/log.h>
#include <common/memory.h>
#include <common/memshfl.h>
#include <common/param.h>
#include <common/timer.h>
#include <common/utf.h>

#include <core/consumer/frame_consumer.h>
#include <core/frame/frame.h>
#include <core/frame/geometry.h>
#include <core/frame/pixel_format.h>
#include <core/video_format.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
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

enum class stretch
{
    none,
    uniform,
    fill,
    uniform_to_fill
};

struct configuration
{
    enum class aspect_ratio
    {
        aspect_4_3 = 0,
        aspect_16_9,
        aspect_invalid,
    };

    enum class colour_spaces
    {
        RGB               = 0,
        datavideo_full    = 1,
        datavideo_limited = 2
    };

    std::wstring    name          = L"Pipe consumer";
    int             screen_index  = 0;
    int             screen_x      = 0;
    int             screen_y      = 0;
    int             screen_width  = 0;
    int             screen_height = 0;
    pipe::stretch stretch       = pipe::stretch::fill;
    bool            windowed      = true;
    bool            key_only      = false;
    bool            sbs_key       = false;
    aspect_ratio    aspect        = aspect_ratio::aspect_invalid;
    bool            vsync         = false;
    bool            interactive   = true;
    bool            borderless    = false;
    bool            always_on_top = false;
    colour_spaces   colour_space  = colour_spaces::RGB;
};

struct pipe_consumer
{
    const configuration     config_;
    core::video_format_desc format_desc_;
    int                     channel_index_;

    //std::vector<frame> frames_;

    int screen_width_  = format_desc_.width;
    int screen_height_ = format_desc_.height;
    int square_width_  = format_desc_.square_width;
    int square_height_ = format_desc_.square_height;
    int screen_x_      = 0;
    int screen_y_      = 0;

    std::vector<core::frame_geometry::coord> draw_coords_;


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

        frame_buffer_.set_capacity(180); //TODO: Update to default of 3s and make configurable

        graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        graph_->set_text(print());
        diagnostics::register_graph(graph_);
        // CASPAR_LOG(warning) << print() << L" Screen-index is not supported on linux";

        

        thread_ = std::thread([this] {
            // Create pipe (if requested)
            CASPAR_LOG(info) << print() << L" Creating pipe...";
            HANDLE pipe_h = CreateNamedPipeA("\\\\.\\pipe\\caspartest",
                                             PIPE_ACCESS_DUPLEX,
                                             PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                             1,
                                             0,
                                             0,
                                             0,
                                             NULL);
            if (pipe_h == INVALID_HANDLE_VALUE) {
                CASPAR_LOG(error) << print() << L" Failed to create named pipe"; //TODO: include pipe name and GetLastError
                if (!CloseHandle(pipe_h)) {
                    CASPAR_LOG(error) << print() << L" Failed to close named pipe";
                }
                is_running_ = false;
                return;
            }

            // Connect pipe
            CASPAR_LOG(info) << print() << L" Pipe created. Connecting to pipe...";
            if (!ConnectNamedPipe(pipe_h, NULL)) {
                CASPAR_LOG(error) << print() << L" Failed to connect to named pipe"; // TODO: include pipe name and GetLastError
                if (!CloseHandle(pipe_h)) {
                    CASPAR_LOG(error) << print() << L" Failed to close named pipe";
                }
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

                    // TODO: send frame data
                    //       The frame format is separated into lines, not a continuous buffer.
                    //       To save on memcpy, we should send each line separately rather than the whole frame
                    //       Need to check performance of that though!
                    /*for (int n = 0; n < planes.size(); ++n) {
                        for (int y = 0; y < av_frame->height; ++y) {
                            std::memcpy(av_frame->data[n] + y * av_frame->linesize[n],
                                        frame.image_data(n).data() + y * planes[n].linesize,
                                        planes[n].linesize);

                        }
                    }*/
                    bool writeSuccess =
                        WriteFile(pipe_h, in_frame.image_data(0).data(), numBytesToWrite, &numBytesWritten, NULL);
                    if (!writeSuccess ||
                        numBytesToWrite != numBytesWritten) {
                        CASPAR_LOG(warning) << print() << L" Frame dropped (failed to write frame to pipe)"; // TODO: Add frame number counter and GetLastError
                        // TODO: DO I need to set something up with the graph to log this dropped frame?
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
            if (!CloseHandle(pipe_h)) {
                CASPAR_LOG(error) << print() << L" Failed to close named pipe";
            }
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

    int index() const override { return 600 + (config_.key_only ? 10 : 0) + config_.screen_index; } // TODO: WHat is this?
};

spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>&                         params,
                                                      const std::vector<spl::shared_ptr<core::video_channel>>& channels)
{
    if (params.empty() || !boost::iequals(params.at(0), L"PIPE")) {
        return core::frame_consumer::empty();
    }

    configuration config;

    if (params.size() > 1) {
        try {
            config.screen_index = std::stoi(params.at(1));
        } catch (...) {
        }
    }

    config.windowed    = !contains_param(L"FULLSCREEN", params); //TODO: Update config
    config.key_only    = contains_param(L"KEY_ONLY", params);
    config.sbs_key     = contains_param(L"SBS_KEY", params);
    config.interactive = !contains_param(L"NON_INTERACTIVE", params);
    config.borderless  = contains_param(L"BORDERLESS", params);

    if (contains_param(L"NAME", params)) {
        config.name = get_param(L"NAME", params);
    }

    if (config.sbs_key && config.key_only) {
        CASPAR_LOG(warning) << L" Key-only not supported with configuration of side-by-side fill and key. Ignored.";
        config.key_only = false;
    }

    return spl::make_shared<pipe_consumer_proxy>(config);
}

spl::shared_ptr<core::frame_consumer>
create_preconfigured_consumer(const boost::property_tree::wptree&                      ptree,
                              const std::vector<spl::shared_ptr<core::video_channel>>& channels)
{
    configuration config;
    config.name          = ptree.get(L"name", config.name);
    config.screen_index  = ptree.get(L"device", config.screen_index + 1) - 1;
    config.screen_x      = ptree.get(L"x", config.screen_x);
    config.screen_y      = ptree.get(L"y", config.screen_y);
    config.screen_width  = ptree.get(L"width", config.screen_width);
    config.screen_height = ptree.get(L"height", config.screen_height);
    config.windowed      = ptree.get(L"windowed", config.windowed);
    config.key_only      = ptree.get(L"key-only", config.key_only);
    config.sbs_key       = ptree.get(L"sbs-key", config.sbs_key);
    config.vsync         = ptree.get(L"vsync", config.vsync);
    config.interactive   = ptree.get(L"interactive", config.interactive);
    config.borderless    = ptree.get(L"borderless", config.borderless);
    config.always_on_top = ptree.get(L"always-on-top", config.always_on_top);

    auto colour_space_value = ptree.get(L"colour-space", L"RGB");
    config.colour_space     = configuration::colour_spaces::RGB;
    if (colour_space_value == L"datavideo-full")
        config.colour_space = configuration::colour_spaces::datavideo_full;
    else if (colour_space_value == L"datavideo-limited")
        config.colour_space = configuration::colour_spaces::datavideo_limited;

    if (config.sbs_key && config.key_only) {
        CASPAR_LOG(warning) << L" Key-only not supported with configuration of side-by-side fill and key. Ignored.";
        config.key_only = false;
    }

    if ((config.colour_space == configuration::colour_spaces::datavideo_full ||
         config.colour_space == configuration::colour_spaces::datavideo_limited) &&
        config.sbs_key) {
        CASPAR_LOG(warning) << L" Side-by-side fill and key not supported for DataVideo TC100/TC200. Ignored.";
        config.sbs_key = false;
    }

    if ((config.colour_space == configuration::colour_spaces::datavideo_full ||
         config.colour_space == configuration::colour_spaces::datavideo_limited) &&
        config.key_only) {
        CASPAR_LOG(warning) << L" Key only not supported for DataVideo TC100/TC200. Ignored.";
        config.key_only = false;
    }

    auto stretch_str = ptree.get(L"stretch", L"fill");
    if (stretch_str == L"none") {
        config.stretch = pipe::stretch::none;
    } else if (stretch_str == L"uniform") {
        config.stretch = pipe::stretch::uniform;
    } else if (stretch_str == L"uniform_to_fill") {
        config.stretch = pipe::stretch::uniform_to_fill;
    }

    auto aspect_str = ptree.get(L"aspect-ratio", L"default");
    if (aspect_str == L"16:9") {
        config.aspect = configuration::aspect_ratio::aspect_16_9;
    } else if (aspect_str == L"4:3") {
        config.aspect = configuration::aspect_ratio::aspect_4_3;
    }

    return spl::make_shared<pipe_consumer_proxy>(config);
}

}} // namespace caspar::pipe
