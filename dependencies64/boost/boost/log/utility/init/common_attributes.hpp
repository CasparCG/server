/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   common_attributes.hpp
 * \author Andrey Semashev
 * \date   16.05.2008
 *
 * The header contains implementation of convenience functions for registering commonly used attributes.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_UTILITY_INIT_COMMON_ATTRIBUTES_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_INIT_COMMON_ATTRIBUTES_HPP_INCLUDED_

#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/core/core.hpp>
#include <boost/log/attributes/clock.hpp>
#include <boost/log/attributes/counter.hpp>
#include <boost/log/attributes/current_process_id.hpp>
#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/log/attributes/current_thread_id.hpp>
#endif

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

template< typename > struct add_common_attributes_constants;

#if defined(BOOST_LOG_USE_CHAR)
template< >
struct add_common_attributes_constants< char >
{
    static const char* line_id_attr_name() { return "LineID"; }
    static const char* time_stamp_attr_name() { return "TimeStamp"; }
    static const char* process_id_attr_name() { return "ProcessID"; }
#if !defined(BOOST_LOG_NO_THREADS)
    static const char* thread_id_attr_name() { return "ThreadID"; }
#endif
};
#endif // defined(BOOST_LOG_USE_CHAR)

#if defined(BOOST_LOG_USE_WCHAR_T)
template< >
struct add_common_attributes_constants< wchar_t >
{
    static const wchar_t* line_id_attr_name() { return L"LineID"; }
    static const wchar_t* time_stamp_attr_name() { return L"TimeStamp"; }
    static const wchar_t* process_id_attr_name() { return L"ProcessID"; }
#if !defined(BOOST_LOG_NO_THREADS)
    static const wchar_t* thread_id_attr_name() { return L"ThreadID"; }
#endif
};
#endif // defined(BOOST_LOG_USE_WCHAR_T)

} // namespace aux

/*!
 * \brief Simple attribute imitialization routine
 *
 * The function adds commonly used attributes to the logging system. Specifically, the following
 * attributes are registered globally:
 *
 * \li LineID - logging records counter with value type <tt>unsigned int</tt>
 * \li TimeStamp - local time generator with value type <tt>boost::posix_time::ptime</tt>
 * \li ProcessID - current process identifier with value type
 *     <tt>attributes::current_process_id::held_type</tt>
 * \li ThreadID - in multithreaded builds, current thread identifier with
 *     value type <tt>attributes::current_thread_id::held_type</tt>
 */
template< typename CharT >
void add_common_attributes()
{
    typedef aux::add_common_attributes_constants< CharT > traits_t;
    shared_ptr< basic_core< CharT > > pCore = basic_core< CharT >::get();
    pCore->add_global_attribute(
        traits_t::line_id_attr_name(),
        boost::make_shared< attributes::counter< unsigned int > >(1));
    pCore->add_global_attribute(
        traits_t::time_stamp_attr_name(),
        boost::make_shared< attributes::local_clock >());
    pCore->add_global_attribute(
        traits_t::process_id_attr_name(),
        boost::make_shared< attributes::current_process_id >());
#if !defined(BOOST_LOG_NO_THREADS)
    pCore->add_global_attribute(
        traits_t::thread_id_attr_name(),
        boost::make_shared< attributes::current_thread_id >());
#endif
}

#if defined(BOOST_LOG_USE_CHAR)
/*!
 * \brief Simple attribute imitialization routine
 *
 * Equivalent to: <tt>add_common_attributes< char >();</tt>
 *
 * The function works for narrow-character logging.
 */
inline void add_common_attributes()
{
    add_common_attributes< char >();
}
#endif // defined(BOOST_LOG_USE_CHAR)

#if defined(BOOST_LOG_USE_WCHAR_T)
/*!
 * \brief Simple attribute imitialization routine
 *
 * Equivalent to: <tt>add_common_attributes< wchar_t >();</tt>
 *
 * The function works for wide-character logging.
 */
inline void wadd_common_attributes()
{
    add_common_attributes< wchar_t >();
}
#endif // defined(BOOST_LOG_USE_WCHAR_T)

} // namespace log

} // namespace boost

#endif // BOOST_LOG_UTILITY_INIT_COMMON_ATTRIBUTES_HPP_INCLUDED_
