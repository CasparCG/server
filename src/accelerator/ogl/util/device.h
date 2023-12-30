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

#pragma once

#include <accelerator/accelerator.h>
#include <common/array.h>
#include <common/bit_depth.h>

#include <functional>
#include <future>

#ifdef WIN32
#include <GL/glew.h>
#endif

namespace caspar { namespace accelerator { namespace ogl {

// This must match description_layout in shader_from_rgba.comp
struct convert_from_texture_description
{
    bool     is_16_bit;
    uint32_t width;
    uint32_t height;
    uint32_t words_per_line;
};

class device final
    : public std::enable_shared_from_this<device>
    , public accelerator_device
{
  public:
    device();
    ~device();

    device(const device&) = delete;

    device& operator=(const device&) = delete;

    std::shared_ptr<class texture> create_texture(int width, int height, int stride, common::bit_depth depth);
    array<uint8_t>                 create_array(int size);

    std::future<std::shared_ptr<class texture>>
    copy_async(const array<const uint8_t>& source, int width, int height, int stride, common::bit_depth depth);
    std::future<array<const uint8_t>> copy_async(const std::shared_ptr<class texture>& source, bool as_rgba8);

    std::future<void> convert_from_texture(const std::shared_ptr<texture>&         texture,
                                           const array<const uint8_t>&             source,
                                           const convert_from_texture_description& description,
                                           int                                     x_count,
                                           int                                     y_count);

    template <typename Func>
    auto dispatch_async(Func&& func)
    {
        using result_type = decltype(func());
        using task_type   = std::packaged_task<result_type()>;

        auto task   = std::make_shared<task_type>(std::forward<Func>(func));
        auto future = task->get_future();
        dispatch([=] { (*task)(); });
        return future;
    }

    template <typename Func>
    auto dispatch_sync(Func&& func)
    {
        return dispatch_async(std::forward<Func>(func)).get();
    }

    std::wstring version() const;

    boost::property_tree::wptree info() const;
    std::future<void>            gc();

  private:
    void dispatch(std::function<void()> func);
    struct impl;
    std::shared_ptr<impl> impl_;
};

}}} // namespace caspar::accelerator::ogl
