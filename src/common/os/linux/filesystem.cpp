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

#include "../../stdafx.h"

#include "../filesystem.h"

#include <list>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

using namespace boost::filesystem;

namespace caspar {

boost::optional<std::wstring> find_case_insensitive(const std::wstring& case_insensitive)
{
    path p(case_insensitive);

    if (exists(p))
        return case_insensitive;

    p = absolute(p);
    path result;

    for (auto part : p) {
        auto concatenated = result / part;

        if (exists(concatenated)) {
            result = concatenated;
        } else {
            bool found = false;

            for (auto it = directory_iterator(absolute(result)); it != directory_iterator(); ++it) {
                auto leaf = it->path().leaf();

                if (boost::algorithm::iequals(part.wstring(), leaf.wstring())) {
                    result = result / leaf;
                    found  = true;
                    break;
                }
            }

            if (!found)
                return boost::none;
        }
    }

    return result.wstring();
}

std::wstring clean_path(std::wstring path)
{
    boost::replace_all(path, L"\\\\", L"/");
    boost::replace_all(path, L"\\", L"/");

    return path;
}

std::wstring ensure_trailing_slash(std::wstring folder)
{
    auto last_char = folder.at(folder.length() - 1);
    if (last_char != L'/')
        folder.append(L"/");

    return folder;
}

} // namespace caspar
