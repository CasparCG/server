/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   nt6_event_log_constants.cpp
 * \author Andrey Semashev
 * \date   07.11.2008
 *
 * The header contains definition of constants related to Windows NT 6 Event Log API.
 * The constants can be used in other places without the NT 6 backend.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SINKS_NT6_EVENT_LOG_CONSTANTS_HPP_INCLUDED_
#define BOOST_LOG_SINKS_NT6_EVENT_LOG_CONSTANTS_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/tagged_integer.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
// non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace experimental {

namespace sinks {

namespace etw {

    struct level_tag;
    //! A tagged integral type that represents log record level for the Windows API
    typedef boost::log::aux::tagged_integer< unsigned char, level_tag > level_t;
    /*!
     * The function constructs log record level from an integer
     */
    inline level_t make_level(unsigned char lev)
    {
        level_t level = { lev };
        return level;
    }

    //  Windows events severity levels
    BOOST_LOG_EXPORT extern const level_t log_always;               //!< Equivalent to WINEVENT_LEVEL_LOG_ALWAYS
    BOOST_LOG_EXPORT extern const level_t critical;                 //!< Equivalent to WINEVENT_LEVEL_CRITICAL
    BOOST_LOG_EXPORT extern const level_t error;                    //!< Equivalent to WINEVENT_LEVEL_ERROR
    BOOST_LOG_EXPORT extern const level_t warning;                  //!< Equivalent to WINEVENT_LEVEL_WARNING
    BOOST_LOG_EXPORT extern const level_t info;                     //!< Equivalent to WINEVENT_LEVEL_INFO
    BOOST_LOG_EXPORT extern const level_t verbose;                  //!< Equivalent to WINEVENT_LEVEL_VERBOSE

} // namespace etw

} // namespace sinks

} // namespace experimental

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_SINKS_NT6_EVENT_LOG_CONSTANTS_HPP_INCLUDED_
