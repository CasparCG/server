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

#include "../StdAfx.h"

#include "call_context.h"

namespace caspar { namespace core { namespace diagnostics {

thread_local call_context context;

call_context& call_context::for_thread() { return context; }

std::wstring call_context::to_string() const
{
    if (video_channel == -1)
        return L"[]";
    if (layer == -1)
        return L"[ch=" + std::to_wstring(video_channel) + L"]";
    return L"[ch=" + std::to_wstring(video_channel) + L"; layer=" + std::to_wstring(layer) + L"]";
}

}}} // namespace caspar::core::diagnostics
