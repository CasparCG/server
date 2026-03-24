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
#include <common/param.h>
#include <common/timer.h>
#include <common/utf.h>

#include <core/consumer/channel_info.h>
#include <core/consumer/frame_consumer.h>
#include <core/frame/frame.h>
#include <core/frame/geometry.h>
#include <core/video_format.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>

#include <tbb/concurrent_queue.h>

#include <thread>
#include <utility>
#include <vector>

#if defined(_MSC_VER)
#include <windows.h>

#pragma warning(push)
#pragma warning(disable : 4244)
#else
#include "../util/x11_util.h"
#endif

#include "consumer_screen_fragment.h"
#include "consumer_screen_vertex.h"
#include <accelerator/ogl/util/shader.h>

namespace caspar { namespace screen {

std::unique_ptr<accelerator::ogl::shader> get_shader()
{
    return std::make_unique<accelerator::ogl::shader>(std::string(vertex_shader), std::string(fragment_shader));
}

enum class stretch
{
    none,
    uniform,
    fill,
    uniform_to_fill
};

struct configuration
{
    enum class colour_spaces
    {
        RGB               = 0,
        datavideo_full    = 1,
        datavideo_limited = 2
    };

    std::wstring    name             = L"Screen consumer";
    int             screen_index     = 0;
    int             screen_x         = 0;
    int             screen_y         = 0;
    int             screen_width     = 0;
    int             screen_height    = 0;
    screen::stretch stretch          = screen::stretch::fill;
    bool            windowed         = true;
    bool            high_bitdepth    = false;
    bool            key_only         = false;
    bool            sbs_key          = false;
    double          aspect_ratio     = 0.0;   // 0.0 = auto (derived from format_desc square dimensions)
    bool            enable_mipmaps   = false; // Useful when content is displayed significantly smaller than native
    double          brightness_boost = 1.0;
    double          saturation_boost = 1.0;
    bool            vsync            = false;
    bool            interactive      = true;
    bool            borderless       = false;
    bool            always_on_top    = false;
    bool            gpu_texture      = false;
    colour_spaces   colour_space     = colour_spaces::RGB;
};

struct frame
{
    GLuint                         pbo = 0;
    GLuint                         tex = 0;
    char*                          ptr = nullptr;
    std::shared_ptr<core::texture> texture;
    GLsync                         fence = nullptr;
};

// Parses aspect ratio strings. Returns 0.0 for "auto" (use format square dimensions).
// Supports: "16:9", "4:3", "3840:1080", "3840/1080", decimal "3.555", "default" or empty.
double parse_aspect_ratio(const std::wstring& aspect_str)
{
    if (aspect_str == L"default" || aspect_str.empty()) {
        return 0.0;
    }

    // Support both ':' and '/' as width/height separators
    auto sep_pos = aspect_str.find(L':');
    if (sep_pos == std::wstring::npos) {
        sep_pos = aspect_str.find(L'/');
    }

    if (sep_pos != std::wstring::npos) {
        try {
            double width  = std::stod(aspect_str.substr(0, sep_pos));
            double height = std::stod(aspect_str.substr(sep_pos + 1));
            if (height == 0.0) {
                CASPAR_LOG(warning) << L"Invalid aspect ratio denominator in '" << aspect_str << L"', using auto.";
                return 0.0;
            }
            return width / height;
        } catch (...) {
            CASPAR_LOG(warning) << L"Failed to parse aspect ratio '" << aspect_str << L"', using auto.";
            return 0.0;
        }
    }

    // Decimal format e.g. "3.555"
    try {
        double result = std::stod(aspect_str);
        if (result < 0.1 || result > 10.0) {
            CASPAR_LOG(warning) << L"Aspect ratio " << aspect_str
                                << L" out of reasonable range (0.1-10.0), using auto.";
            return 0.0;
        }
        return result;
    } catch (...) {
        CASPAR_LOG(warning) << L"Failed to parse aspect ratio '" << aspect_str << L"', using auto.";
        return 0.0;
    }
}

struct screen_consumer;

struct display_strategy
{
    virtual ~display_strategy() {}
    virtual frame init_frame(const configuration& config, const core::video_format_desc& format_desc) = 0;
    virtual void  cleanup_frame(frame& frame)                                                         = 0;
    virtual void  do_tick(screen_consumer* self)                                                      = 0;
};

struct gpu_strategy;
struct host_strategy;

struct screen_consumer
{
    const configuration     config_;
    core::video_format_desc format_desc_;
    int                     channel_index_;

    std::vector<frame> frames_;

    int screen_width_  = format_desc_.width;
    int screen_height_ = format_desc_.height;
    int square_width_  = format_desc_.square_width;
    int square_height_ = format_desc_.square_height;
    int screen_x_      = 0;
    int screen_y_      = 0;

    std::vector<core::frame_geometry::coord> draw_coords_;

    sf::Window window_;

    spl::shared_ptr<diagnostics::graph> graph_;
    caspar::timer                       tick_timer_;

    tbb::concurrent_bounded_queue<core::const_frame> frame_buffer_;

    std::unique_ptr<accelerator::ogl::shader> shader_;
    GLuint                                    vao_;
    GLuint                                    vbo_;

    std::atomic<bool> is_running_{true};
    std::thread       thread_;

    spl::shared_ptr<display_strategy> strategy_;

    screen_consumer(const screen_consumer&)            = delete;
    screen_consumer& operator=(const screen_consumer&) = delete;

  public:
    screen_consumer(const configuration& config, const core::video_format_desc& format_desc, int channel_index)
        : config_(config)
        , format_desc_(format_desc)
        , channel_index_(channel_index)
        , strategy_(config.gpu_texture ? spl::make_shared<display_strategy, gpu_strategy>()
                                       : spl::make_shared<display_strategy, host_strategy>())
    {
        if (config_.gpu_texture) {
            CASPAR_LOG(info) << print() << L" Using GPU texture strategy.";
        }

        // Apply explicit aspect ratio to square dimensions if provided; otherwise use format defaults.
        if (config_.aspect_ratio != 0.0) {
            square_width_  = static_cast<int>(format_desc.height * config_.aspect_ratio);
            square_height_ = format_desc.height;
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

        for (int n = 0; EnumDisplayDevices(nullptr, n, &d_device, NULL); ++n) {
            displayDevices.push_back(d_device);
            memset(&d_device, 0, sizeof(d_device));
            d_device.cb = sizeof(d_device);
        }

        int selected_screen_index = config_.screen_index;

        if (selected_screen_index >= static_cast<int>(displayDevices.size())) {
            CASPAR_LOG(warning) << print() << L" Invalid screen-index " << selected_screen_index << L" (only "
                                << displayDevices.size() << L" devices available), falling back to primary display.";
            for (size_t i = 0; i < displayDevices.size(); ++i) {
                if (displayDevices[i].StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) {
                    selected_screen_index = static_cast<int>(i);
                    break;
                }
            }
        }

        if (selected_screen_index < 0 || selected_screen_index >= static_cast<int>(displayDevices.size())) {
            selected_screen_index = 0;
        }

        DEVMODE selectedDevMode = {};
        BOOL    result          = EnumDisplaySettings(
            displayDevices[selected_screen_index].DeviceName, ENUM_CURRENT_SETTINGS, &selectedDevMode);

        if (!result) {
            result = EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &selectedDevMode);
        }

        if (result) {
            screen_x_      = selectedDevMode.dmPosition.x;
            screen_y_      = selectedDevMode.dmPosition.y;
            screen_width_  = selectedDevMode.dmPelsWidth;
            screen_height_ = selectedDevMode.dmPelsHeight;
            CASPAR_LOG(info) << print() << L" Display " << selected_screen_index << L" ("
                             << displayDevices[selected_screen_index].DeviceName << L"): "
                             << screen_width_ << L"x" << screen_height_
                             << L" at (" << screen_x_ << L"," << screen_y_ << L").";
        } else {
            CASPAR_LOG(error) << print() << L" Failed to get display settings, using 1920x1080 fallback.";
            screen_x_      = 0;
            screen_y_      = 0;
            screen_width_  = 1920;
            screen_height_ = 1080;
        }

#else
        if (config_.screen_index > 1) {
            CASPAR_LOG(warning) << print() << L" Screen-index is not supported on Linux.";
        }
#endif

        if (config_.windowed) {
            screen_x_ += config_.screen_x;
            screen_y_ += config_.screen_y;

            if (config_.screen_width > 0 && config_.screen_height > 0) {
                screen_width_  = config_.screen_width;
                screen_height_ = config_.screen_height;
            } else if (config_.screen_width > 0) {
                screen_width_  = config_.screen_width;
                screen_height_ = square_height_ * config_.screen_width / square_width_;
            } else if (config_.screen_height > 0) {
                screen_height_ = config_.screen_height;
                screen_width_  = square_width_ * config_.screen_height / square_height_;
            } else {
                screen_width_  = square_width_;
                screen_height_ = square_height_;
            }
        } else {
            if (config_.screen_width > 0 && config_.screen_height > 0) {
                screen_width_  = config_.screen_width;
                screen_height_ = config_.screen_height;
                if (config_.screen_x != 0 || config_.screen_y != 0) {
                    // Explicit position replaces device position (enables multi-display spanning)
                    screen_x_ = config_.screen_x;
                    screen_y_ = config_.screen_y;
                }
            } else if (config_.screen_width > 0) {
                screen_width_  = config_.screen_width;
                screen_height_ = square_height_ * config_.screen_width / square_width_;
            } else if (config_.screen_height > 0) {
                screen_height_ = config_.screen_height;
                screen_width_  = square_width_ * config_.screen_height / square_height_;
            }
        }

        CASPAR_LOG(info) << print() << L" Window: " << screen_width_ << L"x" << screen_height_
                         << L" at (" << screen_x_ << L"," << screen_y_ << L")"
                         << (config_.windowed ? L" [windowed]" : L" [fullscreen]");

        thread_ = std::thread([this] {
            try {
                sf::Uint32 window_style;
                if (config_.borderless) {
                    window_style = sf::Style::None;
                } else if (config_.windowed) {
                    window_style = sf::Style::Resize | sf::Style::Close;
                } else {
                    // Use borderless for multi-display spanning (custom position/size), true fullscreen otherwise
                    if ((config_.screen_width > 0 && config_.screen_width != screen_width_) ||
                        (config_.screen_height > 0 && config_.screen_height != screen_height_) ||
                        (config_.screen_x != 0 || config_.screen_y != 0)) {
                        window_style = sf::Style::None;
                    } else {
                        window_style = sf::Style::Fullscreen;
                    }
                }

                sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
                sf::VideoMode mode(
                    config_.sbs_key ? screen_width_ * 2 : screen_width_, screen_height_, desktop.bitsPerPixel);
                window_.create(mode,
                               u8(print()),
                               window_style,
                               sf::ContextSettings(0, 0, 0, 4, 5, sf::ContextSettings::Attribute::Core));
                window_.setPosition(sf::Vector2i(screen_x_, screen_y_));
                window_.setMouseCursorVisible(config_.interactive);
                window_.setActive(true);

                if (config_.always_on_top) {
#ifdef _MSC_VER
                    HWND hwnd = window_.getSystemHandle();
                    if (hwnd) {
                        if (!SetWindowPos(hwnd, HWND_TOPMOST, screen_x_, screen_y_,
                                          screen_width_, screen_height_, SWP_SHOWWINDOW)) {
                            CASPAR_LOG(warning) << print() << L" Failed to set always-on-top.";
                        }
                    }
#else
                    window_always_on_top(window_);
#endif
                }

                if (glewInit() != GLEW_OK) {
                    CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to initialize GLEW."));
                }

                if (!GLEW_VERSION_4_5 && (glewIsSupported("GL_ARB_sync GL_ARB_shader_objects GL_ARB_multitexture "
                                                          "GL_ARB_direct_state_access GL_ARB_texture_barrier") == 0u)) {
                    CASPAR_THROW_EXCEPTION(not_supported() << msg_info(
                                               "Your graphics card does not meet the minimum hardware requirements "
                                               "since it does not support OpenGL 4.5 or higher."));
                }

                GL(glGenVertexArrays(1, &vao_));
                GL(glGenBuffers(1, &vbo_));
                GL(glBindVertexArray(vao_));
                GL(glBindBuffer(GL_ARRAY_BUFFER, vbo_));

                shader_ = get_shader();
                shader_->use();
                shader_->set("background", 0);
                shader_->set("window_width", screen_width_);
                shader_->set("brightness_boost", static_cast<float>(config_.brightness_boost));
                shader_->set("saturation_boost", static_cast<float>(config_.saturation_boost));
                shader_->set("colour_space", config_.colour_space);

                if (config_.colour_space == configuration::colour_spaces::datavideo_full ||
                    config_.colour_space == configuration::colour_spaces::datavideo_limited) {
                    CASPAR_LOG(info) << print() << " Enabled colour conversion for DataVideo TC-100/TC-200 "
                                     << (config_.colour_space == configuration::colour_spaces::datavideo_full
                                             ? "(Full Range)."
                                             : "(Limited Range).");
                }

                for (int n = 0; n < 2; ++n) {
                    frames_.push_back(strategy_->init_frame(config_, format_desc_));
                }

                GL(glDisable(GL_DEPTH_TEST));
                GL(glClearColor(0.0, 0.0, 0.0, 0.0));
                GL(glViewport(
                    0, 0, config_.sbs_key ? format_desc_.width * 2 : format_desc_.width, format_desc_.height));

                calculate_aspect();

                window_.setVerticalSyncEnabled(config_.vsync);
                if (config_.vsync) {
                    CASPAR_LOG(info) << print() << " Enabled vsync.";
                }

                while (is_running_) {
                    strategy_->do_tick(this);
                }
            } catch (tbb::user_abort&) {
                // Do nothing
            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
                is_running_ = false;
            }

            for (auto& frame : frames_) {
                strategy_->cleanup_frame(frame);
            }

            shader_.reset();
            GL(glDeleteVertexArrays(1, &vao_));
            GL(glDeleteBuffers(1, &vbo_));

            window_.close();
        });
    }

    ~screen_consumer()
    {
        is_running_ = false;
        frame_buffer_.abort();
        thread_.join();
    }

    bool poll()
    {
        int       count = 0;
        sf::Event e;
        while (window_.pollEvent(e)) {
            count++;
            if (e.type == sf::Event::Resized) {
                calculate_aspect();
            } else if (e.type == sf::Event::Closed) {
                is_running_ = false;
            }
        }
        return count > 0;
    }

    void draw()
    {
        GL(glBindBuffer(GL_ARRAY_BUFFER, vbo_));
        GL(glBufferData(GL_ARRAY_BUFFER,
                        static_cast<GLsizeiptr>(sizeof(core::frame_geometry::coord)) * draw_coords_.size(),
                        draw_coords_.data(),
                        GL_STATIC_DRAW));

        auto stride = static_cast<GLsizei>(sizeof(core::frame_geometry::coord));

        auto vtx_loc = shader_->get_attrib_location("Position");
        auto tex_loc = shader_->get_attrib_location("TexCoordIn");

        GL(glEnableVertexAttribArray(vtx_loc));
        GL(glEnableVertexAttribArray(tex_loc));

        GL(glVertexAttribPointer(vtx_loc, 2, GL_DOUBLE, GL_FALSE, stride, nullptr));
        GL(glVertexAttribPointer(tex_loc, 4, GL_DOUBLE, GL_FALSE, stride, (GLvoid*)(2 * sizeof(GLdouble))));

        shader_->set("window_width", screen_width_);

        if (config_.sbs_key) {
            auto coords_size = static_cast<GLsizei>(draw_coords_.size());

            // First half fill
            shader_->set("key_only", false);
            GL(glDrawArrays(GL_TRIANGLES, 0, coords_size / 2));

            // Second half key
            shader_->set("key_only", true);
            GL(glDrawArrays(GL_TRIANGLES, coords_size / 2, coords_size / 2));
        } else {
            shader_->set("key_only", config_.key_only);
            GL(glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(draw_coords_.size())));
        }

        GL(glDisableVertexAttribArray(vtx_loc));
        GL(glDisableVertexAttribArray(tex_loc));

        GL(glBindTexture(GL_TEXTURE_2D, 0));
    }

    std::future<bool> send(core::video_field field, const core::const_frame& frame)
    {
        const int MAX_TRIES = 3;
        int       count     = 0;
        while (count++ < MAX_TRIES && is_running_) {
            if (frame_buffer_.try_push(frame))
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if (count == MAX_TRIES) {
            graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
        }
        return make_ready_future(is_running_.load());
    }

    std::wstring channel_and_format() const
    {
        return L"[" + std::to_wstring(channel_index_) + L"|" + format_desc_.name + L"]";
    }

    std::wstring print() const { return config_.name + L" " + channel_and_format(); }

    // Returns the effective aspect ratio: explicit config value, or derived from format square dimensions.
    double effective_aspect_ratio() const
    {
        if (config_.aspect_ratio != 0.0)
            return config_.aspect_ratio;
        return static_cast<double>(square_width_) / static_cast<double>(square_height_);
    }

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

        if (config_.sbs_key) {
            draw_coords_ = {
                // First half fill
                {-target_ratio.first, target_ratio.second, 0.0, 0.0}, // upper left
                {0, target_ratio.second, 1.0, 0.0},                   // upper right
                {0, -target_ratio.second, 1.0, 1.0},                  // lower right

                {-target_ratio.first, target_ratio.second, 0.0, 0.0},  // upper left
                {0, -target_ratio.second, 1.0, 1.0},                   // lower right
                {-target_ratio.first, -target_ratio.second, 0.0, 1.0}, // lower left

                // Second half key
                {0, target_ratio.second, 0.0, 0.0},                   // upper left
                {target_ratio.first, target_ratio.second, 1.0, 0.0},  // upper right
                {target_ratio.first, -target_ratio.second, 1.0, 1.0}, // lower right

                {0, target_ratio.second, 0.0, 0.0},                   // upper left
                {target_ratio.first, -target_ratio.second, 1.0, 1.0}, // lower right
                {0, -target_ratio.second, 0.0, 1.0}                   // lower left
            };
        } else {
            draw_coords_ = {
                {-target_ratio.first, target_ratio.second, 0.0, 0.0}, // upper left
                {target_ratio.first, target_ratio.second, 1.0, 0.0},  // upper right
                {target_ratio.first, -target_ratio.second, 1.0, 1.0}, // lower right

                {-target_ratio.first, target_ratio.second, 0.0, 0.0}, // upper left
                {target_ratio.first, -target_ratio.second, 1.0, 1.0}, // lower right
                {-target_ratio.first, -target_ratio.second, 0.0, 1.0} // lower left
            };
        }
    }

    std::pair<float, float> uniform() const
    {
        float aspect = static_cast<float>(config_.sbs_key ? effective_aspect_ratio() * 2 : effective_aspect_ratio());
        float width  = std::min(1.0f, static_cast<float>(screen_height_) * aspect / static_cast<float>(screen_width_));
        float height = static_cast<float>(screen_width_ * width) / static_cast<float>(screen_height_ * aspect);

        return std::make_pair(width, height);
    }

    std::pair<float, float> none() const
    {
        float aspect = static_cast<float>(config_.sbs_key ? effective_aspect_ratio() * 2 : effective_aspect_ratio());
        float width  = aspect * static_cast<float>(format_desc_.height) / static_cast<float>(screen_width_);
        float height = static_cast<float>(format_desc_.height) / static_cast<float>(screen_height_);

        return std::make_pair(width, height);
    }

    static std::pair<float, float> Fill() { return std::make_pair(1.0f, 1.0f); }

    std::pair<float, float> uniform_to_fill() const
    {
        float aspect = static_cast<float>(config_.sbs_key ? effective_aspect_ratio() * 2 : effective_aspect_ratio());
        float wr     = aspect * static_cast<float>(format_desc_.height) / static_cast<float>(screen_width_);
        float hr     = static_cast<float>(format_desc_.height) / static_cast<float>(screen_height_);
        float r_inv  = 1.0f / std::min(wr, hr);

        float width  = wr * r_inv;
        float height = hr * r_inv;

        return std::make_pair(width, height);
    }
};

struct host_strategy : public display_strategy
{
    virtual ~host_strategy() {}

    virtual frame init_frame(const configuration& config, const core::video_format_desc& format_desc) override
    {
        screen::frame frame;
        auto          flags           = GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_WRITE_BIT;
        auto          size_multiplier = config.high_bitdepth ? 2 : 1;

        GL(glCreateBuffers(1, &frame.pbo));
        GL(glNamedBufferStorage(frame.pbo, format_desc.size * size_multiplier, nullptr, flags));
        frame.ptr = reinterpret_cast<char*>(
            GL2(glMapNamedBufferRange(frame.pbo, 0, format_desc.size * size_multiplier, flags)));

        GL(glCreateTextures(GL_TEXTURE_2D, 1, &frame.tex));

        const GLenum filter_mode = (config.colour_space == configuration::colour_spaces::datavideo_full ||
                                    config.colour_space == configuration::colour_spaces::datavideo_limited)
                                       ? GL_NEAREST
                                       : GL_LINEAR;

        GL(glTextureParameteri(frame.tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GL(glTextureParameteri(frame.tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

        // Mipmaps can improve quality when content is displayed at significantly less than native resolution
        // (e.g. high pixel-pitch LED walls). Capped at 2 levels to avoid unnecessary GPU overhead.
        int mip_levels = 1;
        if (config.enable_mipmaps) {
            mip_levels = 2;
            CASPAR_LOG(debug) << L"Screen consumer: mipmaps enabled (2 levels).";
            GL(glTextureParameteri(frame.tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
            GL(glTextureParameteri(frame.tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        } else {
            GL(glTextureParameteri(frame.tex, GL_TEXTURE_MIN_FILTER, filter_mode));
            GL(glTextureParameteri(frame.tex, GL_TEXTURE_MAG_FILTER, filter_mode));
        }

        GL(glTextureStorage2D(
            frame.tex, mip_levels, config.high_bitdepth ? GL_RGBA16 : GL_RGBA8, format_desc.width, format_desc.height));
        GL(glClearTexImage(
            frame.tex, 0, GL_BGRA, config.high_bitdepth ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE, nullptr));

        return frame;
    }

    virtual void cleanup_frame(frame& frame) override
    {
        GL(glUnmapNamedBuffer(frame.pbo));
        glDeleteBuffers(1, &frame.pbo);
        glDeleteTextures(1, &frame.tex);
    }

    virtual void do_tick(screen_consumer* self) override
    {
        core::const_frame in_frame;

        while (!self->frame_buffer_.try_pop(in_frame) && self->is_running_) {
            if (!self->poll()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        }

        if (!in_frame) {
            return;
        }

        // Upload
        {
            auto& frame = self->frames_.front();

            while (frame.fence != nullptr) {
                auto wait = glClientWaitSync(frame.fence, 0, 0);
                if (wait == GL_ALREADY_SIGNALED || wait == GL_CONDITION_SATISFIED) {
                    glDeleteSync(frame.fence);
                    frame.fence = nullptr;
                }
                if (!self->poll()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                }
            }

            auto size_multiplier = self->config_.high_bitdepth ? 2 : 1;
            std::memcpy(frame.ptr, in_frame.image_data(0).begin(), self->format_desc_.size * size_multiplier);

            GL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, frame.pbo));
            GL(glTextureSubImage2D(frame.tex,
                                   0,
                                   0,
                                   0,
                                   self->format_desc_.width,
                                   self->format_desc_.height,
                                   GL_BGRA,
                                   self->config_.high_bitdepth ? GL_UNSIGNED_SHORT : GL_UNSIGNED_BYTE,
                                   nullptr));

            if (self->config_.enable_mipmaps) {
                GL(glGenerateTextureMipmap(frame.tex));
            }

            GL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
            frame.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        }

        // Display
        {
            auto& frame = self->frames_.back();

            GL(glClear(GL_COLOR_BUFFER_BIT));

            GL(glActiveTexture(GL_TEXTURE0));
            GL(glBindTexture(GL_TEXTURE_2D, frame.tex));

            self->draw();
        }

        self->window_.display();

        std::rotate(self->frames_.begin(), self->frames_.begin() + 1, self->frames_.end());

        self->graph_->set_value("tick-time", self->tick_timer_.elapsed() * self->format_desc_.fps * 0.5);
        self->tick_timer_.restart();
    }
};

struct gpu_strategy : public display_strategy
{
    virtual ~gpu_strategy() {}

    virtual frame init_frame(const configuration& config, const core::video_format_desc& format_desc) override
    {
        return frame();
    }

    virtual void cleanup_frame(frame& frame) override
    {
        if (frame.fence) {
            glDeleteSync(frame.fence);
            frame.fence = nullptr;
        }
        frame.texture.reset();
    }

    virtual void do_tick(screen_consumer* self) override
    {
        core::const_frame in_frame;

        self->poll();

        while (!self->frame_buffer_.try_pop(in_frame) && self->is_running_) {
            if (!self->poll()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        }

        // Display
        {
            auto& frame = self->frames_.front();

            while (frame.fence != nullptr && self->is_running_) {
                auto wait = glClientWaitSync(frame.fence, 0, 0);
                if (wait == GL_ALREADY_SIGNALED || wait == GL_CONDITION_SATISFIED) {
                    glDeleteSync(frame.fence);
                    frame.fence = nullptr;
                    frame.texture.reset();
                }

                if (!self->poll()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                }
            }

            if (!in_frame || !self->is_running_) {
                self->graph_->set_value("tick-time", self->tick_timer_.elapsed() * self->format_desc_.fps * 0.5);
                self->tick_timer_.restart();
                return;
            }

            GL(glClear(GL_COLOR_BUFFER_BIT));

            if (in_frame.texture()) {
                in_frame.texture()->bind(0);

                self->draw();

                frame.fence   = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
                frame.texture = in_frame.texture();
            }
        }

        self->window_.display();

        std::rotate(self->frames_.begin(), self->frames_.begin() + 1, self->frames_.end());

        self->graph_->set_value("tick-time", self->tick_timer_.elapsed() * self->format_desc_.fps * 0.5);
        self->tick_timer_.restart();
    }
};

struct screen_consumer_proxy : public core::frame_consumer
{
    const configuration              config_;
    std::unique_ptr<screen_consumer> consumer_;

  public:
    explicit screen_consumer_proxy(configuration config)
        : config_(std::move(config))
    {
    }

    void initialize(const core::video_format_desc& format_desc,
                    const core::channel_info&      channel_info,
                    int                            port_index) override
    {
        consumer_.reset();
        consumer_ = std::make_unique<screen_consumer>(config_, format_desc, channel_info.index);
    }

    std::future<bool> send(core::video_field field, core::const_frame frame) override
    {
        return consumer_->send(field, frame);
    }

    std::wstring print() const override { return consumer_ ? consumer_->print() : L"[screen_consumer]"; }

    std::wstring name() const override { return L"screen"; }

    bool has_synchronization_clock() const override { return false; }

    int index() const override { return 600 + (config_.key_only ? 10 : 0) + config_.screen_index; }

    core::monitor::state state() const override
    {
        core::monitor::state state;
        state["screen/name"]          = config_.name;
        state["screen/index"]         = config_.screen_index;
        state["screen/key_only"]      = config_.key_only;
        state["screen/always_on_top"] = config_.always_on_top;
        state["screen/gpu_texture"]   = config_.gpu_texture;
        return state;
    }
};

spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>&     params,
                                                      const core::video_format_repository& format_repository,
                                                      const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                                                      const core::channel_info& channel_info)
{
    if (params.empty() || !boost::iequals(params.at(0), L"SCREEN")) {
        return core::frame_consumer::empty();
    }

    configuration config;

    config.high_bitdepth = (channel_info.depth != common::bit_depth::bit8);

    if (params.size() > 1) {
        try {
            config.screen_index = std::stoi(params.at(1));
        } catch (...) {
        }
    }

    config.windowed    = !contains_param(L"FULLSCREEN", params);
    config.gpu_texture = contains_param(L"GPU", params);
    config.key_only    = contains_param(L"KEY_ONLY", params);
    config.sbs_key     = contains_param(L"SBS_KEY", params);
    config.interactive = !contains_param(L"NON_INTERACTIVE", params);
    config.borderless  = contains_param(L"BORDERLESS", params);

    if (contains_param(L"NAME", params)) {
        config.name = get_param(L"NAME", params);
    }

    if (contains_param(L"X", params)) {
        config.screen_x = get_param(L"X", params, 0);
    }
    if (contains_param(L"Y", params)) {
        config.screen_y = get_param(L"Y", params, 0);
    }
    if (contains_param(L"WIDTH", params)) {
        config.screen_width = get_param(L"WIDTH", params, 0);
    }
    if (contains_param(L"HEIGHT", params)) {
        config.screen_height = get_param(L"HEIGHT", params, 0);
    }

    if (config.sbs_key && config.key_only) {
        CASPAR_LOG(warning) << L" Key-only not supported with configuration of side-by-side fill and key. Ignored.";
        config.key_only = false;
    }

    return spl::make_shared<screen_consumer_proxy>(config);
}

spl::shared_ptr<core::frame_consumer>
create_preconfigured_consumer(const boost::property_tree::wptree&                      ptree,
                              const core::video_format_repository&                     format_repository,
                              const std::vector<spl::shared_ptr<core::video_channel>>& channels,
                              const core::channel_info&                                channel_info)
{
    configuration config;

    config.high_bitdepth = (channel_info.depth != common::bit_depth::bit8);

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
    config.gpu_texture   = ptree.get(L"gpu-texture", config.gpu_texture);

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
        config.stretch = screen::stretch::none;
    } else if (stretch_str == L"uniform") {
        config.stretch = screen::stretch::uniform;
    } else if (stretch_str == L"uniform_to_fill") {
        config.stretch = screen::stretch::uniform_to_fill;
    }

    // Aspect ratio priority: explicit value > derived from width/height > auto (0.0, use format square dims)
    auto aspect_str = ptree.get_optional<std::wstring>(L"aspect-ratio");
    if (aspect_str) {
        config.aspect_ratio = parse_aspect_ratio(*aspect_str);
    } else if (config.screen_width > 0 && config.screen_height > 0) {
        config.aspect_ratio = static_cast<double>(config.screen_width) / static_cast<double>(config.screen_height);
    }
    // else: aspect_ratio remains 0.0 (auto)

    config.enable_mipmaps   = ptree.get(L"enable-mipmaps", false);
    config.brightness_boost = ptree.get(L"brightness-boost", 1.0);
    config.saturation_boost = ptree.get(L"saturation-boost", 1.0);

    return spl::make_shared<screen_consumer_proxy>(config);
}

}} // namespace caspar::screen
