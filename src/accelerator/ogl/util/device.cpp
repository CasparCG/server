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
#include "device.h"

#include "buffer.h"
#include "shader.h"
#include "texture.h"

#include <common/array.h>
#include <common/assert.h>
#include <common/except.h>
#include <common/future.h>
#include <common/gl/gl_check.h>
#include <common/os/thread.h>

#include <GL/glew.h>

#include <SFML/Window/Context.hpp>

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/spawn.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>

#include <array>
#include <future>
#include <thread>

namespace caspar { namespace accelerator { namespace ogl {

using namespace boost::asio;

struct device::impl : public std::enable_shared_from_this<impl>
{
    using texture_queue_t = tbb::concurrent_bounded_queue<std::shared_ptr<texture>>;
    using buffer_queue_t  = tbb::concurrent_bounded_queue<std::shared_ptr<buffer>>;

    sf::Context device_;

    std::array<tbb::concurrent_unordered_map<size_t, texture_queue_t>, 4> device_pools_;
    std::array<tbb::concurrent_unordered_map<size_t, buffer_queue_t>, 2>  host_pools_;

    using sync_queue_t = tbb::concurrent_bounded_queue<std::shared_ptr<buffer>>;

    sync_queue_t sync_queue_;

    GLuint fbo_;

    std::wstring version_;

    io_context                          service_;
    decltype(make_work_guard(service_)) work_;
    std::thread                         thread_;

    impl()
        : device_(sf::ContextSettings(0, 0, 0, 4, 5, sf::ContextSettings::Attribute::Core), 1, 1)
        , work_(make_work_guard(service_))
    {
        CASPAR_LOG(info) << L"Initializing OpenGL Device.";

        device_.setActive(true);

        if (glewInit() != GLEW_OK) {
            CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to initialize GLEW."));
        }

        version_ = u16(reinterpret_cast<const char*>(GL2(glGetString(GL_VERSION)))) + L" " +
                   u16(reinterpret_cast<const char*>(GL2(glGetString(GL_VENDOR))));

        CASPAR_LOG(info) << L"Initialized OpenGL " << version();

        if (!GLEW_VERSION_4_5 && !glewIsSupported("GL_ARB_sync GL_ARB_shader_objects GL_ARB_multitexture "
                                                  "GL_ARB_direct_state_access GL_ARB_texture_barrier")) {
            CASPAR_THROW_EXCEPTION(not_supported()
                                   << msg_info("Your graphics card does not meet the minimum hardware requirements "
                                               "since it does not support OpenGL 4.5 or higher."));
        }

        GL(glCreateFramebuffers(1, &fbo_));
        GL(glBindFramebuffer(GL_FRAMEBUFFER, fbo_));

        device_.setActive(false);

        thread_ = std::thread([&] {
            device_.setActive(true);
            set_thread_name(L"OpenGL Device");
            service_.run();
            device_.setActive(false);
        });
    }

    ~impl()
    {
        work_.reset();
        thread_.join();

        device_.setActive(true);

        for (auto& pool : host_pools_)
            pool.clear();

        for (auto& pool : device_pools_)
            pool.clear();

        sync_queue_.clear();

        GL(glDeleteFramebuffers(1, &fbo_));
    }

    template <typename Func>
    auto spawn_async(Func&& func)
    {
        using result_type = decltype(func(std::declval<yield_context>()));
        using task_type   = std::packaged_task<result_type(yield_context)>;

        auto task   = task_type(std::forward<Func>(func));
        auto future = task.get_future();
        boost::asio::spawn(service_, std::move(task));
        return future;
    }

    template <typename Func>
    auto dispatch_async(Func&& func)
    {
        using result_type = decltype(func());
        using task_type   = std::packaged_task<result_type()>;

        auto task   = task_type(std::forward<Func>(func));
        auto future = task.get_future();
        boost::asio::dispatch(service_, std::move(task));
        return future;
    }

    template <typename Func>
    auto dispatch_sync(Func&& func) -> decltype(func())
    {
        return dispatch_async(std::forward<Func>(func)).get();
    }

    std::wstring version() { return version_; }

    std::shared_ptr<texture> create_texture(int width, int height, int stride, bool clear)
    {
        CASPAR_VERIFY(stride > 0 && stride < 5);
        CASPAR_VERIFY(width > 0 && height > 0);

        // TODO (perf) Shared pool.
        auto pool = &device_pools_[stride - 1][width << 16 & 0xFFFF0000 | height & 0x0000FFFF];

        std::shared_ptr<texture> tex;
        if (!pool->try_pop(tex)) {
            tex = std::make_shared<texture>(width, height, stride);
        }

        if (clear) {
            tex->clear();
        }

        auto ptr = tex.get();
        return std::shared_ptr<texture>(
            ptr, [tex = std::move(tex), pool, self = shared_from_this()](texture*) mutable { pool->push(tex); });
    }

    std::shared_ptr<buffer> create_buffer(int size, bool write)
    {
        CASPAR_VERIFY(size > 0);

        // TODO (perf) Shared pool.
        auto pool = &host_pools_[static_cast<int>(write ? 1 : 0)][size];

        std::shared_ptr<buffer> buf;
        if (!pool->try_pop(buf)) {
            // TODO (perf) Avoid blocking in create_array.
            dispatch_sync([&] { buf = std::make_shared<buffer>(size, write); });
        }

        auto ptr = buf.get();
        return std::shared_ptr<buffer>(ptr, [buf = std::move(buf), self = shared_from_this()](buffer*) mutable {
            self->sync_queue_.emplace(std::move(buf));
        });
    }

    array<uint8_t> create_array(int size)
    {
        auto buf = create_buffer(size, true);
        auto ptr = reinterpret_cast<uint8_t*>(buf->data());
        return array<uint8_t>(ptr, buf->size(), buf);
    }

    std::future<std::shared_ptr<texture>>
    copy_async(const array<const uint8_t>& source, int width, int height, int stride)
    {
        return dispatch_async([=] {
            std::shared_ptr<buffer> buf;

            auto tmp = source.storage<std::shared_ptr<buffer>>();
            if (tmp) {
                buf = *tmp;
            } else {
                buf = create_buffer(static_cast<int>(source.size()), true);
                // TODO (perf) Copy inside a TBB worker.
                std::memcpy(buf->data(), source.data(), source.size());
            }

            auto tex = create_texture(width, height, stride, false);
            tex->copy_from(*buf);
            // TODO (perf) save tex on source
            return tex;
        });
    }

    std::future<array<const uint8_t>> copy_async(const std::shared_ptr<texture>& source)
    {
        return spawn_async([=](yield_context yield) {
            auto buf = create_buffer(source->size(), false);
            source->copy_to(*buf);

            sync_queue_.push(nullptr);

            auto fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

            GL(glFlush());

            deadline_timer timer(service_);
            for (auto n = 0; true; ++n) {
                // TODO (perf) Smarter non-polling solution?
                timer.expires_from_now(boost::posix_time::milliseconds(2));
                timer.async_wait(yield);

                auto wait = glClientWaitSync(fence, 0, 1);
                if (wait == GL_ALREADY_SIGNALED || wait == GL_CONDITION_SATISFIED) {
                    break;
                }
            }

            glDeleteSync(fence);

            {
                std::shared_ptr<buffer> buf2;
                while (sync_queue_.try_pop(buf2) && buf2) {
                    auto pool = &host_pools_[static_cast<int>(buf2->write() ? 1 : 0)][buf2->size()];
                    pool->push(std::move(buf2));
                }
            }

            auto ptr  = reinterpret_cast<uint8_t*>(buf->data());
            auto size = buf->size();
            return array<const uint8_t>(ptr, size, std::move(buf));
        });
    }
};

device::device()
    : impl_(new impl())
{
}
device::~device() {}
std::shared_ptr<texture> device::create_texture(int width, int height, int stride)
{
    return impl_->create_texture(width, height, stride, true);
}
array<uint8_t> device::create_array(int size) { return impl_->create_array(size); }
std::future<std::shared_ptr<texture>>
device::copy_async(const array<const uint8_t>& source, int width, int height, int stride)
{
    return impl_->copy_async(source, width, height, stride);
}
std::future<array<const uint8_t>> device::copy_async(const std::shared_ptr<texture>& source)
{
    return impl_->copy_async(source);
}
void         device::dispatch(std::function<void()> func) { boost::asio::dispatch(impl_->service_, std::move(func)); }
std::wstring device::version() const { return impl_->version(); }
}}} // namespace caspar::accelerator::ogl
