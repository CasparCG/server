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

#include "utf.h"

#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/trivial.hpp>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <boost/stacktrace.hpp>

#include <string>

namespace caspar { namespace log {

template <typename T>
void replace_nonprintable(std::basic_string<T, std::char_traits<T>, std::allocator<T>>& str, T with)
{
    std::locale loc;
    std::replace_if(
        str.begin(),
        str.end(),
        [&](T c) -> bool { return (!std::isprint(c, loc) && c != '\r' && c != '\n') || c > static_cast<T>(127); },
        with);
}

template <typename T>
std::basic_string<T> replace_nonprintable_copy(std::basic_string<T, std::char_traits<T>, std::allocator<T>> str, T with)
{
    replace_nonprintable(str, with);
    return str;
}

std::string current_exception_diagnostic_information();

using caspar_logger = boost::log::sources::wseverity_logger<boost::log::trivial::severity_level>;

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(logger, caspar_logger)
#define CASPAR_LOG(lvl) BOOST_LOG_SEV(::caspar::log::logger::get(), boost::log::trivial::severity_level::lvl)

void          add_file_sink(const std::wstring& file);
void          add_cout_sink();
bool          set_log_level(const std::wstring& lvl);
std::wstring& get_log_level();

inline std::wstring get_stack_trace()
{
    auto bt = boost::stacktrace::stacktrace();
    if (bt) {
        return caspar::u16(boost::stacktrace::detail::to_string(&bt.as_vector()[0], bt.size()));
    }
    return L"";
}

#define CASPAR_LOG_CURRENT_EXCEPTION()                                                                                 \
    try {                                                                                                              \
        CASPAR_LOG(error) << L"Exception: " << caspar::u16(::caspar::log::current_exception_diagnostic_information())  \
                          << L"\r\n"                                                                                   \
                          << caspar::log::get_stack_trace();                                                           \
    } catch (...) {                                                                                                    \
    }

#define CASPAR_LOG_CURRENT_CALL_STACK() // TODO (fix)

}} // namespace caspar::log
