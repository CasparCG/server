/*
 * Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
 * Copyright (c) 2025 Den Frie Vilje ApS <hej@denfrievilje.dk>
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
 */
#pragma once

#include <core/producer/frame_producer.h>
#include <core/video_format.h>

#include <memory>
#include <string>
#include <vector>

namespace caspar { namespace ultralight {

spl::shared_ptr<core::frame_producer>
create_ul_producer(const core::frame_producer_dependencies& dependencies,
                   const std::vector<std::wstring>&         params);

spl::shared_ptr<core::frame_producer>
create_ul_cg_producer(const core::frame_producer_dependencies& dependencies,
                      const std::vector<std::wstring>&         params);

}} // namespace caspar::ultralight
