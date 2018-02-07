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

#include "screen_consumer.h"

#include <GL/glew.h>
#include <SFML/Window.hpp>

#include <common/array.h>
#include <common/diagnostics/graph.h>
#include <common/future.h>
#include <common/gl/gl_check.h>
#include <common/log.h>
#include <common/memory.h>
#include <common/memshfl.h>
#include <common/param.h>
#include <common/prec_timer.h>
#include <common/scope_exit.h>
#include <common/timer.h>
#include <common/utf.h>

#include <core/consumer/frame_consumer.h>
#include <core/frame/frame.h>
#include <core/interaction/interaction_sink.h>
#include <core/video_format.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>

#include <tbb/concurrent_queue.h>

#include <vector>

#if defined(_MSC_VER)
#include <windows.h>

#pragma warning(push)
#pragma warning(disable : 4244)
#endif

namespace caspar { namespace screen {

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

    std::wstring    name             = L"Screen consumer";
    int             screen_index     = 0;
    screen::stretch stretch          = screen::stretch::fill;
    bool            windowed         = true;
    bool            auto_deinterlace = true;
    bool            key_only         = false;
    aspect_ratio    aspect           = aspect_ratio::aspect_invalid;
    bool            vsync            = false;
    bool            interactive      = true;
    bool            borderless       = false;
};

struct frame
{
    GLuint pbo = 0;
    GLuint tex = 0;
    char*  ptr = nullptr;
};

struct screen_consumer : boost::noncopyable
{
    const configuration     config_;
    core::video_format_desc format_desc_;
    int                     channel_index_;

    std::vector<frame> frames_;

    float width_;
    float height_;
    int   screen_x_;
    int   screen_y_;
    int   screen_width_  = format_desc_.width;
    int   screen_height_ = format_desc_.height;
    int   square_width_  = format_desc_.square_width;
    int   square_height_ = format_desc_.square_height;

    sf::Window        window_;
    std::atomic<bool> polling_event_;

    spl::shared_ptr<diagnostics::graph> graph_;
    caspar::timer                       perf_timer_;
    caspar::timer                       tick_timer_;

    caspar::prec_timer wait_timer_;

    tbb::concurrent_bounded_queue<core::const_frame> frame_buffer_;
    core::interaction_sink*                          sink_;

    std::thread       thread_;
    std::atomic<bool> is_running_;

  public:
    screen_consumer(const configuration&           config,
                    const core::video_format_desc& format_desc,
                    int                            channel_index,
                    core::interaction_sink*        sink)
        : config_(config)
        , format_desc_(format_desc)
        , channel_index_(channel_index)
        , sink_(sink)
    {
        if (format_desc_.format == core::video_format::ntsc &&
            config_.aspect == configuration::aspect_ratio::aspect_4_3) {
            // Use default values which are 4:3.
        } else {
            if (config_.aspect == configuration::aspect_ratio::aspect_16_9) {
                square_width_ = (format_desc.height * 16) / 9;
            } else if (config_.aspect == configuration::aspect_ratio::aspect_4_3) {
                square_width_ = (format_desc.height * 4) / 3;
            }
        }

        frame_buffer_.set_capacity(1);

        graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        graph_->set_text(print());
        diagnostics::register_graph(graph_);

#if defined(_MSC_VER)
        DISPLAY_DEVICE              d_device = {sizeof(d_device), 0};
        std::vector<DISPLAY_DEVICE> displayDevices;
        for (int n = 0; EnumDisplayDevices(NULL, n, &d_device, NULL); ++n) {
            displayDevices.push_back(d_device);
        }

        if (config_.screen_index >= displayDevices.size()) {
            CASPAR_LOG(warning) << print() << L" Invalid screen-index: " << config_.screen_index;
        }

        DEVMODE devmode = {};
        if (!EnumDisplaySettings(displayDevices[config_.screen_index].DeviceName, ENUM_CURRENT_SETTINGS, &devmode)) {
            CASPAR_LOG(warning) << print() << L" Could not find display settings for screen-index: "
                                << config_.screen_index;
        }

        screen_x_      = devmode.dmPosition.x;
        screen_y_      = devmode.dmPosition.y;
        screen_width_  = config_.windowed ? square_width_ : devmode.dmPelsWidth;
        screen_height_ = config_.windowed ? square_height_ : devmode.dmPelsHeight;
#else
        if (config_.screen_index > 1) {
            CASPAR_LOG(warning) << print() << L" Screen-index is not supported on linux";
        }

        screen_x_      = 0;
        screen_y_      = 0;
        screen_width_  = square_width_;
        screen_height_ = square_height_;
#endif

        polling_event_ = false;
        is_running_    = true;
        thread_        = std::thread([this] { run(); });
    }

    ~screen_consumer()
    {
        is_running_ = false;
        frame_buffer_.try_push(core::const_frame{});
        thread_.join();
    }

    void init()
    {
        auto window_style =
            config_.borderless ? sf::Style::None : (config_.windowed ? sf::Style::Resize : sf::Style::Fullscreen);
        window_.create(sf::VideoMode::getDesktopMode(), u8(print()), window_style);
        window_.setPosition(sf::Vector2i(screen_x_, screen_y_));
        window_.setSize(sf::Vector2u(screen_width_, screen_height_));
        window_.setMouseCursorVisible(config_.interactive);
        window_.setActive();

        if (glewInit() != GLEW_OK) {
            CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to initialize GLEW."));
        }

        if (!GLEW_VERSION_4_5) {
            CASPAR_THROW_EXCEPTION(not_supported() << msg_info("Missing OpenGL 4.5 support."));
        }

        for (int n = 0; n < 3; ++n) {
            screen::frame frame;
            auto          flags = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT;
            GL(glCreateBuffers(1, &frame.pbo));
            GL(glNamedBufferStorage(frame.pbo, format_desc_.size, nullptr, flags));
            frame.ptr = reinterpret_cast<char*>(GL2(glMapNamedBufferRange(frame.pbo, 0, format_desc_.size, flags)));

            GL(glCreateTextures(GL_TEXTURE_2D, 1, &frame.tex));
            GL(glTextureParameteri(frame.tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            GL(glTextureParameteri(frame.tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            GL(glTextureParameteri(frame.tex, GL_TEXTURE_WRAP_S, GL_CLAMP));
            GL(glTextureParameteri(frame.tex, GL_TEXTURE_WRAP_T, GL_CLAMP));
            GL(glTextureStorage2D(frame.tex, 1, GL_RGBA8, format_desc_.width, format_desc_.height));
            GL(glClearTexImage(frame.tex, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr));

            frames_.push_back(frame);
        }

        GL(glEnable(GL_TEXTURE_2D));
        GL(glDisable(GL_DEPTH_TEST));
        GL(glClearColor(0.0, 0.0, 0.0, 0.0));
        GL(glViewport(0, 0, format_desc_.width, format_desc_.height));
        GL(glLoadIdentity());

        calculate_aspect();

        window_.setVerticalSyncEnabled(config_.vsync);

        if (config_.vsync) {
            CASPAR_LOG(info) << print() << " Enabled vsync.";
        }
    }

    void uninit()
    {
        for (auto frame : frames_) {
            GL(glUnmapNamedBuffer(frame.pbo));
            glDeleteBuffers(1, &frame.pbo);
            glDeleteTextures(1, &frame.tex);
        }
    }

    void run()
    {
        try {
            init();

            while (is_running_) {
                try {
                    auto poll_event = [this](sf::Event& e) {
                        polling_event_ = true;
                        CASPAR_SCOPE_EXIT { polling_event_ = false; };
                        return window_.pollEvent(e);
                    };

                    sf::Event e;
                    while (poll_event(e)) {
                        if (e.type == sf::Event::Resized) {
                            calculate_aspect();
                        } else if (e.type == sf::Event::Closed) {
                            is_running_ = false;
                        } else if (config_.interactive && sink_) {
                            switch (e.type) {
                                case sf::Event::MouseMoved: {
                                    auto& mouse_move = e.mouseMove;
                                    sink_->on_interaction(spl::make_shared<core::mouse_move_event>(
                                        1,
                                        static_cast<double>(mouse_move.x) / screen_width_,
                                        static_cast<double>(mouse_move.y) / screen_height_));
                                    break;
                                }
                                case sf::Event::MouseButtonPressed:
                                case sf::Event::MouseButtonReleased: {
                                    auto& mouse_button = e.mouseButton;
                                    sink_->on_interaction(spl::make_shared<core::mouse_button_event>(
                                        1,
                                        static_cast<double>(mouse_button.x) / screen_width_,
                                        static_cast<double>(mouse_button.y) / screen_height_,
                                        static_cast<int>(mouse_button.button),
                                        e.type == sf::Event::MouseButtonPressed));
                                    break;
                                }
                                case sf::Event::MouseWheelMoved: {
                                    auto& wheel_moved = e.mouseWheel;
                                    sink_->on_interaction(spl::make_shared<core::mouse_wheel_event>(
                                        1,
                                        static_cast<double>(wheel_moved.x) / screen_width_,
                                        static_cast<double>(wheel_moved.y) / screen_height_,
                                        wheel_moved.delta));
                                    break;
                                }
                            }
                        }
                    }

                    core::const_frame in_frame;
                    frame_buffer_.pop(in_frame);

                    // Display
                    {
                        auto& frame = frames_[2];

                        GL(glBindTexture(GL_TEXTURE_2D, frame.tex));
                        GL(glClear(GL_COLOR_BUFFER_BIT));

                        glBegin(GL_QUADS);
                        glTexCoord2f(0.0f, 1.0f);
                        glVertex2f(-width_, -height_);
                        glTexCoord2f(1.0f, 1.0f);
                        glVertex2f(width_, -height_);
                        glTexCoord2f(1.0f, 0.0f);
                        glVertex2f(width_, height_);
                        glTexCoord2f(0.0f, 0.0f);
                        glVertex2f(-width_, height_);
                        glEnd();

                        GL(glBindTexture(GL_TEXTURE_2D, 0));
                    }

                    GL(glFlush());

                    // Upload
                    {
                        auto& frame = frames_[0];

                        // TODO (fix) Wait for sync before re-using pbo?

                        if (config_.key_only) {
                            for (int n = 0; n < format_desc_.height; ++n) {
                                aligned_memshfl(frame.ptr + n * format_desc_.width * 4,
                                                in_frame.image_data(0).begin() + n * format_desc_.width * 4,
                                                format_desc_.width * 4,
                                                0x0F0F0F0F,
                                                0x0B0B0B0B,
                                                0x07070707,
                                                0x03030303);
                            }
                        } else {
                            std::memcpy(frame.ptr, in_frame.image_data(0).begin(), format_desc_.size);
                        }

                        GL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, frame.pbo));
                        GL(glTextureSubImage2D(frame.tex,
                                               0,
                                               0,
                                               0,
                                               format_desc_.width,
                                               format_desc_.height,
                                               GL_BGRA,
                                               GL_UNSIGNED_BYTE,
                                               nullptr));
                        GL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
                    }

                    window_.display();

                    std::rotate(frames_.begin(), frames_.begin() + 1, frames_.end());

                    graph_->set_value("tick-time", tick_timer_.elapsed() * format_desc_.fps * 0.5);
                    tick_timer_.restart();
                } catch (...) {
                    CASPAR_LOG_CURRENT_EXCEPTION();
                    is_running_ = false;
                }
            }

            uninit();
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
        }
    }

    std::future<bool> send(core::const_frame frame)
    {
        if (!frame_buffer_.try_push(frame) && !polling_event_) {
            graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
        }
        return make_ready_future(is_running_.load());
    }

    std::wstring channel_and_format() const
    {
        return L"[" + boost::lexical_cast<std::wstring>(channel_index_) + L"|" + format_desc_.name + L"]";
    }

    std::wstring print() const { return config_.name + L" " + channel_and_format(); }

    void calculate_aspect()
    {
        if (config_.windowed) {
            screen_height_ = window_.getSize().y;
            screen_width_  = window_.getSize().x;
        }

        GL(glViewport(0, 0, screen_width_, screen_height_));

        std::pair<float, float> target_ratio = none();
        if (config_.stretch == screen::stretch::fill) {
            target_ratio = Fill();
        } else if (config_.stretch == screen::stretch::uniform) {
            target_ratio = uniform();
        } else if (config_.stretch == screen::stretch::uniform_to_fill) {
            target_ratio = uniform_to_fill();
        }

        width_  = target_ratio.first;
        height_ = target_ratio.second;
    }

    std::pair<float, float> none()
    {
        float width  = static_cast<float>(square_width_) / static_cast<float>(screen_width_);
        float height = static_cast<float>(square_height_) / static_cast<float>(screen_height_);

        return std::make_pair(width, height);
    }

    std::pair<float, float> uniform()
    {
        float aspect = static_cast<float>(square_width_) / static_cast<float>(square_height_);
        float width  = std::min(1.0f, static_cast<float>(screen_height_) * aspect / static_cast<float>(screen_width_));
        float height = static_cast<float>(screen_width_ * width) / static_cast<float>(screen_height_ * aspect);

        return std::make_pair(width, height);
    }

    std::pair<float, float> Fill() { return std::make_pair(1.0f, 1.0f); }

    std::pair<float, float> uniform_to_fill()
    {
        float wr    = static_cast<float>(square_width_) / static_cast<float>(screen_width_);
        float hr    = static_cast<float>(square_height_) / static_cast<float>(screen_height_);
        float r_inv = 1.0f / std::min(wr, hr);

        float width  = wr * r_inv;
        float height = hr * r_inv;

        return std::make_pair(width, height);
    }
};

struct screen_consumer_proxy : public core::frame_consumer
{
    core::monitor::subject           monitor_subject_;
    const configuration              config_;
    std::unique_ptr<screen_consumer> consumer_;
    core::interaction_sink*          sink_;

  public:
    screen_consumer_proxy(const configuration& config, core::interaction_sink* sink)
        : config_(config)
        , sink_(sink)
    {
    }

    // frame_consumer

    void initialize(const core::video_format_desc& format_desc, int channel_index) override
    {
        consumer_.reset();
        consumer_.reset(new screen_consumer(config_, format_desc, channel_index, sink_));
    }

    std::future<bool> send(core::const_frame frame) override { return consumer_->send(frame); }

    std::wstring print() const override { return consumer_ ? consumer_->print() : L"[screen_consumer]"; }

    std::wstring name() const override { return L"screen"; }

    bool has_synchronization_clock() const override { return false; }

    int buffer_depth() const override { return 1; }

    int index() const override { return 600 + (config_.key_only ? 10 : 0) + config_.screen_index; }

    core::monitor::subject& monitor_output() { return monitor_subject_; }
};

spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>&                  params,
                                                      core::interaction_sink*                           sink,
                                                      std::vector<spl::shared_ptr<core::video_channel>> channels)
{
    if (params.size() < 1 || !boost::iequals(params.at(0), L"SCREEN")) {
        return core::frame_consumer::empty();
    }

    configuration config;

    if (params.size() > 1) {
        config.screen_index = boost::lexical_cast<int>(params.at(1));
    }

    config.windowed         = !contains_param(L"FULLSCREEN", params);
    config.key_only         = contains_param(L"KEY_ONLY", params);
    config.interactive      = !contains_param(L"NON_INTERACTIVE", params);
    config.auto_deinterlace = !contains_param(L"NO_AUTO_DEINTERLACE", params);
    config.borderless       = contains_param(L"BORDERLESS", params);

    if (contains_param(L"NAME", params)) {
        config.name = get_param(L"NAME", params);
    }

    return spl::make_shared<screen_consumer_proxy>(config, sink);
}

spl::shared_ptr<core::frame_consumer>
create_preconfigured_consumer(const boost::property_tree::wptree&               ptree,
                              core::interaction_sink*                           sink,
                              std::vector<spl::shared_ptr<core::video_channel>> channels)
{
    configuration config;
    config.name             = ptree.get(L"name", config.name);
    config.screen_index     = ptree.get(L"device", config.screen_index + 1) - 1;
    config.windowed         = ptree.get(L"windowed", config.windowed);
    config.key_only         = ptree.get(L"key-only", config.key_only);
    config.auto_deinterlace = ptree.get(L"auto-deinterlace", config.auto_deinterlace);
    config.vsync            = ptree.get(L"vsync", config.vsync);
    config.interactive      = ptree.get(L"interactive", config.interactive);
    config.borderless       = ptree.get(L"borderless", config.borderless);

    auto stretch_str = ptree.get(L"stretch", L"default");
    if (stretch_str == L"uniform") {
        config.stretch = screen::stretch::uniform;
    } else if (stretch_str == L"uniform_to_fill") {
        config.stretch = screen::stretch::uniform_to_fill;
    }

    auto aspect_str = ptree.get(L"aspect-ratio", L"default");
    if (aspect_str == L"16:9") {
        config.aspect = configuration::aspect_ratio::aspect_16_9;
    } else if (aspect_str == L"4:3") {
        config.aspect = configuration::aspect_ratio::aspect_4_3;
    }

    return spl::make_shared<screen_consumer_proxy>(config, sink);
}

}} // namespace caspar::screen
