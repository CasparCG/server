/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   to_file.hpp
 * \author Andrey Semashev
 * \date   16.05.2008
 *
 * The header contains implementation of convenience functions for enabling logging to a file.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_UTILITY_INIT_TO_FILE_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_INIT_TO_FILE_HPP_INCLUDED_

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/parameter/parameters.hpp> // for is_named_argument
#include <boost/preprocessor/comparison/greater.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_shifted_params.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/sink_init_helpers.hpp>
#include <boost/log/detail/parameter_tools.hpp>
#include <boost/log/core/core.hpp>
#ifndef BOOST_LOG_NO_THREADS
#include <boost/log/sinks/sync_frontend.hpp>
#else
#include <boost/log/sinks/unlocked_frontend.hpp>
#endif
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/keywords/scan_method.hpp>

//! \cond
#ifndef BOOST_LOG_NO_THREADS
#define BOOST_LOG_FILE_SINK_FRONTEND_INTERNAL sinks::synchronous_sink
#else
#define BOOST_LOG_FILE_SINK_FRONTEND_INTERNAL sinks::unlocked_sink
#endif
//! \endcond

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

//! The function creates a file collector according to the specified arguments
template< typename ArgsT >
inline shared_ptr< sinks::file::collector > setup_file_collector(ArgsT const&, mpl::true_ const&)
{
    return shared_ptr< sinks::file::collector >();
}
template< typename ArgsT >
inline shared_ptr< sinks::file::collector > setup_file_collector(ArgsT const& args, mpl::false_ const&)
{
    return sinks::file::make_collector(args);
}

//! The function constructs the sink and adds it to the core
template< typename CharT, typename ArgsT >
shared_ptr<
    BOOST_LOG_FILE_SINK_FRONTEND_INTERNAL<
        sinks::basic_text_file_backend< CharT >
    >
> init_log_to_file(ArgsT const& args)
{
    typedef sinks::basic_text_file_backend< CharT > backend_t;
    shared_ptr< backend_t > pBackend = boost::make_shared< backend_t >(args);

    shared_ptr< sinks::file::collector > pCollector = aux::setup_file_collector(args,
        typename is_void< typename parameter::binding< ArgsT, keywords::tag::target, void >::type >::type());
    if (pCollector)
    {
        pBackend->set_file_collector(pCollector);
        pBackend->scan_for_files(args[keywords::scan_method | sinks::file::scan_matching]);
    }

    aux::setup_formatter(*pBackend, args,
        typename is_void< typename parameter::binding< ArgsT, keywords::tag::format, void >::type >::type());

    shared_ptr< BOOST_LOG_FILE_SINK_FRONTEND_INTERNAL< backend_t > > pSink =
        boost::make_shared< BOOST_LOG_FILE_SINK_FRONTEND_INTERNAL< backend_t > >(pBackend);

    aux::setup_filter(*pSink, args,
        typename is_void< typename parameter::binding< ArgsT, keywords::tag::filter, void >::type >::type());

    basic_core< CharT >::get()->add_sink(pSink);

    return pSink;
}

//! The function wraps the argument into a file_name named argument, if needed
template< typename T >
inline T const& wrap_file_name(T const& arg, mpl::true_)
{
    return arg;
}
template< typename T >
inline typename parameter::aux::tag< keywords::tag::file_name, T const >::type
wrap_file_name(T const& arg, mpl::false_)
{
    return keywords::file_name = arg;
}

} // namespace aux

#ifndef BOOST_LOG_DOXYGEN_PASS

#define BOOST_LOG_INIT_LOG_TO_FILE_INTERNAL(z, n, data)\
    template< typename CharT, BOOST_PP_ENUM_PARAMS(n, typename T) >\
    inline shared_ptr<\
        BOOST_LOG_FILE_SINK_FRONTEND_INTERNAL<\
            sinks::basic_text_file_backend< CharT >\
        >\
    > init_log_to_file(BOOST_PP_ENUM_BINARY_PARAMS(n, T, const& arg))\
    {\
        return aux::init_log_to_file< CharT >((\
            aux::wrap_file_name(arg0, typename parameter::aux::is_named_argument< T0 >::type())\
            BOOST_PP_COMMA_IF(BOOST_PP_GREATER(n, 1))\
            BOOST_PP_ENUM_SHIFTED_PARAMS(n, arg)\
            ));\
    }

BOOST_PP_REPEAT_FROM_TO(1, BOOST_LOG_MAX_PARAMETER_ARGS, BOOST_LOG_INIT_LOG_TO_FILE_INTERNAL, ~)

#undef BOOST_LOG_INIT_LOG_TO_FILE_INTERNAL

#if defined(BOOST_LOG_USE_CHAR)

#define BOOST_LOG_INIT_LOG_TO_FILE_INTERNAL(z, n, data)\
    template< BOOST_PP_ENUM_PARAMS(n, typename T) >\
    inline shared_ptr<\
        BOOST_LOG_FILE_SINK_FRONTEND_INTERNAL<\
            sinks::text_file_backend\
        >\
    > init_log_to_file(BOOST_PP_ENUM_BINARY_PARAMS(n, T, const& arg))\
    {\
        return init_log_to_file< char >(BOOST_PP_ENUM_PARAMS(n, arg));\
    }

BOOST_PP_REPEAT_FROM_TO(1, BOOST_LOG_MAX_PARAMETER_ARGS, BOOST_LOG_INIT_LOG_TO_FILE_INTERNAL, ~)

#undef BOOST_LOG_INIT_LOG_TO_FILE_INTERNAL

#endif // defined(BOOST_LOG_USE_CHAR)

#if defined(BOOST_LOG_USE_WCHAR_T)

#define BOOST_LOG_INIT_LOG_TO_FILE_INTERNAL(z, n, data)\
    template< BOOST_PP_ENUM_PARAMS(n, typename T) >\
    inline shared_ptr<\
        BOOST_LOG_FILE_SINK_FRONTEND_INTERNAL<\
            sinks::wtext_file_backend\
        >\
    > winit_log_to_file(BOOST_PP_ENUM_BINARY_PARAMS(n, T, const& arg))\
    {\
        return init_log_to_file< wchar_t >(BOOST_PP_ENUM_PARAMS(n, arg));\
    }

BOOST_PP_REPEAT_FROM_TO(1, BOOST_LOG_MAX_PARAMETER_ARGS, BOOST_LOG_INIT_LOG_TO_FILE_INTERNAL, ~)

#undef BOOST_LOG_INIT_LOG_TO_FILE_INTERNAL

#endif // defined(BOOST_LOG_USE_WCHAR_T)

#else // BOOST_LOG_DOXYGEN_PASS

/*!
 * The function initializes the logging library to write logs to a file stream.
 *
 * \param args A number of named arguments. The following parameters are supported:
 *             \li \c file_name The file name or its pattern. This parameter is mandatory.
 *             \li \c open_mode The mask that describes the open mode for the file. See <tt>std::ios_base::openmode</tt>.
 *             \li \c rotation_size The size of the file at which rotation should occur. See <tt>basic_text_file_backend</tt>.
 *             \li \c time_based_rotation The predicate for time-based file rotations. See <tt>basic_text_file_backend</tt>.
 *             \li \c auto_flush A boolean flag that shows whether the sink should automatically flush the file
 *                               after each written record.
 *             \li \c target The target directory to store rotated files in. See <tt>file::make_collector</tt>.
 *             \li \c max_size The maximum total size of rotated files in the target directory. See <tt>file::make_collector</tt>.
 *             \li \c min_free_space Minimum free space in the target directory. See <tt>file::make_collector</tt>.
 *             \li \c scan_method The method of scanning the target directory for log files. See <tt>file::scan_method</tt>.
 *             \li \c filter Specifies a filter to install into the sink. May be a string that represents a filter,
 *                           or a filter lambda expression.
 *             \li \c format Specifies a formatter to install into the sink. May be a string that represents a formatter,
 *                           or a formatter lambda expression (either streaming or Boost.Format-like notation).
 * \return Pointer to the constructed sink.
 */
template< typename CharT, typename... ArgsT >
shared_ptr<
    BOOST_LOG_FILE_SINK_FRONTEND_INTERNAL<
        sinks::basic_text_file_backend< CharT >
    >
> init_log_to_file(ArgsT... const& args);

/*!
 * Equivalent to <tt>init_log_to_file< char >(args...);</tt>
 *
 * \overload
 */
template< typename... ArgsT >
shared_ptr<
    BOOST_LOG_FILE_SINK_FRONTEND_INTERNAL<
        sinks::text_file_backend
    >
> init_log_to_file(ArgsT... const& args);

/*!
 * Equivalent to <tt>init_log_to_file< wchar_t >(args...);</tt>
 */
template< typename... ArgsT >
shared_ptr<
    BOOST_LOG_FILE_SINK_FRONTEND_INTERNAL<
        sinks::wtext_file_backend
    >
> winit_log_to_file(ArgsT... const& args);

#endif // BOOST_LOG_DOXYGEN_PASS

} // namespace log

} // namespace boost

#undef BOOST_LOG_FILE_SINK_FRONTEND_INTERNAL

#endif // BOOST_LOG_UTILITY_INIT_TO_FILE_HPP_INCLUDED_
