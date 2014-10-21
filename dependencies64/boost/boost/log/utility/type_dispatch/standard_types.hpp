/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   standard_types.hpp
 * \author Andrey Semashev
 * \date   19.05.2007
 *
 * The header contains definition of standard types supported by the library by default.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_STANDARD_TYPES_HPP_INCLUDED_
#define BOOST_LOG_STANDARD_TYPES_HPP_INCLUDED_

#include <string>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/utility/string_literal.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

/*!
 * An MPL-sequence of integral types of attributes, supported by default
 */
typedef mpl::vector<
    bool,
    char,
#if !defined(BOOST_NO_INTRINSIC_WCHAR_T)
    wchar_t,
#endif
    signed char,
    unsigned char,
    short,
    unsigned short,
    int,
    unsigned int,
    long,
    unsigned long
#if defined(BOOST_HAS_LONG_LONG)
    , long long
    , unsigned long long
#endif // defined(BOOST_HAS_LONG_LONG)
>::type integral_types;

/*!
 * An MPL-sequence of FP types of attributes, supported by default
 */
typedef mpl::vector<
    float,
    double,
    long double
>::type floating_point_types;

/*!
 * An MPL-sequence of all numeric types of attributes, supported by default
 */
typedef mpl::joint_view<
    integral_types,
    floating_point_types
>::type numeric_types;

/*!
 * An MPL-sequence of string types of attributes, supported by default
 */
template< typename CharT >
struct basic_string_types :
    public mpl::vector<
        std::basic_string< CharT >,
        basic_string_literal< CharT >
    >::type
{
};

#ifdef BOOST_LOG_USE_CHAR
typedef basic_string_types< char > string_types;        //!< Convenience typedef for narrow-character string types
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
typedef basic_string_types< wchar_t > wstring_types;    //!< Convenience typedef for wide-character string types
#endif

/*!
 * An auxiliary type sequence maker. The sequence contains all
 * attribute value types that are supported by the library by default.
 */
template< typename CharT >
struct make_default_attribute_types :
    public mpl::joint_view<
        numeric_types,
        basic_string_types< CharT >
    >
{
};

} // namespace log

} // namespace boost

#endif // BOOST_LOG_STANDARD_TYPES_HPP_INCLUDED_
