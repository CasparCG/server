/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   stream.hpp
 * \author Andrey Semashev
 * \date   22.04.2007
 *
 * The header contains implementation of a hook for streaming formatters.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_FORMATTERS_STREAM_HPP_INCLUDED_
#define BOOST_LOG_FORMATTERS_STREAM_HPP_INCLUDED_

#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/embedded_string_type.hpp>
#include <boost/log/utility/string_literal.hpp>
#include <boost/log/formatters/basic_formatters.hpp>
#include <boost/log/formatters/chain.hpp>
#include <boost/log/formatters/wrappers.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace formatters {

/*!
 * \brief A placeholder class to represent a stream in lambda expressions of formatters
 *
 * The \c stream_placeholder class template is a hook that allows to construct formatters
 * from streaming lambda expressions.
 */
template< typename CharT >
struct stream_placeholder
{
public:
    /*!
     * Trap operator to begin building the lambda expression
     *
     * \param fmt Either a formatter or an object to be wrapped into a formatter
     */
    template< typename T >
    typename wrap_if_not_formatter<
        CharT,
        typename boost::log::aux::make_embedded_string_type< T >::type
    >::type operator<< (T const& fmt) const
    {
        typedef typename wrap_if_not_formatter<
            CharT,
            typename boost::log::aux::make_embedded_string_type< T >::type
        >::type result_type;
        return result_type(fmt);
    }

#if !defined(BOOST_LOG_DOXYGEN_PASS) && !defined(BOOST_LOG_BROKEN_STRING_LITERALS)

#ifdef BOOST_LOG_USE_CHAR
    template< typename T, std::size_t LenV >
    inline typename enable_if<
        is_same< T, const char >,
        fmt_wrapper<
            char,
            string_literal
        >
    >::type operator<< (T(&str)[LenV]) const
    {
        return fmt_wrapper< char, string_literal >(str);
    }
#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T
    template< typename T, std::size_t LenV >
    inline typename enable_if<
        is_same< T, const wchar_t >,
        fmt_wrapper<
            wchar_t,
            wstring_literal
        >
    >::type operator<< (T(&str)[LenV]) const
    {
        return fmt_wrapper< wchar_t, wstring_literal >(str);
    }
#endif // BOOST_LOG_USE_WCHAR_T

#endif // !defined(BOOST_LOG_DOXYGEN_PASS) && !defined(BOOST_LOG_BROKEN_STRING_LITERALS)

    static const stream_placeholder instance;
};

template< typename CharT >
const stream_placeholder< CharT > stream_placeholder< CharT >::instance = {};

//  Placeholders to begin lambda expressions
namespace {

#ifdef BOOST_LOG_USE_CHAR
    //! A placeholder used to construct lambda expressions of streaming formatters for narrow-character logging
    stream_placeholder< char > const& stream = stream_placeholder< char >::instance;
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
    //! A placeholder used to construct lambda expressions of streaming formatters for wide-character logging
    stream_placeholder< wchar_t > const& wstream = stream_placeholder< wchar_t >::instance;
#endif

} // namespace

/*!
 * The operator chains streaming formatters or a formatter and a wrapped object
 *
 * \param left Left-hand formatter
 * \param right Right-hand operand. May either be a formatter or some another object. In the latter case the object
 *              will be wrapped into a surrogate formatter that will attempt to put the object into the formatting stream.
 * \return The constructed chained formatters
 */
template< typename CharT, typename LeftFmtT, typename RightT >
inline fmt_chain<
    CharT,
    LeftFmtT,
    typename wrap_if_not_formatter<
        CharT,
        typename boost::log::aux::make_embedded_string_type< RightT >::type
    >::type
> operator<< (basic_formatter< CharT, LeftFmtT > const& left, RightT const& right)
{
    typedef fmt_chain<
        CharT,
        LeftFmtT,
        typename wrap_if_not_formatter<
            CharT,
            typename boost::log::aux::make_embedded_string_type< RightT >::type
        >::type
    > result_type;
    return result_type(static_cast< LeftFmtT const& >(left), right);
}

#if !defined(BOOST_LOG_DOXYGEN_PASS) && !defined(BOOST_LOG_BROKEN_STRING_LITERALS)

#ifdef BOOST_LOG_USE_CHAR

template< typename FmtT, typename T, std::size_t LenV >
inline typename enable_if<
    is_same< T, const char >,
    fmt_chain<
        char,
        FmtT,
        fmt_wrapper<
            char,
            string_literal
        >
    >
>::type operator<< (basic_formatter< char, FmtT > const& left, T(&str)[LenV])
{
    return fmt_chain<
        char,
        FmtT,
        fmt_wrapper<
            char,
            string_literal
        >
    >(static_cast< FmtT const& >(left), fmt_wrapper< char, string_literal >(str));
}

#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T

template< typename FmtT, typename T, std::size_t LenV >
inline typename enable_if<
    is_same< T, const wchar_t >,
    fmt_chain<
        wchar_t,
        FmtT,
        fmt_wrapper<
            wchar_t,
            wstring_literal
        >
    >
>::type operator<< (basic_formatter< wchar_t, FmtT > const& left, T(&str)[LenV])
{
    return fmt_chain<
        wchar_t,
        FmtT,
        fmt_wrapper<
            wchar_t,
            wstring_literal
        >
    >(static_cast< FmtT const& >(left), fmt_wrapper< wchar_t, wstring_literal >(str));
}

#endif // BOOST_LOG_USE_WCHAR_T

#endif // !defined(BOOST_LOG_DOXYGEN_PASS) && !defined(BOOST_LOG_BROKEN_STRING_LITERALS)

} // namespace formatters

} // namespace log

} // namespace boost

#endif // BOOST_LOG_FORMATTERS_STREAM_HPP_INCLUDED_
