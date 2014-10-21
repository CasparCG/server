/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   to_console.hpp
 * \author Andrey Semashev
 * \date   16.05.2008
 *
 * The header contains implementation of convenience functions for enabling logging to console.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_UTILITY_INIT_TO_CONSOLE_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_INIT_TO_CONSOLE_HPP_INCLUDED_

#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/sink_init_helpers.hpp>
#ifndef BOOST_LOG_NO_THREADS
#include <boost/log/sinks/sync_frontend.hpp>
#else
#include <boost/log/sinks/unlocked_frontend.hpp>
#endif
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/empty_deleter.hpp>
#include <boost/log/keywords/format.hpp>
#include <boost/log/keywords/filter.hpp>
#include <boost/log/keywords/auto_flush.hpp>

//! \cond
#ifndef BOOST_LOG_NO_THREADS
#define BOOST_LOG_CONSOLE_SINK_FRONTEND_INTERNAL sinks::synchronous_sink
#else
#define BOOST_LOG_CONSOLE_SINK_FRONTEND_INTERNAL sinks::unlocked_sink
#endif
//! \endcond

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

// The function creates and initializes the sink
template< typename CharT, typename ArgsT >
shared_ptr<
    BOOST_LOG_CONSOLE_SINK_FRONTEND_INTERNAL<
        sinks::basic_text_ostream_backend< CharT >
    >
> init_log_to_console(std::basic_ostream< CharT >& strm, ArgsT const& args)
{
    shared_ptr< std::basic_ostream< CharT > > pStream(&strm, empty_deleter());

    typedef sinks::basic_text_ostream_backend< CharT > backend_t;
    shared_ptr< backend_t > pBackend = boost::make_shared< backend_t >();

    pBackend->add_stream(pStream);
    pBackend->auto_flush(args[keywords::auto_flush | false]);

    aux::setup_formatter(*pBackend, args,
        typename is_void< typename parameter::binding< ArgsT, keywords::tag::format, void >::type >::type());

    typedef BOOST_LOG_CONSOLE_SINK_FRONTEND_INTERNAL< backend_t > sink_t;
    shared_ptr< sink_t > pSink = boost::make_shared< sink_t >(pBackend);

    aux::setup_filter(*pSink, args,
        typename is_void< typename parameter::binding< ArgsT, keywords::tag::filter, void >::type >::type());

    basic_core< CharT >::get()->add_sink(pSink);

    return pSink;
}

template< typename CharT >
struct default_console_stream;

#ifdef BOOST_LOG_USE_CHAR
template< >
struct default_console_stream< char >
{
    static std::ostream& get() { return std::clog; }
};
#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T
template< >
struct default_console_stream< wchar_t >
{
    static std::wostream& get() { return std::wclog; }
};
#endif // BOOST_LOG_USE_WCHAR_T

} // namespace aux

#ifndef BOOST_LOG_DOXYGEN_PASS

template< typename CharT >
inline shared_ptr<
    BOOST_LOG_CONSOLE_SINK_FRONTEND_INTERNAL<
        sinks::basic_text_ostream_backend< CharT >
    >
> init_log_to_console()
{
    return aux::init_log_to_console(
        aux::default_console_stream< CharT >::get(), keywords::auto_flush = false);
}


template< typename CharT >
inline shared_ptr<
    BOOST_LOG_CONSOLE_SINK_FRONTEND_INTERNAL<
        sinks::basic_text_ostream_backend< CharT >
    >
> init_log_to_console(std::basic_ostream< CharT >& strm)
{
    return aux::init_log_to_console(strm, keywords::auto_flush = false);
}

template< typename CharT, typename ArgT1 >
inline shared_ptr<
    BOOST_LOG_CONSOLE_SINK_FRONTEND_INTERNAL<
        sinks::basic_text_ostream_backend< CharT >
    >
> init_log_to_console(std::basic_ostream< CharT >& strm, ArgT1 const& arg1)
{
    return aux::init_log_to_console(strm, arg1);
}

template< typename CharT, typename ArgT1, typename ArgT2 >
inline shared_ptr<
    BOOST_LOG_CONSOLE_SINK_FRONTEND_INTERNAL<
        sinks::basic_text_ostream_backend< CharT >
    >
> init_log_to_console(std::basic_ostream< CharT >& strm, ArgT1 const& arg1, ArgT2 const& arg2)
{
    return aux::init_log_to_console(strm, (arg1, arg2));
}

template< typename CharT, typename ArgT1, typename ArgT2, typename ArgT3 >
inline shared_ptr<
    BOOST_LOG_CONSOLE_SINK_FRONTEND_INTERNAL<
        sinks::basic_text_ostream_backend< CharT >
    >
> init_log_to_console(std::basic_ostream< CharT >& strm, ArgT1 const& arg1, ArgT2 const& arg2, ArgT3 const& arg3)
{
    return aux::init_log_to_console(strm, (arg1, arg2, arg3));
}

#else // BOOST_LOG_DOXYGEN_PASS

/*!
 * The function constructs sink for the specified console stream and adds it to the core
 *
 * \param strm One of the standard console streams: <tt>std::cout</tt>, <tt>std::cerr</tt> or <tt>std::clog</tt>
 *             (or the corresponding wide-character analogues).
 * \param args Optional additional named arguments for the sink initialization. The following arguments are supported:
 *             \li \c filter Specifies a filter to install into the sink. May be a string that represents a filter,
 *                           or a filter lambda expression.
 *             \li \c format Specifies a formatter to install into the sink. May be a string that represents a formatter,
 *                           or a formatter lambda expression (either streaming or Boost.Format-like notation).
 *             \li \c auto_flush A boolean flag that shows whether the sink should automaticallu flush the stream
 *                               after each written record.
 * \return Pointer to the constructed sink.
 */
template< typename CharT, typename... ArgsT >
shared_ptr<
    BOOST_LOG_CONSOLE_SINK_FRONTEND_INTERNAL<
        sinks::basic_text_ostream_backend< CharT >
    >
> init_log_to_console(std::basic_ostream< CharT >& strm, ArgsT... const& args);

/*!
 * Equivalent to: <tt>init_log_to_console(std::clog);</tt> or <tt>init_log_to_console(std::wclog);</tt>,
 * depending on the \c CharT type.
 *
 * \overload
 */
template< typename CharT, typename... ArgsT >
shared_ptr<
    BOOST_LOG_CONSOLE_SINK_FRONTEND_INTERNAL<
        sinks::basic_text_ostream_backend< CharT >
    >
> init_log_to_console(ArgsT... const& args);

#endif // BOOST_LOG_DOXYGEN_PASS

#ifdef BOOST_LOG_USE_CHAR

/*!
 * The function constructs sink for the <tt>std::clog</tt> stream and adds it to the core
 *
 * \overload
 *
 * \return Pointer to the constructed sink.
 */
inline shared_ptr<
    BOOST_LOG_CONSOLE_SINK_FRONTEND_INTERNAL<
        sinks::text_ostream_backend
    >
> init_log_to_console()
{
    return init_log_to_console(std::clog);
}

#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T

/*!
 * The function constructs sink for the <tt>std::wclog</tt> stream and adds it to the core
 *
 * \return Pointer to the constructed sink.
 */
inline shared_ptr<
    BOOST_LOG_CONSOLE_SINK_FRONTEND_INTERNAL<
        sinks::wtext_ostream_backend
    >
> winit_log_to_console()
{
    return init_log_to_console(std::wclog);
}

#endif // BOOST_LOG_USE_WCHAR_T

} // namespace log

} // namespace boost

#undef BOOST_LOG_CONSOLE_SINK_FRONTEND_INTERNAL

#endif // BOOST_LOG_UTILITY_INIT_TO_CONSOLE_HPP_INCLUDED_
