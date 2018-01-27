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
#include "thread_info.h"
#include "enum_class.h"

#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/exception/all.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/stacktrace.hpp>

#include <string>
#include <locale>
#include <functional>
#include <memory>

namespace caspar { namespace log {

namespace internal{
void init();
std::string current_exception_diagnostic_information();
}

const char* remove_source_prefix(const char* file);

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

void add_file_sink(const std::wstring& file, const boost::log::filter& filter);
std::shared_ptr<void> add_preformatted_line_sink(std::function<void(std::string line)> formatted_line_sink);

enum class log_category
{
	normal			= 1,
	calltrace		= 2,
	communication	= 4
};
ENUM_ENABLE_BITWISE(log_category)
BOOST_LOG_ATTRIBUTE_KEYWORD(category, "Channel", ::caspar::log::log_category)

typedef boost::log::sources::wseverity_channel_logger_mt<boost::log::trivial::severity_level, log_category> caspar_logger;

BOOST_LOG_INLINE_GLOBAL_LOGGER_INIT(logger, caspar_logger)
{
	internal::init();
	return caspar_logger(
			boost::log::keywords::severity = boost::log::trivial::trace,
			boost::log::keywords::channel = log_category::normal);
}

#define CASPAR_LOG(lvl)\
	BOOST_LOG_CHANNEL_SEV(::caspar::log::logger::get(), ::caspar::log::log_category::normal,		::boost::log::trivial::lvl)
#define CASPAR_LOG_CALL(lvl)\
	BOOST_LOG_CHANNEL_SEV(::caspar::log::logger::get(), ::caspar::log::log_category::calltrace,		::boost::log::trivial::lvl)
#define CASPAR_LOG_COMMUNICATION(lvl)\
	BOOST_LOG_CHANNEL_SEV(::caspar::log::logger::get(), ::caspar::log::log_category::communication,	::boost::log::trivial::lvl)

#define CASPAR_LOG_CALL_STACK()	try{\
		CASPAR_LOG(info) << L"callstack" << L":\n" << boost::stacktrace::stacktrace();\
	}\
	catch(...){}

#define CASPAR_LOG_CURRENT_EXCEPTION() try{\
		CASPAR_LOG(error) << caspar::u16(::caspar::log::internal::current_exception_diagnostic_information()) << L"Caught at" << L":\n" << boost::stacktrace::stacktrace();\
	}\
	catch(...){}

#define CASPAR_LOG_CURRENT_EXCEPTION_AT_LEVEL(lvl) try{\
		CASPAR_LOG(lvl) << caspar::u16(::caspar::log::internal::current_exception_diagnostic_information()) << L"Caught at" << L"):\n" << boost::stacktrace::stacktrace();\
	}\
	catch(...){}

void set_log_level(const std::wstring& lvl);
void set_log_category(const std::wstring& cat, bool enabled);

void print_child(
		boost::log::trivial::severity_level level,
		const std::wstring& indent,
		const std::wstring& elem,
		const boost::property_tree::wptree& tree);

}}
