/*
 * Copyright (c) 2018 Norsk rikskringkasting AS
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

#include "../../fwd.h"
#include "../../video_format.h"

#include <common/memory.h>

#include <string>

namespace caspar { namespace core {

struct sting_info
{
    std::wstring mask_filename       = L"";
    std::wstring overlay_filename    = L"";
    uint32_t     trigger_point       = 0;
    uint32_t     audio_fade_start    = 0;
    uint32_t     audio_fade_duration = UINT32_MAX;
};

spl::shared_ptr<frame_producer> create_sting_producer(const frame_producer_dependencies& dependencies,
                                                      const frame_producer_and_attrs&    destination,
                                                      sting_info&                        info);

}} // namespace caspar::core
