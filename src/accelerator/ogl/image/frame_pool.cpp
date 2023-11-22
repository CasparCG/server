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

#include "frame_pool.h"

namespace caspar { namespace accelerator { namespace ogl {

frame_pool::frame_pool(std::shared_ptr<core::frame_factory> frame_factory,
                       const void*                          tag,
                       const core::pixel_format_desc&       desc)
    : frame_factory_(std::move(frame_factory))
    , tag_(tag)
    , desc_(desc)
{
    // TODO
}

frame_pool::~frame_pool()
{
    // TODO
}

core::mutable_frame frame_pool::create_frame() { return frame_factory_->create_frame(tag_, desc_); }

}}} // namespace caspar::accelerator::ogl