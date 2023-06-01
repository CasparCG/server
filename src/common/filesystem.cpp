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
#include "except.h"

#include "filesystem.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace caspar {

boost::filesystem::path get_relative(const boost::filesystem::path& file, const boost::filesystem::path& relative_to)
{
    auto result       = file.filename();
    auto current_path = file;

    if (boost::filesystem::equivalent(current_path, relative_to))
        return L"";

    while (true) {
        current_path = current_path.parent_path();

        if (boost::filesystem::equivalent(current_path, relative_to))
            break;

        if (current_path.empty())
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info("File " + file.string() + " not relative to folder " +
                                                                  relative_to.string()));

        result = current_path.filename() / result;
    }

    return result;
}

boost::filesystem::path get_relative_without_extension(const boost::filesystem::path& file,
                                                       const boost::filesystem::path& relative_to)
{
    return get_relative(file.parent_path() / file.stem(), relative_to);
}

} // namespace caspar
