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

#include "../../fwd.h"

#include <common/memory.h>
#include <common/tweener.h>

#include <string>

namespace caspar { namespace core {

enum class transition_type
{
    cut,
    mix,
    push,
    slide,
    wipe,
    count
};

enum class transition_direction
{
    from_left,
    from_right,
    count
};

struct transition_info
{
    int                  duration  = 0;
    transition_direction direction = transition_direction::from_left;
    transition_type      type      = transition_type::cut;
    caspar::tweener      tweener{L"linear"};
};

bool try_match_transition(const std::wstring& message, transition_info& transitionInfo);

spl::shared_ptr<frame_producer> create_transition_producer(const spl::shared_ptr<frame_producer>& destination,
                                                           const transition_info&                 info);

}} // namespace caspar::core
