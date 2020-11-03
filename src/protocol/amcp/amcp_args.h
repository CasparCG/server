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

#include <boost/algorithm/string.hpp>
#include <map>
#include <vector>

namespace caspar { namespace protocol { namespace amcp {

inline bool is_args_token(const std::wstring& message)
{
    return (message[0] == L'(' && message[message.size() - 1] == L')');
}

inline bool
get_arg_value(const std::map<std::wstring, std::wstring>& args, const std::wstring& key, std::wstring& value)
{
    auto it = args.find(boost::to_upper_copy(key));
    if (it != args.end()) {
        value = it->second;
        return true;
    }
    return false;
}

inline bool get_bool_arg(const std::map<std::wstring, std::wstring>& args, const std::wstring& key)
{
    std::wstring value;
    if (get_arg_value(args, key, value)) {
        return (value != L"0" && boost::to_upper_copy(value) != L"FALSE");
    }
    return false;
}

std::map<std::wstring, std::wstring> tokenize_args(const std::wstring& message);

}}} // namespace caspar::protocol::amcp
