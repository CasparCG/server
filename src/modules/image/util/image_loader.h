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

#include <core/frame/pixel_format.h>
#include <core/frame/draw_frame.h>

#include <common/bit_depth.h>

#include <ffmpeg/util/av_util.h>

#include <memory>
#include <set>
#include <string>

#include <boost/filesystem.hpp>

struct FIBITMAP;

namespace caspar { namespace image {

struct loaded_image
{
    std::shared_ptr<FIBITMAP> bitmap;
    core::pixel_format        format;
    int                       stride;
    common::bit_depth         depth;
    bool                      is_straight;
};

std::shared_ptr<AVFrame> load_image2(const std::wstring& filename);

loaded_image load_image(const std::wstring& filename, bool allow_all_formats);
loaded_image load_png_from_memory(const void* memory_location, size_t size, bool allow_all_formats);

bool is_valid_file(const boost::filesystem::path& filename);

}} // namespace caspar::image
