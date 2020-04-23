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

#include "../stdafx.h"

#if defined(_MSC_VER)
#pragma warning(disable : 4146)
#pragma warning(disable : 4244)
#endif

#include "FlashAxContainer.h"
#include "flash_producer.h"

#include "../util/swf.h"

#include <core/video_format.h>

#include <core/frame/draw_frame.h>
#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/pixel_format.h>
#include <core/monitor/monitor.h>
#include <core/producer/frame_producer.h>

#include <common/array.h>
#include <common/diagnostics/graph.h>
#include <common/env.h>
#include <common/executor.h>
#include <common/future.h>
#include <common/prec_timer.h>
#include <common/timer.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

namespace caspar { namespace flash {

class bitmap
{
  public:
    bitmap(int width, int height)
        : bmp_data_(nullptr)
        , hdc_(CreateCompatibleDC(nullptr), DeleteDC)
    {
        BITMAPINFO info;
        std::memset(&info, 0, sizeof(BITMAPINFO));
        info.bmiHeader.biBitCount    = 32;
        info.bmiHeader.biCompression = BI_RGB;
        info.bmiHeader.biHeight      = static_cast<LONG>(-height);
        info.bmiHeader.biPlanes      = 1;
        info.bmiHeader.biSize        = sizeof(BITMAPINFO);
        info.bmiHeader.biWidth       = static_cast<LONG>(width);

        bmp_.reset(
            CreateDIBSection(
                static_cast<HDC>(hdc_.get()), &info, DIB_RGB_COLORS, reinterpret_cast<void**>(&bmp_data_), nullptr, 0),
            DeleteObject);
        SelectObject(static_cast<HDC>(hdc_.get()), bmp_.get());

        if (!bmp_data_)
            CASPAR_THROW_EXCEPTION(bad_alloc());
    }

    operator HDC() { return static_cast<HDC>(hdc_.get()); }

    BYTE*       data() { return bmp_data_; }
    const BYTE* data() const { return bmp_data_; }

  private:
    BYTE*                 bmp_data_;
    std::shared_ptr<void> hdc_;
    std::shared_ptr<void> bmp_;
};

struct template_host
{
    std::wstring video_mode;
    std::wstring filename;
    int          width;
    int          height;
};

template_host get_template_host(const core::video_format_desc& desc)
{
    try {
        std::vector<template_host> template_hosts;
        auto template_hosts_element = env::properties().get_child_optional(L"configuration.template-hosts");

        if (template_hosts_element) {
            for (auto& xml_mapping : *template_hosts_element) {
                try {
                    template_host template_host;
                    template_host.video_mode = xml_mapping.second.get(L"video-mode", L"");
                    template_host.filename   = xml_mapping.second.get(L"filename", L"cg.fth");
                    template_host.width      = xml_mapping.second.get(L"width", desc.width);
                    template_host.height     = xml_mapping.second.get(L"height", desc.height);
                    template_hosts.push_back(template_host);
                } catch (...) {
                }
            }
        }

        auto template_host_it = boost::find_if(
            template_hosts, [&](template_host template_host) { return template_host.video_mode == desc.name; });
        if (template_host_it == template_hosts.end())
            template_host_it = boost::find_if(
                template_hosts, [&](template_host template_host) { return template_host.video_mode.empty(); });

        if (template_host_it != template_hosts.end())
            return *template_host_it;
    } catch (...) {
    }

    template_host template_host;
    template_host.filename = L"cg.fth";

    for (auto it = boost::filesystem::directory_iterator(env::template_folder());
         it != boost::filesystem::directory_iterator();
         ++it) {
        if (boost::iequals(it->path().extension().wstring(), L"." + desc.name)) {
            template_host.filename = it->path().filename().wstring();
            break;
        }
    }

    template_host.width  = desc.square_width;
    template_host.height = desc.square_height;
    return template_host;
}

std::mutex& get_global_init_destruct_mutex()
{
    static std::mutex m;

    return m;
}

class flash_renderer
{
    struct com_init
    {
        HRESULT result_ = CoInitialize(nullptr);

        com_init()
        {
            if (FAILED(result_))
                CASPAR_THROW_EXCEPTION(caspar_exception()
                                       << msg_info("Failed to initialize com-context for flash-player"));
        }

        ~com_init()
        {
            if (SUCCEEDED(result_))
                ::CoUninitialize();
        }
    } com_init_;

    const std::wstring filename_;
    const int          width_;
    const int          height_;

    const std::shared_ptr<core::frame_factory> frame_factory_;

    CComObject<caspar::flash::FlashAxContainer>* ax_ = nullptr;
    core::draw_frame                             head_;
    bitmap                                       bmp_{width_, height_};
    prec_timer                                   timer_;
    caspar::timer                                tick_timer_;

    spl::shared_ptr<diagnostics::graph> graph_;

  public:
    flash_renderer(const spl::shared_ptr<diagnostics::graph>&  graph,
                   const std::shared_ptr<core::frame_factory>& frame_factory,
                   const std::wstring&                         filename,
                   int                                         width,
                   int                                         height)
        : filename_(filename)
        , width_(width)
        , height_(height)
        , frame_factory_(frame_factory)
        , bmp_(width, height)
        , graph_(graph)
    {
        graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
        graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("param", diagnostics::color(1.0f, 0.5f, 0.0f));

        if (FAILED(CComObject<caspar::flash::FlashAxContainer>::CreateInstance(&ax_)))
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to create FlashAxContainer"));

        if (FAILED(ax_->CreateAxControl()))
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to Create FlashAxControl"));

        ax_->set_print([this] { return print(); });

        CComPtr<IShockwaveFlash> spFlash;
        if (FAILED(ax_->QueryControl(&spFlash)))
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to Query FlashAxControl"));

        if (FAILED(spFlash->put_Playing(VARIANT_TRUE)))
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to start playing Flash"));

        // Concurrent initialization of two template hosts causes a
        // SecurityException later when CG ADD is performed. Initialization is
        // therefore serialized via a global mutex.
        {
            std::lock_guard<std::mutex> lock(get_global_init_destruct_mutex());
            if (FAILED(spFlash->put_Movie(CComBSTR(filename.c_str()))))
                CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to Load Template Host"));
        }

        if (FAILED(spFlash->put_ScaleMode(2))) // Exact fit. Scale without respect to the aspect ratio.
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to Set Scale Mode"));

        ax_->SetSize(width_, height_);
        render_frame(0.0);

        CASPAR_LOG(info) << print() << L" Initialized.";
    }

    ~flash_renderer()
    {
        if (ax_) {
            ax_->DestroyAxControl();
            ax_->Release();
        }
        graph_->set_value("tick-time", 0.0f);
        graph_->set_value("frame-time", 0.0f);
        CASPAR_LOG(info) << print() << L" Uninitialized.";
    }

    std::wstring call(const std::wstring& param)
    {
        std::wstring result;

        CASPAR_LOG(debug) << print() << " Call: " << param;

        if (!ax_->FlashCall(param, result))
            CASPAR_LOG(warning) << print() << L" Flash call failed:"
                                << param; // CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("Flash function call
                                          // failed.") << arg_name_info("param") << arg_value_info(narrow(param)));

        if (boost::starts_with(result, L"<exception>"))
            CASPAR_LOG(warning) << print() << L" Flash call failed:" << result;

        graph_->set_tag(diagnostics::tag_severity::INFO, "param");

        return result;
    }

    core::draw_frame render_frame(double sync)
    {
        const float frame_time = 1.0f / fps();

        if (!ax_->IsReadyToRender()) {
            return head_;
        }

        if (is_empty()) {
            return core::draw_frame::empty();
        }

        if (sync > 0.00001)
            timer_.tick(frame_time * sync); // This will block the thread.

        graph_->set_value("tick-time", static_cast<float>(tick_timer_.elapsed() / frame_time) * 0.5f);
        tick_timer_.restart();
        caspar::timer frame_timer;
        ax_->Tick();

        if (ax_->InvalidRect()) {
            core::pixel_format_desc desc = core::pixel_format::bgra;
            desc.planes.push_back(core::pixel_format_desc::plane(width_, height_, 4));
            auto frame = frame_factory_->create_frame(this, desc);

            std::memset(bmp_.data(), 0, width_ * height_ * 4);
            ax_->DrawControl(bmp_);

            std::memcpy(frame.image_data(0).begin(), bmp_.data(), width_ * height_ * 4);
            head_ = core::draw_frame(std::move(frame));
        }

        MSG msg;
        while (PeekMessage(&msg,
                           nullptr,
                           NULL,
                           NULL,
                           PM_REMOVE)) // DO NOT REMOVE THE MESSAGE DISPATCH LOOP. Without this some stuff doesn't work!
        {
            if (msg.message == WM_TIMER && msg.wParam == 3 && msg.lParam == 0) // We tick this inside FlashAxContainer
                continue;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        graph_->set_value("frame-time", static_cast<float>(frame_timer.elapsed() / frame_time) * 0.5f);
        return head_;
    }

    bool is_empty() const { return ax_->IsEmpty(); }

    double fps() const { return ax_->GetFPS(); }

    std::wstring print()
    {
        return L"flash-player[" + boost::filesystem::path(filename_).filename().wstring() + L"|" +
               std::to_wstring(width_) + L"x" + std::to_wstring(height_) + L"]";
    }
};

struct flash_producer : public core::frame_producer
{
    core::monitor::state                       state_;
    const std::wstring                         filename_;
    const spl::shared_ptr<core::frame_factory> frame_factory_;
    const core::video_format_desc              format_desc_;
    const int                                  width_;
    const int                                  height_;
    const int                                  buffer_size_ =
        env::properties().get(L"configuration.flash.buffer-depth", format_desc_.fps > 30.0 ? 4 : 2);

    std::atomic<int> fps_;

    spl::shared_ptr<diagnostics::graph> graph_;

    std::queue<core::draw_frame>                    frame_buffer_;
    tbb::concurrent_bounded_queue<core::draw_frame> output_buffer_;

    core::draw_frame last_frame_;

    std::unique_ptr<flash_renderer> renderer_;
    std::atomic<bool>               has_renderer_;

    executor executor_ = L"flash_producer";

  public:
    flash_producer(const spl::shared_ptr<core::frame_factory>& frame_factory,
                   const core::video_format_desc&              format_desc,
                   const std::wstring&                         filename,
                   int                                         width,
                   int                                         height)
        : filename_(filename)
        , frame_factory_(frame_factory)
        , format_desc_(format_desc)
        , width_(width > 0 ? width : format_desc.width)
        , height_(height > 0 ? height : format_desc.height)
    {
        fps_ = 0;

        graph_->set_color("buffered", diagnostics::color(1.0f, 1.0f, 0.0f));
        graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.9f));
        graph_->set_text(print());
        diagnostics::register_graph(graph_);

        has_renderer_ = false;

        CASPAR_LOG(info) << print() << L" Initialized";
    }

    ~flash_producer()
    {
        executor_.invoke([this] { renderer_.reset(); });
    }

    // frame_producer

    void log_buffered()
    {
        double buffered = output_buffer_.size();
        auto   ratio    = buffered / buffer_size_;
        graph_->set_value("buffered", ratio);
    }

    core::draw_frame receive_impl(int nb_samples) override
    {
        auto frame = last_frame_;

        double buffered = output_buffer_.size();
        auto   ratio    = buffered / buffer_size_;
        graph_->set_value("buffered", ratio);

        if (output_buffer_.try_pop(frame))
            last_frame_ = frame;
        else
            graph_->set_tag(diagnostics::tag_severity::WARNING, "late-frame");

        fill_buffer();

        state_["host/path"]   = filename_;
        state_["host/width"]  = width_;
        state_["host/height"] = height_;
        state_["host/fps"]    = fps_;
        state_["buffer"]      = output_buffer_.size() % buffer_size_;

        return frame;
    }

    std::future<std::wstring> call(const std::vector<std::wstring>& params) override
    {
        auto param = boost::algorithm::join(params, L" ");

        if (param == L"?")
            return make_ready_future(std::wstring(has_renderer_ ? L"1" : L"0"));

        return executor_.begin_invoke([this, param]() -> std::wstring {
            try {
                bool initialize_renderer = !renderer_;

                if (initialize_renderer) {
                    renderer_.reset(new flash_renderer(graph_, frame_factory_, filename_, width_, height_));

                    has_renderer_ = true;
                }

                std::wstring result = param == L"start_rendering" ? L"" : renderer_->call(param);

                if (initialize_renderer) {
                    do_fill_buffer(true);
                }

                return result;
            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
                renderer_.reset(nullptr);
                has_renderer_ = false;
            }

            return L"";
        });
    }

    std::wstring print() const override
    {
        return L"flash[" + boost::filesystem::path(filename_).wstring() + L"|" + std::to_wstring(fps_) + L"]";
    }

    std::wstring name() const override { return L"flash"; }

    core::monitor::state state() const override { return state_; }

    // flash_producer

    void fill_buffer()
    {
        if (executor_.size() > 0)
            return;

        executor_.begin_invoke([this] { do_fill_buffer(false); });
    }

    void do_fill_buffer(bool initial_buffer_fill)
    {
        int       nothing_rendered             = 0;
        const int MAX_NOTHING_RENDERED_RETRIES = 4;

        auto to_render              = buffer_size_ - output_buffer_.size();
        bool allow_faster_rendering = !initial_buffer_fill;
        int  rendered               = 0;

        while (rendered < to_render) {
            bool was_rendered = next(allow_faster_rendering);
            log_buffered();

            if (was_rendered) {
                ++rendered;
            } else {
                if (nothing_rendered++ < MAX_NOTHING_RENDERED_RETRIES) {
                    // Flash player not ready with first frame, sleep to not busy-loop;
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    std::this_thread::yield();
                } else
                    return;
            }

            // TODO yield
        }
    }

    bool next(bool allow_faster_rendering)
    {
        if (!renderer_)
            frame_buffer_.push(core::draw_frame{});

        if (frame_buffer_.empty()) {
            if (abs(renderer_->fps() - format_desc_.fps / 2.0) < 2.0) // format == 2 * flash -> duplicate
            {
                auto frame = render_frame(allow_faster_rendering);
                if (frame) {
                    frame_buffer_.push(frame);
                    frame_buffer_.push(frame);
                }
            } else // if(abs(renderer_->fps() - format_desc_.fps) < 0.1) // format == flash -> simple
            {
                auto frame = render_frame(allow_faster_rendering);
                if (frame) {
                    frame_buffer_.push(frame);
                }
            }

            fps_ = static_cast<int>(renderer_->fps() * 100.0);
            graph_->set_text(print());

            if (renderer_->is_empty()) {
                renderer_.reset();
                has_renderer_ = false;
            }
        }

        if (frame_buffer_.empty()) {
            return false;
        }
        output_buffer_.push(std::move(frame_buffer_.front()));
        frame_buffer_.pop();
        return true;
    }

    core::draw_frame render_frame(bool allow_faster_rendering)
    {
        double sync;

        if (allow_faster_rendering) {
            double ratio = std::min(
                1.0, static_cast<double>(output_buffer_.size()) / static_cast<double>(std::max(1, buffer_size_ - 1)));
            sync = 2 * ratio - ratio * ratio;
        } else {
            sync = 1.0;
        }

        return renderer_->render_frame(sync);
    }
};

spl::shared_ptr<core::frame_producer> create_producer(const core::frame_producer_dependencies& dependencies,
                                                      const std::vector<std::wstring>&         params)
{
    auto template_host = get_template_host(dependencies.format_desc);

    auto filename = env::template_folder() + L"\\" + template_host.filename;

    if (!boost::filesystem::exists(filename))
        CASPAR_THROW_EXCEPTION(file_not_found() << msg_info(L"Could not open flash movie " + filename));

    return create_destroy_proxy(spl::make_shared<flash_producer>(
        dependencies.frame_factory, dependencies.format_desc, filename, template_host.width, template_host.height));
}

spl::shared_ptr<core::frame_producer> create_swf_producer(const core::frame_producer_dependencies& dependencies,
                                                          const std::vector<std::wstring>&         params)
{
    auto filename = env::media_folder() + L"\\" + params.at(0) + L".swf";

    if (!boost::filesystem::exists(filename))
        return core::frame_producer::empty();

    swf_t::header_t header(filename);

    auto producer = spl::make_shared<flash_producer>(
        dependencies.frame_factory, dependencies.format_desc, filename, header.frame_width, header.frame_height);

    producer->call({L"start_rendering"}).get();

    return create_destroy_proxy(producer);
}

std::wstring find_template(const std::wstring& template_name)
{
    if (boost::filesystem::exists(template_name + L".ft"))
        return template_name + L".ft";

    if (boost::filesystem::exists(template_name + L".ct"))
        return template_name + L".ct";

    if (boost::filesystem::exists(template_name + L".swf"))
        return template_name + L".swf";

    return L"";
}

}} // namespace caspar::flash
