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
 */

#include "window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include <common/except.h>
#include <common/log.h>

#include <mutex>

namespace caspar { namespace screen { namespace vulkan {

namespace {
std::mutex g_glfw_mutex;
int        g_glfw_refcount = 0;
} // namespace

struct screen_window::impl
{
    GLFWwindow* window_ = nullptr;
    int         width_  = 0;
    int         height_ = 0;

    explicit impl(const window_config& config)
    {
        {
            std::lock_guard<std::mutex> lock(g_glfw_mutex);
            if (g_glfw_refcount == 0) {
                if (!glfwInit())
                    CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to initialize GLFW"));
            }
            ++g_glfw_refcount;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, config.windowed ? GLFW_TRUE : GLFW_FALSE);
        if (config.borderless)
            glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

        width_  = config.width;
        height_ = config.height;

        GLFWmonitor* monitor = nullptr;
        if (!config.windowed) {
            monitor                = glfwGetPrimaryMonitor();
            int           count    = 0;
            GLFWmonitor** monitors = glfwGetMonitors(&count);
            if (config.screen_index > 0 && config.screen_index < count)
                monitor = monitors[config.screen_index];
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            width_                  = mode->width;
            height_                 = mode->height;
        }

        window_ = glfwCreateWindow(width_, height_, config.title.c_str(), monitor, nullptr);
        if (!window_)
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to create GLFW window"));

        if (config.windowed)
            glfwSetWindowPos(window_, config.x, config.y);
        if (!config.interactive)
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        if (config.always_on_top)
            glfwSetWindowAttrib(window_, GLFW_FLOATING, GLFW_TRUE);

        glfwShowWindow(window_);
    }

    ~impl()
    {
        if (window_)
            glfwDestroyWindow(window_);
        std::lock_guard<std::mutex> lock(g_glfw_mutex);
        if (--g_glfw_refcount == 0)
            glfwTerminate();
    }

    bool poll()
    {
        if (!window_)
            return true;
        glfwPollEvents();
        return glfwWindowShouldClose(window_);
    }

    void framebuffer_size(int& w, int& h)
    {
        w = 0;
        h = 0;
        if (window_)
            glfwGetFramebufferSize(window_, &w, &h);
    }

    void wait_for_events()
    {
        glfwWaitEvents();
    }
};

screen_window::screen_window(const window_config& config)
    : impl_(std::make_unique<impl>(config))
{
}
screen_window::~screen_window() {}
int            screen_window::width() const { return impl_->width_; }
int            screen_window::height() const { return impl_->height_; }
bool           screen_window::poll() { return impl_->poll(); }
void           screen_window::framebuffer_size(int& w, int& h) { impl_->framebuffer_size(w, h); }
void           screen_window::wait_for_events() { impl_->wait_for_events(); }
vk::SurfaceKHR screen_window::create_surface(vk::Instance vk_instance)
{
    VkSurfaceKHR raw = VK_NULL_HANDLE;
    VkResult result = glfwCreateWindowSurface(static_cast<VkInstance>(vk_instance), impl_->window_, nullptr, &raw);
    if (result != VK_SUCCESS)
        CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to create Vulkan window surface"));
    return vk::SurfaceKHR(raw);
}

}}} // namespace caspar::screen::vulkan
