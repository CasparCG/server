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

#include "../util/image_loader.h"

#include <common/base64.h>
#include <common/env.h>
#include <common/filesystem.h>
#include <common/param.h>

#include <boost/algorithm/string.hpp>

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
        auto av_frame = load_image(description_);
        if (!is_frame_compatible_with_mixer(av_frame)) av_frame = convert_to_bgra32(av_frame);

        auto frame = ffmpeg::make_frame(this, *frame_factory, av_frame, nullptr, core::color_space::bt709, scale_mode, true);
        frame_ = core::draw_frame(std::move(frame));

        state_["file/path"] = description_;

        CASPAR_LOG(info) << print() << L" Initialized";
    }

    image_producer(const spl::shared_ptr<core::frame_factory>& frame_factory,
                   std::vector<unsigned char>                  image_data,
                   uint32_t                                    length,
                   core::frame_geometry::scale_mode            scale_mode)
        : description_(L"base64 image from memory")
        , frame_factory_(frame_factory)
        , length_(length)
    {
        auto av_frame = load_from_memory2(std::move(image_data));
        if (!is_frame_compatible_with_mixer(av_frame)) av_frame = convert_to_bgra32(av_frame);

        auto frame = ffmpeg::make_frame(this, *frame_factory, av_frame, nullptr, core::color_space::bt709, scale_mode, true);
        frame_ = core::draw_frame(std::move(frame));

        CASPAR_LOG(info) << print() << L" Initialized";
    }

    // frame_producer

    core::draw_frame last_frame(const core::video_field field) override { return frame_; }

    core::draw_frame first_frame(const core::video_field field) override { return frame_; }

    bool is_ready() override { return true; }

    core::draw_frame receive_impl(const core::video_field field, int nb_samples) override
    {
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

//    if (boost::iequals(params.at(0), L"[PNG_BASE64]")) {
//        if (params.size() < 2)
//            return core::frame_producer::empty();
//
//        auto png_data = from_base64(u8(params.at(1)));
//
//        return spl::make_shared<image_producer>(dependencies.frame_factory, std::move(png_data), length, scale_mode);
//    }

    auto filename = find_file_within_dir_or_absolute(env::media_folder(), params.at(0), is_valid_file);
    if (!filename) {
        return core::frame_producer::empty();
    }

    return spl::make_shared<image_producer>(dependencies.frame_factory, filename->wstring(), length, scale_mode);
}

}} // namespace caspar::image
