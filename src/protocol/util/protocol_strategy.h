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

#include <common/memory.h>

namespace caspar { namespace IO {

/**
 * A handle for a protocol_strategy to use when interacting with the client.
 */
template <class CharT>
class client_connection
{
  public:
    using ptr = spl::shared_ptr<client_connection<CharT>>;

    virtual ~client_connection() {}

    virtual void         send(std::basic_string<CharT>&& data, bool skip_log = false) = 0;
    virtual void         disconnect()                                                 = 0;
    virtual std::wstring address() const                                              = 0;

    virtual void add_lifecycle_bound_object(const std::wstring& key, const std::shared_ptr<void>& lifecycle_bound) = 0;
    virtual std::shared_ptr<void> remove_lifecycle_bound_object(const std::wstring& key)                           = 0;
};

}} // namespace caspar::IO
