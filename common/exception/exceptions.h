#pragma once

#include <exception>
#include <boost/exception/all.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/throw_exception.hpp>

namespace caspar {

typedef boost::error_info<struct tag_arg_name_info, std::string> arg_name_info;
typedef boost::error_info<struct tag_arg_value_info, std::string> arg_value_info;
typedef boost::error_info<struct tag_msg_info, std::string> msg_info;
typedef boost::error_info<struct tag_inner_info, std::exception_ptr> inner_info;
typedef boost::error_info<struct tag_line_info, int> line_info;

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

struct invalid_operation		: virtual caspar_exception {};
struct operation_failed			: virtual caspar_exception {};

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