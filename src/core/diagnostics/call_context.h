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
 * Author: Helge Norberg, helge.norberg@svt.se
 */

#pragma once

#include <string>

namespace caspar { namespace core { namespace diagnostics {

struct call_context
{
    int video_channel = -1;
    int layer         = -1;

    static call_context& for_thread();
    std::wstring         to_string() const;
};

class scoped_call_context
{
    call_context saved_;

    scoped_call_context(const scoped_call_context&) = delete;
    scoped_call_context& operator=(const scoped_call_context&) = delete;

  public:
    scoped_call_context() { saved_ = call_context::for_thread(); }
    ~scoped_call_context() { call_context::for_thread() = saved_; }
};

}}} // namespace caspar::core::diagnostics
