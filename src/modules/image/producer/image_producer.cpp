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

#include "image_producer.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#if defined(_MSC_VER)
#include <windows.h>
#endif
#include <FreeImage.h>

#include "../util/image_loader.h"

#include <core/video_format.h>

#include <core/frame/draw_frame.h>
#include <core/frame/frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/geometry.h>
#include <core/frame/pixel_format.h>
#include <core/monitor/monitor.h>
#include <core/producer/frame_producer.h>

#include <common/array.h>
#include <common/env.h>
#include <common/filesystem.h>
#include <common/log.h>
#include <common/os/filesystem.h>
#include <common/param.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <algorithm>
#include <set>
#include <utility>

namespace caspar { namespace image {

struct image_producer : public core::frame_producer
{
    core::monitor::state                       state_;
    const std::wstring                         description_;
    const spl::shared_ptr<core::frame_factory> frame_factory_;
    const uint32_t                             length_ = 0;
    core::draw_frame                           frame_;

    image_producer(const spl::shared_ptr<core::frame_factory>& frame_factory,
                   std::wstring                                description,
                   uint32_t                                    length,
                   core::frame_geometry::scale_mode            scale_mode)
        : description_(std::move(description))
        , frame_factory_(frame_factory)
        , length_(length)
    {
        load(load_image(description_, true), scale_mode);

        CASPAR_LOG(info) << print() << L" Initialized";
    }

    image_producer(const spl::shared_ptr<core::frame_factory>& frame_factory,
                   const void*                                 png_data,
                   size_t                                      size,
                   uint32_t                                    length,
                   core::frame_geometry::scale_mode            scale_mode)
        : description_(L"png from memory")
        , frame_factory_(frame_factory)
        , length_(length)
    {
        load(load_png_from_memory(png_data, size, true), scale_mode);

        CASPAR_LOG(info) << print() << L" Initialized";
    }

    void load(const loaded_image& image, core::frame_geometry::scale_mode scale_mode)
    {
        core::pixel_format_desc desc(image.format);
        desc.is_straight_alpha = image.is_straight;
        desc.planes.emplace_back(
            FreeImage_GetWidth(image.bitmap.get()), FreeImage_GetHeight(image.bitmap.get()), image.stride, image.depth);

        auto frame       = frame_factory_->create_frame(this, desc);
        frame.geometry() = core::frame_geometry::get_default_vflip(scale_mode);

        std::copy_n(FreeImage_GetBits(image.bitmap.get()), frame.image_data(0).size(), frame.image_data(0).begin());
        frame_ = core::draw_frame(std::move(frame));
    }

    // frame_producer

    core::draw_frame last_frame(const core::video_field field) override { return frame_; }

    core::draw_frame first_frame(const core::video_field field) override { return frame_; }

    bool is_ready() override { return true; }

    core::draw_frame receive_impl(const core::video_field field, int nb_samples) override
    {
        state_["file/path"] = description_;
        return frame_;
    }

    uint32_t nb_frames() const override { return length_; }

    std::wstring print() const override { return L"image_producer[" + description_ + L"]"; }

    std::wstring name() const override { return L"image"; }

    core::monitor::state state() const override { return state_; }
};

spl::shared_ptr<core::frame_producer> create_producer(const core::frame_producer_dependencies& dependencies,
                                                      const std::vector<std::wstring>&         params)
{
    if (boost::contains(params.at(0), L"://")) {
        return core::frame_producer::empty();
    }

    auto length     = get_param(L"LENGTH", params, std::numeric_limits<uint32_t>::max());
    auto scale_mode = core::scale_mode_from_string(get_param(L"SCALE_MODE", params, L"STRETCH"));

    auto filename = find_file_within_dir_or_absolute(env::media_folder(), params.at(0), is_valid_file);
    if (!filename) {
        return core::frame_producer::empty();
    }

    return spl::make_shared<image_producer>(dependencies.frame_factory, filename->wstring(), length, scale_mode);
}

}} // namespace caspar::image
