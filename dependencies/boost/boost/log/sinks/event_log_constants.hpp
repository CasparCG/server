/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   event_log_constants.hpp
 * \author Andrey Semashev
 * \date   07.11.2008
 *
 * The header contains definition of constants related to Windows NT Event Log API.
 * The constants can be used in other places without the event log backend.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SINKS_EVENT_LOG_CONSTANTS_HPP_INCLUDED_
#define BOOST_LOG_SINKS_EVENT_LOG_CONSTANTS_HPP_INCLUDED_

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

namespace event_log {

    struct event_id_tag;
    //! A tagged integral type that represents event identifier for the Windows API
    typedef boost::log::aux::tagged_integer< unsigned int, event_id_tag > event_id_t;
    /*!
     * The function constructs event identifier from an integer
     */
    inline event_id_t make_event_id(unsigned int id)
    {
        event_id_t iden = { id };
        return iden;
    }

    struct event_category_tag;
    //! A tagged integral type that represents event category for the Windows API
    typedef boost::log::aux::tagged_integer< unsigned short, event_category_tag > event_category_t;
    /*!
     * The function constructs event category from an integer
     */
    inline event_category_t make_event_category(unsigned short cat)
    {
        event_category_t category = { cat };
        return category;
    }

    struct event_type_tag;
    //! A tagged integral type that represents log record level for the Windows API
    typedef boost::log::aux::tagged_integer< unsigned short, event_type_tag > event_type_t;
    /*!
     * The function constructs log record level from an integer
     */
    inline event_type_t make_event_type(unsigned short lev)
    {
        event_type_t evt = { lev };
        return evt;
    }

    //  Windows event types
    BOOST_LOG_EXPORT extern const event_type_t success;                 //!< Equivalent to EVENTLOG_SUCCESS
    BOOST_LOG_EXPORT extern const event_type_t info;                    //!< Equivalent to EVENTLOG_INFORMATION_TYPE
    BOOST_LOG_EXPORT extern const event_type_t warning;                 //!< Equivalent to EVENTLOG_WARNING_TYPE
    BOOST_LOG_EXPORT extern const event_type_t error;                   //!< Equivalent to EVENTLOG_ERROR_TYPE

} // namespace event_log

} // namespace sinks

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_SINKS_EVENT_LOG_CONSTANTS_HPP_INCLUDED_
