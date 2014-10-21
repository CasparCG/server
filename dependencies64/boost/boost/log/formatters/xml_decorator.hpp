/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   xml_decorator.hpp
 * \author Andrey Semashev
 * \date   07.06.2009
 *
 * The header contains implementation of XML-style decorator.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_FORMATTERS_XML_DECORATOR_HPP_INCLUDED_
#define BOOST_LOG_FORMATTERS_XML_DECORATOR_HPP_INCLUDED_

#include <boost/range/iterator_range.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/formatters/char_decorator.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace formatters {

namespace aux {

    template< typename >
    struct xml_decorator_traits;

#ifdef BOOST_LOG_USE_CHAR
    template< >
    struct xml_decorator_traits< char >
    {
        static boost::iterator_range< const char* const* > get_patterns()
        {
            static const char* const patterns[] =
            {
                "&", "<", ">", "'"
            };
            return boost::make_iterator_range(patterns);
        }
        static boost::iterator_range< const char* const* > get_replacements()
        {
            static const char* const replacements[] =
            {
                "&amp;", "&lt;", "&gt;", "&apos;"
            };
            return boost::make_iterator_range(replacements);
        }
    };
#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T
    template< >
    struct xml_decorator_traits< wchar_t >
    {
        static boost::iterator_range< const wchar_t* const* > get_patterns()
        {
            static const wchar_t* const patterns[] =
            {
                L"&", L"<", L">", L"'"
            };
            return boost::make_iterator_range(patterns);
        }
        static boost::iterator_range< const wchar_t* const* > get_replacements()
        {
            static const wchar_t* const replacements[] =
            {
                L"&amp;", L"&lt;", L"&gt;", L"&apos;"
            };
            return boost::make_iterator_range(replacements);
        }
    };
#endif // BOOST_LOG_USE_WCHAR_T

    struct fmt_xml_decorator_gen
    {
        template< typename FormatterT >
        fmt_char_decorator< FormatterT > operator[] (FormatterT const& fmt) const
        {
            typedef fmt_char_decorator< FormatterT > decorator;
            typedef xml_decorator_traits< typename decorator::char_type > traits_t;
            return decorator(
                fmt,
                traits_t::get_patterns(),
                traits_t::get_replacements());
        }
    };

} // namespace aux

#ifndef BOOST_LOG_DOXYGEN_PASS

const aux::fmt_xml_decorator_gen xml_dec = {};

#else // BOOST_LOG_DOXYGEN_PASS

/*!
 * XML-style decorator generator object. The decorator replaces characters that have special meaning
 * in XML documents with the corresponding decorated counterparts. The generator provides
 * <tt>operator[]</tt> that can be used to construct the actual decorator. For example:
 *
 * <code>
 * xml_dec[ attr< std::string >("MyAttr") ]
 * </code>
 */
implementation_defined xml_dec;

#endif // BOOST_LOG_DOXYGEN_PASS

} // namespace formatters

} // namespace log

} // namespace boost

#endif // BOOST_LOG_FORMATTERS_XML_DECORATOR_HPP_INCLUDED_
