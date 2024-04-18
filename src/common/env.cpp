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
#include "env.h"

#include "version.h"

#include "except.h"
#include "log.h"
#include "os/filesystem.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/property_tree/ptree.hpp>

#include <boost/algorithm/string.hpp>
#include <fstream>

namespace caspar { namespace env {

std::wstring                 initial;
std::wstring                 media;
std::wstring                 ftemplate;
boost::property_tree::wptree pt;

void check_is_configured()
{
    if (pt.empty())
        CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info(L"Enviroment properties has not been configured"));
}

std::wstring resolve_or_create(const std::wstring& folder)
{
    auto found_path = find_case_insensitive(folder);

    if (found_path)
        return *found_path;
    boost::system::error_code ec;
    boost::filesystem::create_directories(folder, ec);

    if (ec)
        CASPAR_THROW_EXCEPTION(user_error()
                               << msg_info("Failed to create directory " + u8(folder) + " (" + ec.message() + ")"));

    return folder;
}

void ensure_writable(const std::wstring& folder)
{
    static const std::wstring CREATE_FILE_TEST = L"casparcg_test_writable.empty";

    boost::system::error_code   ec;
    boost::filesystem::path     test_file(folder + L"/" + CREATE_FILE_TEST);
    boost::filesystem::ofstream out(folder + L"/" + CREATE_FILE_TEST);

    if (out.fail()) {
        boost::filesystem::remove(test_file, ec);
        CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Directory " + folder + L" is not writable."));
    }

    out.close();
    boost::filesystem::remove(test_file, ec);
}

void configure(const std::wstring&          initial_path,
               const std::wstring&          media_path,
               const std::wstring&          template_path,
               boost::property_tree::wptree pt_)
{
    initial   = initial_path;
    media     = media_path;
    ftemplate = template_path;
    pt        = pt_;

    media     = ensure_trailing_slash(resolve_or_create(media));
    ftemplate = ensure_trailing_slash(resolve_or_create(ftemplate));

    ensure_writable(media);
    ensure_writable(ftemplate);
}

const std::wstring& initial_folder()
{
    check_is_configured();
    return initial;
}

const std::wstring& media_folder()
{
    check_is_configured();
    return media;
}

const std::wstring& template_folder()
{
    check_is_configured();
    return ftemplate;
}

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

const std::wstring& version()
{
    static std::wstring ver = u16(EXPAND_AND_QUOTE(CASPAR_GEN) "." EXPAND_AND_QUOTE(CASPAR_MAJOR) "." EXPAND_AND_QUOTE(
        CASPAR_MINOR) " " CASPAR_HASH " " CASPAR_TAG);
    return ver;
}

const boost::property_tree::wptree& properties()
{
    check_is_configured();
    return pt;
}

}} // namespace caspar::env
