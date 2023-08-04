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
#include <core/video_format.h>

#include <memory>

namespace caspar { namespace decklink {

std::shared_ptr<void> convert_frame_for_port(const core::video_format_desc& channel_format_desc,
                                         const core::video_format_desc& decklink_format_desc,
                                         const port_configuration&      config,
                                         const core::const_frame&       frame1,
                                         const core::const_frame&       frame2,
                                         BMDFieldDominance              field_dominance);

}} // namespace caspar::decklink