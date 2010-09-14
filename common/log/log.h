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

#include "../utility/string_convert.h"

namespace caspar{ namespace log{
	
namespace internal{

void init();

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

#define CASPAR_LOG_CURRENT_EXCEPTION() \
	try\
	{CASPAR_LOG(error) << caspar::common::widen(boost::current_exception_diagnostic_information());}\
	catch(...){}

}}

#if defined(_MSC_VER)
#pragma warning (pop)
#endif