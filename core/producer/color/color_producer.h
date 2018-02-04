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

#include <common/memory.h>

#include <core/fwd.h>

#include <cstdint>
#include <string>
#include <vector>

namespace caspar { namespace core {

bool try_get_color(const std::wstring& str, uint32_t& value);

spl::shared_ptr<frame_producer> create_color_producer(const spl::shared_ptr<frame_factory>& frame_factory,
                                                      uint32_t                              value);
spl::shared_ptr<frame_producer> create_color_producer(const spl::shared_ptr<frame_factory>& frame_factory,
                                                      const std::vector<std::wstring>&      params);
draw_frame create_color_frame(void* tag, const spl::shared_ptr<frame_factory>& frame_factory, uint32_t value);
draw_frame
create_color_frame(void* tag, const spl::shared_ptr<frame_factory>& frame_factory, const std::wstring& color);

}} // namespace caspar::core
