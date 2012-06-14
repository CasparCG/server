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

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4100)
#pragma warning (disable : 4127) // conditional expression is constant
#pragma warning (disable : 4512)
#pragma warning (disable : 4714) // marked as __forceinline not inlined
#pragma warning (disable : 4996) // _CRT_SECURE_NO_WARNINGS
#endif

#include <boost/log/detail/prologue.hpp>
#include <boost/log/keywords/severity.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#include <string>
#include <locale>

namespace caspar { namespace log {
	
namespace internal{
void init();
std::wstring get_call_stack();
}

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

void add_file_sink(const std::wstring& folder);

enum severity_level
{
	trace,
	debug,
	info,
	warning,
	error,
	fatal
};

template< typename CharT, typename TraitsT >
inline std::basic_ostream< CharT, TraitsT >& operator<< (
	std::basic_ostream< CharT, TraitsT >& strm, severity_level lvl)
{
	if(lvl == trace)
		strm << "trace";
	else if(lvl == debug)
		strm << "debug";
	else if(lvl == info)
		strm << "info";
	else if(lvl == warning)
		strm << "warning";
	else if(lvl == error)
		strm << "error";
	else if(lvl == fatal)
		strm << "fatal";
	else
		strm << static_cast<int>(lvl);

	return strm;
}

typedef boost::log::sources::wseverity_logger_mt<severity_level> caspar_logger;

BOOST_LOG_DECLARE_GLOBAL_LOGGER_INIT(logger, caspar_logger)
{
	internal::init();
	return caspar_logger(boost::log::keywords::severity = trace);
}

#define CASPAR_LOG(lvl)\
	BOOST_LOG_STREAM_WITH_PARAMS(::caspar::log::get_logger(),\
		(::boost::log::keywords::severity = ::caspar::log::lvl))

#define CASPAR_LOG_CALL_STACK()	try{\
		CASPAR_LOG(info) << L"callstack:\n" << caspar::log::internal::get_call_stack();\
	}\
	catch(...){}

#define CASPAR_LOG_CURRENT_EXCEPTION() try{\
		CASPAR_LOG(error)  << caspar::u16(boost::current_exception_diagnostic_information()) << L"Caught at:\n" << caspar::log::internal::get_call_stack();\
	}\
	catch(...){}
	
void set_log_level(const std::wstring& lvl);

}}

#if defined(_MSC_VER)
#pragma warning (pop)
#endif