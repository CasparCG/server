/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   syslog_constants.hpp
 * \author Andrey Semashev
 * \date   08.01.2008
 *
 * The header contains definition of constants related to Syslog API. The constants can be
 * used in other places without the Syslog backend.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SINKS_SYSLOG_CONSTANTS_HPP_INCLUDED_HPP_
#define BOOST_LOG_SINKS_SYSLOG_CONSTANTS_HPP_INCLUDED_HPP_

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

namespace sinks {

namespace syslog {

    struct level_tag;
    //! A tagged integal type that represents log record level for the syslog API
    typedef boost::log::aux::tagged_integer< int, level_tag > level_t;
    /*!
     * The function constructs log record level from an integer
     */
    inline level_t make_level(int lev)
    {
        level_t level = { lev };
        return level;
    }

    struct facility_tag;
    //! A tagged integal type that represents log source facility for the syslog API
    typedef boost::log::aux::tagged_integer< int, facility_tag > facility_t;
    /*!
     * The function constructs log source facility from an integer
     */
    inline facility_t make_facility(int fac)
    {
        facility_t facility = { fac };
        return facility;
    }

    //  Syslog record levels
    BOOST_LOG_EXPORT extern const level_t emergency;                //!< Equivalent to LOG_EMERG in syslog API
    BOOST_LOG_EXPORT extern const level_t alert;                    //!< Equivalent to LOG_ALERT in syslog API
    BOOST_LOG_EXPORT extern const level_t critical;                 //!< Equivalent to LOG_CRIT in syslog API
    BOOST_LOG_EXPORT extern const level_t error;                    //!< Equivalent to LOG_ERROR in syslog API
    BOOST_LOG_EXPORT extern const level_t warning;                  //!< Equivalent to LOG_WARNING in syslog API
    BOOST_LOG_EXPORT extern const level_t notice;                   //!< Equivalent to LOG_NOTICE in syslog API
    BOOST_LOG_EXPORT extern const level_t info;                     //!< Equivalent to LOG_INFO in syslog API
    BOOST_LOG_EXPORT extern const level_t debug;                    //!< Equivalent to LOG_DEBUG in syslog API

    //  Syslog facility codes
    BOOST_LOG_EXPORT extern const facility_t kernel;                //!< Kernel messages
    BOOST_LOG_EXPORT extern const facility_t user;                  //!< User-level messages. Equivalent to LOG_USER in syslog API.
    BOOST_LOG_EXPORT extern const facility_t mail;                  //!< Mail system messages. Equivalent to LOG_MAIL in syslog API.
    BOOST_LOG_EXPORT extern const facility_t daemon;                //!< System daemons. Equivalent to LOG_DAEMON in syslog API.
    BOOST_LOG_EXPORT extern const facility_t security0;             //!< Security/authorization messages
    BOOST_LOG_EXPORT extern const facility_t syslogd;               //!< Messages from the syslogd daemon. Equivalent to LOG_SYSLOG in syslog API.
    BOOST_LOG_EXPORT extern const facility_t printer;               //!< Line printer subsystem. Equivalent to LOG_LPR in syslog API.
    BOOST_LOG_EXPORT extern const facility_t news;                  //!< Network news subsystem. Equivalent to LOG_NEWS in syslog API.
    BOOST_LOG_EXPORT extern const facility_t uucp;                  //!< Messages from UUCP subsystem. Equivalent to LOG_UUCP in syslog API.
    BOOST_LOG_EXPORT extern const facility_t clock0;                //!< Messages from the clock daemon
    BOOST_LOG_EXPORT extern const facility_t security1;             //!< Security/authorization messages
    BOOST_LOG_EXPORT extern const facility_t ftp;                   //!< Messages from FTP daemon
    BOOST_LOG_EXPORT extern const facility_t ntp;                   //!< Messages from NTP daemon
    BOOST_LOG_EXPORT extern const facility_t log_audit;             //!< Security/authorization messages
    BOOST_LOG_EXPORT extern const facility_t log_alert;             //!< Security/authorization messages
    BOOST_LOG_EXPORT extern const facility_t clock1;                //!< Messages from the clock daemon
    BOOST_LOG_EXPORT extern const facility_t local0;                //!< For local use. Equivalent to LOG_LOCAL0 in syslog API
    BOOST_LOG_EXPORT extern const facility_t local1;                //!< For local use. Equivalent to LOG_LOCAL1 in syslog API
    BOOST_LOG_EXPORT extern const facility_t local2;                //!< For local use. Equivalent to LOG_LOCAL2 in syslog API
    BOOST_LOG_EXPORT extern const facility_t local3;                //!< For local use. Equivalent to LOG_LOCAL3 in syslog API
    BOOST_LOG_EXPORT extern const facility_t local4;                //!< For local use. Equivalent to LOG_LOCAL4 in syslog API
    BOOST_LOG_EXPORT extern const facility_t local5;                //!< For local use. Equivalent to LOG_LOCAL5 in syslog API
    BOOST_LOG_EXPORT extern const facility_t local6;                //!< For local use. Equivalent to LOG_LOCAL6 in syslog API
    BOOST_LOG_EXPORT extern const facility_t local7;                //!< For local use. Equivalent to LOG_LOCAL7 in syslog API

} // namespace syslog

} // namespace sinks

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_SINKS_SYSLOG_CONSTANTS_HPP_INCLUDED_HPP_
