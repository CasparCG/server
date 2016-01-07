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

#include "os/stack_trace.h"

#include <exception>
#include <list>

#include <boost/exception/all.hpp>
#include <boost/exception/error_info.hpp>
#include <boost/throw_exception.hpp>
#include <boost/noncopyable.hpp>

namespace caspar {

typedef boost::error_info<struct tag_arg_name_info,		std::string>	arg_name_info_t;
typedef boost::error_info<struct tag_arg_value_info,	std::string>	arg_value_info_t;
typedef boost::error_info<struct tag_msg_info,			std::string>	msg_info_t;
typedef boost::error_info<struct tag_call_stack_info,	std::string>	call_stack_info_t;
typedef boost::error_info<struct tag_error_info,		std::string>	error_info_t;
typedef boost::error_info<struct tag_source_info,		std::string>	source_info_t;
typedef boost::error_info<struct tag_file_name_info,	std::string>	file_name_info_t;
typedef boost::error_info<struct tag_context_info,		std::string>	context_info_t;

template<typename T>
inline arg_name_info_t		arg_name_info(const T& str)		{return arg_name_info_t(u8(str));}
template<typename T>
inline arg_value_info_t		arg_value_info(const T& str)	{return arg_value_info_t(u8(str));}
template<typename T>
inline msg_info_t			msg_info(const T& str)			{return msg_info_t(u8(str));}
template<typename T>
inline call_stack_info_t 	call_stack_info(const T& str)	{return call_stack_info_t(u8(str));}
template<typename T>
inline error_info_t			error_info(const T& str)		{return error_info_t(u8(str));}
template<typename T>
inline source_info_t		source_info(const T& str)		{return source_info_t(u8(str));}
template<typename T>
inline file_name_info_t		file_name_info(const T& str)	{return file_name_info_t(u8(str));}
template<typename T>
inline context_info_t		context_info(const T& str)		{return context_info_t(u8(str));}

typedef boost::error_info<struct tag_line_info, size_t>						line_info;
typedef boost::error_info<struct tag_nested_exception_, std::exception_ptr> nested_exception;

struct caspar_exception			: virtual boost::exception, virtual std::exception 
{
	caspar_exception(){}
	const char* what() const throw() override
	{
		return boost::diagnostic_information_what(*this);
	}
};

struct io_error					: virtual caspar_exception {};
struct directory_not_found		: virtual io_error {};
struct file_not_found			: virtual io_error {};
struct file_read_error          : virtual io_error {};
struct file_write_error         : virtual io_error {};

struct invalid_argument			: virtual caspar_exception {};
struct null_argument			: virtual invalid_argument {};
struct out_of_range				: virtual invalid_argument {};
struct programming_error		: virtual caspar_exception {};
struct bad_alloc				: virtual caspar_exception {};

struct invalid_operation		: virtual caspar_exception {};
struct operation_failed			: virtual caspar_exception {};
struct timed_out				: virtual caspar_exception {};

struct not_implemented			: virtual caspar_exception {};

struct user_error				: virtual caspar_exception {};
struct expected_user_error		: virtual user_error {};
struct not_supported			: virtual user_error {};

std::string get_context();

class scoped_context : boost::noncopyable
{
public:
	scoped_context();
	scoped_context(std::string msg);
	template <typename Str>
	scoped_context(Str msg)
		: scoped_context(u8(std::move(msg)))
	{
	}

	~scoped_context();
	void replace_msg(std::string msg);
	template <typename Str>
	void replace_msg(std::string msg)
	{
		replace_msg(u8(std::move(msg)));
	}
	void clear_msg();
private:
	std::list<std::string>&	for_thread_;
	std::string*			msg_;
};

#define _CASPAR_GENERATE_UNIQUE_IDENTIFIER_CAT(name, line) name##line
#define _CASPAR_GENERATE_UNIQUE_IDENTIFIER(name, line) _CASPAR_GENERATE_UNIQUE_IDENTIFIER_CAT(name, line)
#define CASPAR_SCOPED_CONTEXT_MSG(ctx_msg) ::caspar::scoped_context _CASPAR_GENERATE_UNIQUE_IDENTIFIER(SCOPED_CONTEXT, __LINE__)(u8(ctx_msg));

#define CASPAR_THROW_EXCEPTION(e) BOOST_THROW_EXCEPTION(e << call_stack_info(caspar::get_call_stack()) << context_info(get_context()))

std::string get_message_and_context(const caspar_exception& e);

}

