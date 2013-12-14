/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   log/trivial.hpp
 * \author Andrey Semashev
 * \date   07.11.2009
 *
 * This header defines tools for trivial logging support
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_TRIVIAL_HPP_INCLUDED_
#define BOOST_LOG_TRIVIAL_HPP_INCLUDED_

#include <ostream>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/keywords/severity.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
// non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

#if !defined(BOOST_LOG_USE_CHAR)
#error Boost.Log: Trivial logging is available for narrow-character builds only. Use advanced initialization routines to setup wide-character logging.
#endif

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace trivial {

namespace aux {

    //! Initialization routine
    BOOST_LOG_EXPORT void init();

} // namespace aux

//! Trivial severity levels
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
    switch (lvl)
    {
    case trace:
        strm << "trace"; break;
    case debug:
        strm << "debug"; break;
    case info:
        strm << "info"; break;
    case warning:
        strm << "warning"; break;
    case error:
        strm << "error"; break;
    case fatal:
        strm << "fatal"; break;
    default:
        strm << static_cast< int >(lvl); break;
    }

    return strm;
}

//! Trivial logger type
#if !defined(BOOST_LOG_NO_THREADS)
typedef sources::severity_logger_mt< severity_level > trivial_logger;
#else
typedef sources::severity_logger< severity_level > trivial_logger;
#endif

#if !defined(BOOST_LOG_BUILDING_THE_LIB)

//! Global logger instance
BOOST_LOG_DECLARE_GLOBAL_LOGGER_INIT(logger, trivial_logger)
{
    trivial::aux::init();
    return trivial_logger(keywords::severity = info);
}

//! The macro is used to initiate logging
#define BOOST_LOG_TRIVIAL(lvl)\
    BOOST_LOG_STREAM_WITH_PARAMS(::boost::log::trivial::get_logger(),\
        (::boost::log::keywords::severity = ::boost::log::trivial::lvl))

#endif // !defined(BOOST_LOG_BUILDING_THE_LIB)

} // namespace trivial

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_TRIVIAL_HPP_INCLUDED_
