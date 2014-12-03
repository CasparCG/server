/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include <boost/log/trivial.hpp>
#include <boost/log/sources/global_logger_storage.hpp>

#include <string>
#include <locale>

namespace caspar { namespace log {
	
namespace internal{
void init();
}

void add_file_sink(const std::wstring& folder);

typedef boost::log::sources::wseverity_logger_mt<boost::log::trivial::severity_level> caspar_logger;

BOOST_LOG_INLINE_GLOBAL_LOGGER_INIT(logger, caspar_logger)
{
	internal::init();
	return caspar_logger(boost::log::trivial::trace);
}

#define CASPAR_LOG(lvl)\
	BOOST_LOG_SEV(::caspar::log::logger::get(), ::boost::log::trivial::lvl)

#define CASPAR_LOG_CURRENT_EXCEPTION() \
	try\
	{CASPAR_LOG(error) << boost::current_exception_diagnostic_information().c_str();}\
	catch(...){}

void set_log_level(const std::wstring& lvl);

template<typename T>
inline void replace_nonprintable(std::basic_string<T, std::char_traits<T>, std::allocator<T>>& str, T with)
{
	std::locale loc;
	std::replace_if(str.begin(), str.end(), [&](T c)->bool {
		return 
			(!std::isprint(c, loc) 
			&& c != '\r' 
			&& c != '\n')
			|| c > static_cast<T>(127);
	}, with);
}

template<typename T>
inline std::basic_string<T> replace_nonprintable_copy(std::basic_string<T, std::char_traits<T>, std::allocator<T>> str, T with)
{
	replace_nonprintable(str, with);
	return str;
}

}}
