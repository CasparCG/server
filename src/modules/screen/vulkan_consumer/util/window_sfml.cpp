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

// Must come before SFML headers to avoid Windows.h macro pollution
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <SFML/Window.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_win32.h>

#include <common/except.h>
#include <common/log.h>

namespace caspar { namespace screen { namespace vulkan {

struct screen_window::impl
{
    sf::Window window_;
    int        width_  = 0;
    int        height_ = 0;
    bool       close_  = false;

    explicit impl(const window_config& config)
    {
        // The consumer has already resolved per-monitor coordinates via
        // EnumDisplaySettings, so config.x/y/width/height are correct for the
        // target display. For non-windowed mode we use a borderless window at
        // those coordinates — sf::Style::Fullscreen is restricted to the
        // primary monitor in SFML 2.x.
        sf::Uint32 style = (!config.windowed || config.borderless) ? sf::Style::None : sf::Style::Default;

        window_.create(sf::VideoMode(config.width, config.height),
                       sf::String::fromUtf8(config.title.begin(), config.title.end()),
                       style);

        if (!window_.isOpen())
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to create SFML window"));

        window_.setPosition(sf::Vector2i(config.x, config.y));

        if (!config.interactive)
            window_.setMouseCursorVisible(false);

        if (config.always_on_top) {
            SetWindowPos(static_cast<HWND>(window_.getSystemHandle()),
                         HWND_TOPMOST,
                         0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE);
        }

        auto size = window_.getSize();
        width_  = static_cast<int>(size.x);
        height_ = static_cast<int>(size.y);
    }

    bool poll()
    {
        sf::Event event;
        while (window_.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                close_ = true;
        }
        return close_;
    }

    void framebuffer_size(int& w, int& h)
    {
        auto size = window_.getSize();
        w = static_cast<int>(size.x);
        h = static_cast<int>(size.y);
    }

    void wait_for_events()
    {
        sf::Event event;
        if (window_.waitEvent(event) && event.type == sf::Event::Closed)
            close_ = true;
    }

    vk::SurfaceKHR create_surface(vk::Instance vk_instance)
    {
        auto instance = static_cast<VkInstance>(vk_instance);

        auto fn = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(
            VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR"));
        if (!fn)
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("vkCreateWin32SurfaceKHR not available"));

        VkWin32SurfaceCreateInfoKHR info{};
        info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        info.hwnd      = static_cast<HWND>(window_.getSystemHandle());
        info.hinstance = GetModuleHandle(nullptr);

        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if (fn(instance, &info, nullptr, &surface) != VK_SUCCESS)
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("Failed to create Win32 Vulkan surface"));

        return vk::SurfaceKHR(surface);
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
vk::SurfaceKHR screen_window::create_surface(vk::Instance vk_instance) { return impl_->create_surface(vk_instance); }

}}} // namespace caspar::screen::vulkan
