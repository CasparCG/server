/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   csv_decorator.hpp
 * \author Andrey Semashev
 * \date   07.06.2009
 *
 * The header contains implementation of CSV-style decorator. See:
 * http://en.wikipedia.org/wiki/Comma-separated_values
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_FORMATTERS_CSV_DECORATOR_HPP_INCLUDED_
#define BOOST_LOG_FORMATTERS_CSV_DECORATOR_HPP_INCLUDED_

#include <boost/range/iterator_range.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/formatters/char_decorator.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace formatters {

namespace aux {

    template< typename >
    struct csv_decorator_traits;

#ifdef BOOST_LOG_USE_CHAR
    template< >
    struct csv_decorator_traits< char >
    {
        static boost::iterator_range< const char* const* > get_patterns()
        {
            static const char* const patterns[] =
            {
                "\""
            };
            return boost::make_iterator_range(patterns);
        }
        static boost::iterator_range< const char* const* > get_replacements()
        {
            static const char* const replacements[] =
            {
                "\"\""
            };
            return boost::make_iterator_range(replacements);
        }
    };
#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T
    template< >
    struct csv_decorator_traits< wchar_t >
    {
        static boost::iterator_range< const wchar_t* const* > get_patterns()
        {
            static const wchar_t* const patterns[] =
            {
                L"\""
            };
            return boost::make_iterator_range(patterns);
        }
        static boost::iterator_range< const wchar_t* const* > get_replacements()
        {
            static const wchar_t* const replacements[] =
            {
                L"\"\""
            };
            return boost::make_iterator_range(replacements);
        }
    };
#endif // BOOST_LOG_USE_WCHAR_T

    struct fmt_csv_decorator_gen
    {
        template< typename FormatterT >
        fmt_char_decorator< FormatterT > operator[] (FormatterT const& fmt) const
        {
            typedef fmt_char_decorator< FormatterT > decorator;
            typedef csv_decorator_traits< typename decorator::char_type > traits_t;
            return decorator(
                fmt,
                traits_t::get_patterns(),
                traits_t::get_replacements());
        }
    };

} // namespace aux

#ifndef BOOST_LOG_DOXYGEN_PASS

const aux::fmt_csv_decorator_gen csv_dec = {};

#else // BOOST_LOG_DOXYGEN_PASS

/*!
 * CSV-style decorator generator object. The decorator doubles double quotes that may be found
 * in the output. See http://en.wikipedia.org/wiki/Comma-separated_values for more information on
 * the CSV format. The generator provides <tt>operator[]</tt> that can be used to construct
 * the actual decorator. For example:
 *
 * <code>
 * csv_dec[ attr< std::string >("MyAttr") ]
 * </code>
 */
implementation_defined csv_dec;

#endif // BOOST_LOG_DOXYGEN_PASS

} // namespace formatters

} // namespace log

} // namespace boost

#endif // BOOST_LOG_FORMATTERS_CSV_DECORATOR_HPP_INCLUDED_
