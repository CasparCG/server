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

namespace caspar { namespace core {

class frame_visitor
{
  public:
    frame_visitor()                     = default;
    frame_visitor(const frame_visitor&) = delete;
    virtual ~frame_visitor()            = default;

    frame_visitor& operator=(const frame_visitor&) = delete;

    virtual void push(const struct frame_transform& transform) = 0;
    virtual void visit(const class const_frame& frame)         = 0;
    virtual void pop()                                         = 0;
};

}} // namespace caspar::core