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

// TODO: Smart GC

#include "../../StdAfx.h"

#include "device.h"

#include "buffer.h"
#include "texture.h"
#include "shader.h"

#include <common/assert.h>
#include <common/future.h>
#include <common/except.h>
#include <common/array.h>
#include <common/gl/gl_check.h>

#include <GL/glew.h>

#include <SFML/Window/Context.hpp>

#include <boost/asio/deadline_timer.hpp>

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_queue.h>

#include <array>
#include <future>
#include <thread>

namespace caspar { namespace accelerator { namespace ogl {

using namespace boost::asio;

struct device::impl : public std::enable_shared_from_this<impl>
{
    typedef tbb::concurrent_bounded_queue<std::shared_ptr<texture>> texture_queue_t;
    typedef tbb::concurrent_bounded_queue<std::shared_ptr<buffer>>  buffer_queue_t;

	std::unique_ptr<sf::Context> device_;

	std::array<tbb::concurrent_unordered_map<size_t, texture_queue_t>, 4>  device_pools_;
    std::array<tbb::concurrent_unordered_map<size_t, buffer_queue_t>, 2>   host_pools_;

    typedef std::pair<GLsync, std::shared_ptr<buffer>> sync_t;
    typedef tbb::concurrent_bounded_queue<sync_t>      sync_queue_t;

    sync_queue_t sync_queue_;
    GLsync       sync_fence_ = 0;

	GLuint fbo_;

	io_service& service_;
    io_service::work work_;
    std::thread thread_;

	impl(boost::asio::io_service& service)
		: service_(service)
        , work_(service)
        , thread_([&] { service_.run(); })
	{
		CASPAR_LOG(info) << L"Initializing OpenGL Device.";

        dispatch_sync([=]
		{
			device_.reset(new sf::Context());
			device_->setActive(true);

            if (glewInit() != GLEW_OK) {
                CASPAR_THROW_EXCEPTION(gl::ogl_exception() << msg_info("Failed to initialize GLEW."));
            }

            if (!GLEW_VERSION_4_0) {
                CASPAR_THROW_EXCEPTION(not_supported() << msg_info("Your graphics card does not meet the minimum hardware requirements since it does not support OpenGL 4.0 or higher."));
            }

            glCreateFramebuffers(1, &fbo_);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
		});

		CASPAR_LOG(info) << L"Successfully initialized OpenGL " << version();
	}

	~impl()
	{
        dispatch_sync([=]
		{
			for (auto& pool : host_pools_)
				pool.clear();

			for (auto& pool : device_pools_)
				pool.clear();

            if (sync_fence_) {
                glDeleteSync(sync_fence_);
            }

            sync_t sync;
            while (sync_queue_.try_pop(sync)) {
                if (sync.first) {
                    glDeleteSync(sync.first);
                }
            }

			glDeleteFramebuffers(1, &fbo_);

			device_.reset();
		});
        service_.stop();
        thread_.join();
	}

    template <typename F>
    void post(F&& func)
    {
        auto func_ptr = std::make_shared<std::function<void()>>(std::forward<F>(func));
        service_.post(std::bind(&std::function<void()>::operator(), std::move(func_ptr)));
    }

    template <typename F>
    void dispatch(F&& func)
    {
        auto func_ptr = std::make_shared<std::function<void()>>(std::forward<F>(func));
        service_.dispatch(std::bind(&std::function<void()>::operator(), std::move(func_ptr)));
    }

    template<typename Func>
    auto dispatch_async(Func&& func) -> std::future<decltype(func())>
    {
        typedef typename std::remove_reference<Func>::type	function_type;
        typedef decltype(func())							result_type;
        typedef std::packaged_task<result_type()>			task_type;

        auto task = std::make_shared<task_type>(std::forward<Func>(func));
        auto future = task->get_future();
        service_.dispatch(std::bind(&task_type::operator(), std::move(task)));
        return future;
    }

    template<typename Func>
    auto dispatch_sync(Func&& func) -> decltype(func())
    {
        return dispatch_async(std::forward<Func>(func)).get();
    }

	std::wstring version()
	{
		try
		{
			return dispatch_sync([]
			{
				return u16(reinterpret_cast<const char*>(GL2(glGetString(GL_VERSION)))) + L" " + u16(reinterpret_cast<const char*>(GL2(glGetString(GL_VENDOR))));
			});
		}
		catch(...)
		{
			return L"Not found";;
		}
	}

	spl::shared_ptr<texture> create_texture(int width, int height, int stride, bool clear)
	{
		CASPAR_VERIFY(stride > 0 && stride < 5);
		CASPAR_VERIFY(width > 0 && height > 0);

        // TODO (perf) Shared pool.
        auto pool = &device_pools_[stride - 1][((width << 16) & 0xFFFF0000) | (height & 0x0000FFFF)];

        std::shared_ptr<texture> tex;
        if (!pool->try_pop(tex)) {
            dispatch_sync([&]
            {
                tex = spl::make_shared<texture>(width, height, stride);
            });
        }
        if (clear) {
            dispatch_sync([&]
            {
                tex->clear();
            });
        }

        return spl::shared_ptr<texture>(tex.get(), [tex, pool](texture*) mutable
        {
            pool->push(tex);
        });
	}

    std::shared_ptr<buffer> create_buffer(int size, bool write)
	{
		CASPAR_VERIFY(size > 0);

        // TODO (perf) Shared pool.
		auto pool = &host_pools_[static_cast<int>(write ? 1 : 0)][size];

        std::shared_ptr<buffer> buf;
        if (!pool->try_pop(buf)) {
            dispatch_sync([&]
            {
                buf = std::make_shared<buffer>(size, write);
            });
        }

        return std::shared_ptr<buffer>(buf.get(), [=](buffer*) mutable
        {
            sync_queue_.emplace(static_cast<GLsync>(0), std::move(buf));
        });
	}

	array<uint8_t> create_array(int size)
	{
		auto buf = create_buffer(size, true);
        auto ptr = reinterpret_cast<uint8_t*>(buf->data());
		return array<uint8_t>(ptr, buf->size(), false, buf);
	}

	template<typename T>
	std::shared_ptr<buffer> copy_to_buf(const T& source)
	{
		std::shared_ptr<buffer> buf;

        auto tmp = source.template storage<std::shared_ptr<buffer>>();
        if (tmp) {
            buf = *tmp;
        } else {
            buf = create_buffer(static_cast<int>(source.size()), true);
            std::memcpy(buf->data(), source.data(), source.size());
        }

		return buf;
	}

	std::future<std::shared_ptr<texture>> copy_async(const array<const uint8_t>& source, int width, int height, int stride)
	{
		auto buf = copy_to_buf(source);
		return dispatch_async([=]() -> std::shared_ptr<texture>
		{
			auto texture = create_texture(width, height, stride, false);
			texture->copy_from(*buf);
			return texture;
		});
	}

	std::future<array<const uint8_t>> copy_async(const spl::shared_ptr<texture>& source)
	{
        return flatten(dispatch_async([=]
        {
            auto buf = create_buffer(source->size(), false);
            source->copy_to(*buf);
            buf->lock();

            return std::async(std::launch::deferred, [=, buf = std::move(buf)]()
            {
                for (auto n = 0; true; n = std::min(10, n + 1)) {
                    if (dispatch_sync([&] { return buf->try_wait(); })) {
                        break;
                    }

                    boost::asio::deadline_timer timer(service_, boost::posix_time::milliseconds(n));
                    // TODO (fix) Does timer.wait yield?
                    timer.wait();
                }

                auto ptr = reinterpret_cast<uint8_t*>(buf->data());
                auto size = buf->size();
                return array<const uint8_t>(ptr, size, true, std::move(buf));
            });
        }));
	}

    void flush()
    {
        post([&]
        {
            sync_queue_.push(sync_t(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0), nullptr));

            if (sync_fence_) {
                auto wait = glClientWaitSync(sync_fence_, GL_SYNC_FLUSH_COMMANDS_BIT, 1);
                if (wait != GL_ALREADY_SIGNALED && wait != GL_CONDITION_SATISFIED) {
                    return;
                }
                glDeleteSync(sync_fence_);
                sync_fence_ = 0;
            }

            sync_t sync;
            while (sync_queue_.try_pop(sync)) {
                if (sync.first) {
                    sync_fence_ = sync.first;
                    break;
                } else {
                    auto buf = sync.second;
                    auto pool = &host_pools_[static_cast<int>(buf->write() ? 1 : 0)][buf->size()];
                    pool->push(std::move(buf));
                }

            }
        });
    }
};

device::device()
	: impl_(new impl(service_)){}
device::~device() {}
spl::shared_ptr<texture>					device::create_texture(int width, int height, int stride) { return impl_->create_texture(width, height, stride, true); }
array<uint8_t>							    device::create_array(int size) { return impl_->create_array(size); }
std::future<std::shared_ptr<texture>>		device::copy_async(const array<const uint8_t>& source, int width, int height, int stride) { return impl_->copy_async(source, width, height, stride); }
std::future<array<const uint8_t>>		    device::copy_async(const spl::shared_ptr<texture>& source) { return impl_->copy_async(source); }
std::wstring								device::version() const { return impl_->version(); }
void                                        device::flush() { return impl_->flush(); }
}}}
