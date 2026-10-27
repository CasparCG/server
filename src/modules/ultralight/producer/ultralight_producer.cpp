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
 * Author: Ole Kristensen, Den Frie Vilje ApS, ole@kristensen.name
 *
 * Synchronous HTML producer using Ultralight (WebKit fork).
 * Renders one frame per receive_impl() call — no wall-clock dependency.
 * Enables faster-than-real-time offline rendering with HTML templates.
 */

#include "ultralight_producer.h"

#include <common/diagnostics/graph.h>
#include <common/env.h>
#include <common/future.h>
#include <common/log.h>
#include <common/os/filesystem.h>
#include <common/timer.h>
#include <common/utf.h>

#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/pixel_format.h>
#include <core/producer/frame_producer.h>
#include <core/video_format.h>

#include <Ultralight/Ultralight.h>
#include <Ultralight/Renderer.h>
#include <Ultralight/View.h>
#include <Ultralight/platform/Config.h>
#include <Ultralight/platform/Platform.h>
#include <Ultralight/platform/Surface.h>
#include <Ultralight/Bitmap.h>

// We implement our own FileSystem instead of using AppCore's GetPlatformFileSystem
// which crashes with file:// URLs in headless Docker containers.
#include <AppCore/Platform.h> // still needed for GetPlatformFontLoader()

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

namespace ul = ::ultralight;

namespace caspar { namespace ultralight {

// ---------------------------------------------------------------------------
// Custom FileSystem — simple POSIX implementation for Ultralight.
// The AppCore GetPlatformFileSystem() crashes with file:// URLs in Docker.
// This implementation handles all file I/O directly via std::ifstream.
// ---------------------------------------------------------------------------
class caspar_file_system : public ul::FileSystem
{
public:
    // file_path is the string after "file:///" — e.g. "templates/sync_test.html"
    // We resolve it using boost::filesystem and CasparCG's find_case_insensitive
    // for cross-platform compatibility (Windows, macOS, Linux).

    bool FileExists(const ul::String& file_path) override
    {
        auto raw = std::string(file_path.utf8().data(), file_path.utf8().length());
        auto resolved = resolve_path(file_path);
        bool exists = boost::filesystem::exists(resolved);
        CASPAR_LOG(info) << L"[ultralight-fs] FileExists('" << u16(raw) << L"') -> resolved='"
                         << resolved << L"' exists=" << (exists ? L"YES" : L"NO");
        return exists;
    }

    ul::String GetFileMimeType(const ul::String& file_path) override
    {
        auto p = std::string(file_path.utf8().data(), file_path.utf8().length());
        auto dot = p.rfind('.');
        if (dot == std::string::npos) return ul::String("application/octet-stream");
        auto ext = p.substr(dot + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == "html" || ext == "htm") return ul::String("text/html");
        if (ext == "css")  return ul::String("text/css");
        if (ext == "js")   return ul::String("application/javascript");
        if (ext == "json") return ul::String("application/json");
        if (ext == "png")  return ul::String("image/png");
        if (ext == "jpg" || ext == "jpeg") return ul::String("image/jpeg");
        if (ext == "gif")  return ul::String("image/gif");
        if (ext == "svg")  return ul::String("image/svg+xml");
        if (ext == "webp") return ul::String("image/webp");
        if (ext == "ttf")  return ul::String("font/ttf");
        if (ext == "woff") return ul::String("font/woff");
        if (ext == "woff2") return ul::String("font/woff2");
        if (ext == "dat")  return ul::String("application/octet-stream");
        return ul::String("application/octet-stream");
    }

    ul::String GetFileCharset(const ul::String& /*file_path*/) override
    {
        return ul::String("utf-8");
    }

    ul::RefPtr<ul::Buffer> OpenFile(const ul::String& file_path) override
    {
        auto resolved = resolve_path(file_path);
        auto resolved_narrow = u8(resolved);
        std::ifstream ifs(resolved_narrow, std::ios::binary | std::ios::ate);
        if (!ifs.good()) {
            CASPAR_LOG(warning) << L"[ultralight-fs] Not found: " << resolved;
            return nullptr;
        }
        auto size = static_cast<size_t>(ifs.tellg());
        ifs.seekg(0);

        if (size == 0) {
            return ul::Buffer::CreateFromCopy(nullptr, 0);
        }

        // Allocate 16-byte aligned buffer (required for ICU data files)
        auto alloc_size = (size + 15) & ~static_cast<size_t>(15);
        char* buf = static_cast<char*>(std::aligned_alloc(16, alloc_size));
        if (!buf) return nullptr;
        ifs.read(buf, size);

        auto buffer = ul::Buffer::Create(buf, size, buf, [](void* /*data*/, void* user) {
            std::free(user);
        });

        CASPAR_LOG(debug) << L"[ultralight-fs] Loaded: " << resolved
                          << L" (" << size << L" bytes)";
        return buffer;
    }

private:
    // Resolve Ultralight's file path to an OS path.
    // Uses CasparCG's find_case_insensitive for cross-platform compatibility.
    static std::wstring resolve_path(const ul::String& file_path)
    {
        auto utf8 = std::string(file_path.utf8().data(), file_path.utf8().length());
        auto wide = u16(utf8);

        // Ultralight strips the leading / from file:/// URLs, giving us
        // "templates/foo.html" instead of "/templates/foo.html".
        // Prepend / to make it absolute if it doesn't start with / or ./
        if (!wide.empty() && wide[0] != L'/' && wide[0] != L'.') {
            auto abs_wide = L"/" + wide;
            if (boost::filesystem::exists(abs_wide)) {
                return abs_wide;
            }
        }

        // Try the path as-is (already absolute, or relative to CWD)
        if (boost::filesystem::exists(wide)) {
            return wide;
        }

        // Try case-insensitive lookup (important on case-sensitive Linux FS)
        auto ci = caspar::find_case_insensitive(wide);
        if (ci) {
            return *ci;
        }

        // Return original path — caller will handle the "not found" case
        return wide;
    }
};

static caspar_file_system s_file_system;

// ---------------------------------------------------------------------------
// Platform singleton — initialized once, never torn down.
// ---------------------------------------------------------------------------
static std::once_flag s_platform_init_flag;

static void ensure_platform_initialized()
{
    std::call_once(s_platform_init_flag, [] {
        ul::Config config;
        config.resource_path_prefix = ul::String("./resources/");
        config.cache_path           = ul::String("/tmp/ultralight-cache");
        config.face_winding         = ul::FaceWinding::Clockwise;
        config.bitmap_alignment     = 16;

        ul::Platform::instance().set_config(config);
        ul::Platform::instance().set_font_loader(ul::GetPlatformFontLoader());
        ul::Platform::instance().set_file_system(&s_file_system);
        ul::Platform::instance().set_logger(ul::GetDefaultLogger("ultralight.log"));

        CASPAR_LOG(info) << L"Ultralight platform initialized with custom FileSystem.";
    });
}

// (Data URI asset inlining removed — custom FileSystem handles file loading)

// ---------------------------------------------------------------------------
// LoadListener — detect when page has finished loading
// ---------------------------------------------------------------------------
class ul_load_listener : public ul::LoadListener
{
    std::atomic<bool> loaded_{false};
    std::atomic<bool> failed_{false};

  public:
    void OnFinishLoading(ul::View*      caller,
                         uint64_t       frame_id,
                         bool           is_main_frame,
                         const ul::String& url) override
    {
        if (is_main_frame)
            loaded_ = true;
    }

    void OnFailLoading(ul::View*         caller,
                       uint64_t          frame_id,
                       bool              is_main_frame,
                       const ul::String& url,
                       const ul::String& description,
                       const ul::String& error_domain,
                       int               error_code) override
    {
        if (is_main_frame)
            failed_ = true;
    }

    bool is_loaded() const { return loaded_; }
    bool is_failed() const { return failed_; }
};

// ---------------------------------------------------------------------------
// ultralight_producer — synchronous frame_producer
// ---------------------------------------------------------------------------
class ultralight_producer : public core::frame_producer
{
    core::video_format_desc              format_desc_;
    spl::shared_ptr<core::frame_factory> frame_factory_;
    std::wstring                         url_;
    spl::shared_ptr<diagnostics::graph>  graph_;

    ul::RefPtr<ul::Renderer> renderer_;
    ul::RefPtr<ul::View>    view_;
    ul_load_listener load_listener_;

    core::pixel_format_desc pixel_desc_;
    std::int64_t            channel_frame_ = -1; // starts at -1 so first ++makes it 0
    std::atomic<bool>       initialized_{false};
    std::atomic<bool>       playing_{false};
    std::atomic<bool>       play_pending_{false};
    std::string             html_content_;  // cached file contents for inline HTML
    bool                    use_load_url_ = false; // true for file:// URLs

    // Guarded by producer_mutex_ — accessed from both AMCP and tick threads
    mutable std::mutex              producer_mutex_;
    std::vector<std::wstring>       pending_js_calls_;
    core::draw_frame                frozen_frame_;

    core::monitor::state state_;
    mutable std::mutex   state_mutex_;

  public:
    ultralight_producer(const spl::shared_ptr<core::frame_factory>& frame_factory,
                        const core::video_format_desc&              format_desc,
                        const std::wstring&                         url)
        : format_desc_(format_desc)
        , frame_factory_(frame_factory)
        , url_(url)
    {
        graph_ = spl::make_shared<diagnostics::graph>();
        graph_->set_color("frame-time", diagnostics::color(0.1f, 1.0f, 0.1f));
        graph_->set_text(print());
        diagnostics::register_graph(graph_);

        // Pre-read the HTML file (any thread is fine for I/O).
        // Ultralight Renderer creation is deferred to first receive_impl() call
        // because Ultralight requires all API calls on the thread that created
        // the Renderer (thread-affinity).
        // For file:// URLs, use LoadURL — our custom FileSystem handles I/O.
        // Ultralight resolves relative assets against the template's directory.
        use_load_url_ = boost::algorithm::istarts_with(url, L"file://");
        if (use_load_url_) {
            // Verify file exists before attempting load
            auto file_path = u8(url.substr(7)); // strip file://
            std::ifstream ifs(file_path);
            if (ifs.good()) {
                CASPAR_LOG(info) << print() << L" Will LoadURL: " << url;
            } else {
                CASPAR_LOG(error) << print() << L" File not found: " << u16(file_path);
            }
        }

        pixel_desc_ = core::pixel_format_desc(core::pixel_format::bgra);
        pixel_desc_.planes.emplace_back(
            core::pixel_format_desc::plane(format_desc.square_width, format_desc.square_height, 4));

        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            state_["file/path"] = u8(url_);
        }

        CASPAR_LOG(info) << print() << L" Created (lazy init). " << format_desc.square_width
                         << L"x" << format_desc.square_height;
    }

    // Called on the channel tick thread — safe to create Ultralight Renderer here.
    // Loads the HTML, pumps until the page is ready, renders one frozen frame,
    // and caches it. This frame is returned on every receive_impl() until play().
    void ensure_initialized()
    {
        if (initialized_)
            return;
        initialized_ = true;

        CASPAR_LOG(info) << print() << L" Initializing Ultralight on channel tick thread.";

        // Platform is a global singleton — only initialized once across all producers.
        ensure_platform_initialized();

        renderer_ = ul::Renderer::Create();

        ul::ViewConfig view_config;
        view_config.is_accelerated      = false;
        view_config.is_transparent      = true;
        view_config.initial_device_scale = 1.0;

        view_ = renderer_->CreateView(static_cast<uint32_t>(format_desc_.square_width),
                                       static_cast<uint32_t>(format_desc_.square_height),
                                       view_config,
                                       nullptr);
        view_->set_load_listener(&load_listener_);

        // Load via LoadURL for file:// URLs (our custom FileSystem handles I/O).
        // This lets Ultralight resolve relative assets against the template dir.
        // For inline HTML, fall back to LoadHTML.
        if (use_load_url_) {
            CASPAR_LOG(info) << print() << L" LoadURL: " << url_;
            view_->LoadURL(ul::String(u8(url_).c_str()));
        } else if (!html_content_.empty()) {
            view_->LoadHTML(ul::String(html_content_.c_str()));
        }

        // Pump until loaded — up to 10 seconds.
        for (int i = 0; i < 1000 && !load_listener_.is_loaded() && !load_listener_.is_failed(); ++i) {
            renderer_->Update();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (load_listener_.is_failed())
            CASPAR_LOG(error) << print() << L" Failed to load: " << url_;

        // Inject __caspar_frame = 0 and __caspar_playing = false so the
        // template can read these before play() and render a static state.
        view_->EvaluateScript("window.__caspar_frame=0;window.__caspar_playing=false;window.__caspar_producer='ultralight';");

        // Render frame 0 — the frozen "ready" state shown before play().
        renderer_->Update();
        renderer_->Render();

        // Cache the frozen frame so receive_impl can return it without
        // advancing the renderer while paused.
        frozen_frame_ = render_current_frame();

        CASPAR_LOG(info) << print() << L" Ultralight initialized. Template loaded and frozen frame cached.";

        // If play() was queued before init (e.g. CG ADD with play_on_load=1),
        // transition to playing state now.
        auto it = std::find(pending_js_calls_.begin(), pending_js_calls_.end(), L"play()");
        if (it != pending_js_calls_.end()) {
            pending_js_calls_.erase(it);
            start_playing();
        }
    }

    // Transition to playing state. Sets the flag and queues the play() JS call.
    // The actual JS execution happens on the next receive_impl() call, which
    // runs on the channel tick thread (required for Ultralight thread safety).
    void start_playing()
    {
        playing_.store(true);
        channel_frame_ = -1; // first ++channel_frame_ in receive_impl makes it 0
        play_pending_.store(true);

        CASPAR_LOG(info) << print() << L" Playing (will execute on next tick).";
    }

    ~ultralight_producer() override
    {
        view_ = nullptr;
        renderer_ = nullptr;
    }

    // ── Internal: read the current Ultralight surface into a draw_frame ──

    core::draw_frame render_current_frame()
    {
        auto* surface = static_cast<ul::BitmapSurface*>(view_->surface());
        if (!surface)
            return core::draw_frame::empty();

        auto bitmap = surface->bitmap();
        if (!bitmap)
            return core::draw_frame::empty();

        auto pixels = bitmap->LockPixels();
        if (!pixels)
            return core::draw_frame::empty();

        auto frame = frame_factory_->create_frame(this, pixel_desc_);

        const uint32_t src_stride = bitmap->row_bytes();
        const uint32_t dst_stride = static_cast<uint32_t>(format_desc_.square_width) * 4;
        const uint32_t height     = static_cast<uint32_t>(format_desc_.square_height);
        auto*          dst        = reinterpret_cast<uint8_t*>(frame.image_data(0).begin());
        auto*          src        = reinterpret_cast<const uint8_t*>(pixels);

        if (src_stride == dst_stride) {
            std::memcpy(dst, src, dst_stride * height);
        } else {
            for (uint32_t y = 0; y < height; ++y)
                std::memcpy(dst + y * dst_stride, src + y * src_stride, dst_stride);
        }

        bitmap->UnlockPixels();
        return core::draw_frame(std::move(frame));
    }

    // ── frame_producer interface ──

    std::wstring name() const override { return L"ultralight"; }

    core::draw_frame receive_impl(const core::video_field field, int nb_samples) override
    {
        try {
            ensure_initialized();

            // Before play(): return the frozen frame. The template is loaded
            // and rendered but animations haven't started. This is the "ready"
            // state shown during LOAD / before CG PLAY.
            if (!playing_.load()) {
                std::lock_guard<std::mutex> lock(producer_mutex_);
                return frozen_frame_;
            }

            // Drain queued JS calls (from AMCP thread) on the tick thread.
            {
                std::vector<std::wstring> js_batch;
                {
                    std::lock_guard<std::mutex> lock(producer_mutex_);
                    js_batch.swap(pending_js_calls_);
                }
                for (const auto& js : js_batch) {
                    auto js_utf8 = u8(js);
                    view_->EvaluateScript(js_utf8.c_str());
                }
            }

            // On first tick after play(): signal playback start to template.
            // No extra Update+Render here — falls through to the normal tick below
            // so frame 0 is rendered exactly once.
            if (play_pending_.exchange(false)) {
                view_->EvaluateScript("window.__caspar_playing=true;");
                view_->EvaluateScript("if(typeof play==='function')play();");
            }

            caspar::timer frame_timer;

            // Advance the deterministic frame counter and compute virtual time.
            // These are the ONLY timing sources for the template:
            //   window.__caspar_frame   — integer frame count since play()
            //   window.__caspar_time    — virtual time in seconds (frame / fps)
            //   window.__caspar_playing — true after play(), false after stop()
            //   window.__caspar_tick()  — called exactly once per channel tick
            //
            // Templates MUST use these instead of requestAnimationFrame counting
            // or wall-clock time for deterministic offline rendering.
            ++channel_frame_;
            auto js = "window.__caspar_frame=" + std::to_string(channel_frame_)
                     + ";window.__caspar_time=" + std::to_string(
                           static_cast<double>(channel_frame_) / format_desc_.fps)
                     + ";if(typeof window.__caspar_tick==='function')window.__caspar_tick("
                     + std::to_string(channel_frame_) + ");";
            view_->EvaluateScript(js.c_str());

            // Advance the renderer exactly one tick. Ultralight processes JS
            // (rAF callbacks, timers) in Update() and paints in Render().
            // One Update+Render per receive_impl = one frame per channel tick.
            renderer_->Update();
            renderer_->Render();

            auto result = render_current_frame();

            graph_->set_value("frame-time", frame_timer.elapsed() * format_desc_.fps * 0.5);

            return result;
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << print() << L" receive_impl exception: " << e.what();
            return core::draw_frame::empty();
        } catch (...) {
            CASPAR_LOG(error) << print() << L" receive_impl unknown exception.";
            return core::draw_frame::empty();
        }
    }

    core::draw_frame first_frame(const core::video_field field) override
    {
        // first_frame triggers initialization and caches the frozen frame.
        ensure_initialized();
        return frozen_frame_;
    }

    bool is_ready() override
    {
        return initialized_;
    }

    // call() is invoked from the AMCP protocol thread. Ultralight requires
    // all API calls on the channel tick thread. We queue JS and set flags;
    // receive_impl() drains the queue on the correct thread.
    std::future<std::wstring> call(const std::vector<std::wstring>& params) override
    {
        if (params.empty())
            return make_ready_future(std::wstring(L""));

        auto javascript = params.at(0);

        if (javascript == L"play()") {
            if (initialized_.load())
                start_playing();
            else {
                std::lock_guard<std::mutex> lock(producer_mutex_);
                pending_js_calls_.push_back(javascript);
            }
            return make_ready_future(std::wstring(L""));
        }

        if (javascript == L"stop()") {
            playing_.store(false);
            // Queue stop JS for tick thread — never call view_ from here.
            std::lock_guard<std::mutex> lock(producer_mutex_);
            pending_js_calls_.push_back(
                L"window.__caspar_playing=false;if(typeof stop==='function')stop();");
            return make_ready_future(std::wstring(L""));
        }

        // All other JS calls — queue for tick thread.
        {
            std::lock_guard<std::mutex> lock(producer_mutex_);
            pending_js_calls_.push_back(javascript);
        }

        return make_ready_future(std::wstring(L""));
    }

    std::wstring print() const override
    {
        return L"ultralight[" + url_ + L"]";
    }

    core::monitor::state state() const override
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return state_;
    }
};

// ---------------------------------------------------------------------------
// Factory functions
// ---------------------------------------------------------------------------

spl::shared_ptr<core::frame_producer>
create_ul_producer(const core::frame_producer_dependencies& dependencies,
                   const std::vector<std::wstring>&         params)
{
    if (params.empty())
        return core::frame_producer::empty();

    auto url      = params.at(0);
    auto url_utf8 = u8(url);

    // Check for explicit [ULTRALIGHT] prefix or .html extension
    if (boost::iequals(url, L"[ULTRALIGHT]")) {
        if (params.size() < 2)
            return core::frame_producer::empty();
        url = params.at(1);
    }

    // Try to find as a template file
    const auto filename = env::template_folder() + url + L".html";
    if (!boost::filesystem::exists(u8(filename))) {
        // Try as direct URL
        if (!boost::algorithm::istarts_with(url, L"http:") &&
            !boost::algorithm::istarts_with(url, L"https:") &&
            !boost::algorithm::istarts_with(url, L"file:"))
            return core::frame_producer::empty();
    }

    const auto resolved_url = boost::filesystem::exists(u8(filename))
                                  ? L"file://" + filename
                                  : url;

    return spl::make_shared<ultralight_producer>(
        dependencies.frame_factory,
        dependencies.format_desc,
        resolved_url);
}

spl::shared_ptr<core::frame_producer>
create_ul_cg_producer(const core::frame_producer_dependencies& dependencies,
                      const std::vector<std::wstring>&         params)
{
    if (params.empty())
        return core::frame_producer::empty();

    auto template_path = env::template_folder() + params.at(0) + L".html";

    if (!boost::filesystem::exists(u8(template_path)))
        return core::frame_producer::empty();

    auto url = L"file://" + template_path;

    return spl::make_shared<ultralight_producer>(
        dependencies.frame_factory,
        dependencies.format_desc,
        url);
}

}} // namespace caspar::ultralight
