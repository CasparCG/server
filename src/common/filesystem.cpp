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

#include "./os/filesystem.h"
#include "filesystem.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace caspar {

std::optional<boost::filesystem::path>
probe_path(const boost::filesystem::path&                             full_path,
           const std::function<bool(const boost::filesystem::path&)>& is_valid_file)
{
    auto parent = find_case_insensitive(full_path.parent_path().wstring());

    if (!parent)
        return {};

    auto dir = boost::filesystem::path(*parent);
    auto loc = std::locale(""); // Use system locale

    auto leaf_name     = full_path.filename().stem().wstring();
    auto has_extension = !full_path.filename().extension().wstring().empty();
    auto leaf_filename = full_path.filename().wstring();

    for (auto it = boost::filesystem::directory_iterator(dir); it != boost::filesystem::directory_iterator(); ++it) {
        if (has_extension) {
            auto it_path = it->path().filename().wstring();
            if (boost::iequals(it_path, leaf_filename, loc) && is_valid_file(it->path().wstring()))
                return it->path();
        } else if (boost::iequals(it->path().stem().wstring(), leaf_name, loc) && is_valid_file(it->path().wstring()))
            return it->path();
    }

    return {};
}

std::optional<boost::filesystem::path>
find_file_within_dir_or_absolute(const std::wstring&                                        parent_dir,
                                 const std::wstring&                                        filename,
                                 const std::function<bool(const boost::filesystem::path&)>& is_valid_file)
{
    // Try it assuming an absolute path was given
    auto file_path       = boost::filesystem::path(filename);
    auto file_path_match = probe_path(file_path, is_valid_file);
    if (file_path_match) {
        return file_path_match;
    }

    // Try and find within the default parent directory
    auto full_path = boost::filesystem::path(parent_dir) / boost::filesystem::path(filename);
    return probe_path(full_path, is_valid_file);
}

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
