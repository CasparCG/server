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

#include <exception>
#include <boost/exception/all.hpp>
#include <boost/exception/error_info.hpp>
#include <boost/throw_exception.hpp>

namespace caspar {

typedef boost::error_info<struct tag_arg_name_info, std::string>		arg_name_info;
typedef boost::error_info<struct tag_arg_value_info, std::string>		arg_value_info;
typedef boost::error_info<struct tag_msg_info, std::string>				msg_info;
typedef boost::error_info<struct tag_errorstr, std::string>				errorstr;
typedef boost::error_info<struct tag_source_info, std::string>			source_info;
typedef boost::error_info<struct tag_line_info, size_t>					line_info;
typedef boost::error_info<struct errinfo_nested_exception_, std::exception_ptr> errinfo_nested_exception;

struct caspar_exception			: virtual boost::exception, virtual std::exception 
{
	caspar_exception(){}
	explicit caspar_exception(const char* msg) : std::exception(msg) {}
};

struct io_error					: virtual caspar_exception {};
struct directory_not_found		: virtual io_error {};
struct file_not_found			: virtual io_error {};
struct file_read_error          : virtual io_error {};

struct invalid_argument			: virtual caspar_exception {};
struct null_argument			: virtual invalid_argument {};
struct out_of_range				: virtual invalid_argument {};
struct bad_alloc				: virtual caspar_exception {};

struct invalid_operation		: virtual caspar_exception {};
struct operation_failed			: virtual caspar_exception {};
struct timed_out				: virtual caspar_exception {};

struct not_supported			: virtual caspar_exception {};
struct not_implemented			: virtual caspar_exception {};

}

namespace std
{

inline bool operator!=(const std::exception_ptr& lhs, const std::exception_ptr& rhs)
{
	return !(lhs == rhs);
}

inline bool operator!=(const std::exception_ptr& lhs, std::nullptr_t)
{
	return !(lhs == nullptr);
}

inline bool operator!=(std::nullptr_t, const std::exception_ptr& rhs)
{
	return !(nullptr == rhs);
}

}