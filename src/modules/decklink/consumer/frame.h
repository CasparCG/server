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
 * Author: Julian Waller, julian@superfly.tv
 */

#pragma once

#include "../StdAfx.h"

#include "config.h"

#include "../decklink_api.h"

#include <core/frame/frame.h>
#include <core/frame/frame_converter.h>
#include <core/video_format.h>

#include <memory>

namespace caspar { namespace decklink {

BMDPixelFormat get_pixel_format(const configuration::pixel_format_t& pixel_format);
int get_row_bytes(const core::video_format_desc& format_desc, const configuration::pixel_format_t& pixel_format);

core::frame_conversion_format get_download_format_for_port(const configuration&           root_config,
                                                           const port_configuration&      port_config,
                                                           const core::video_format_desc& decklink_format_desc);

std::shared_ptr<void> allocate_frame_data(const core::video_format_desc&       format_desc,
                                          const configuration::pixel_format_t& pixel_format);

std::shared_ptr<void> convert_frame_for_port(const core::video_format_desc&   decklink_format_desc,
                                             const array<const std::uint8_t>& frame1,
                                             const array<const std::uint8_t>& frame2,
                                             BMDFieldDominance                field_dominance,
                                             configuration::pixel_format_t    pixel_format);

}} // namespace caspar::decklink
