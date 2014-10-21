/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   support/xpressive.hpp
 * \author Andrey Semashev
 * \date   18.07.2009
 *
 * This header enables Boost.Xpressive support for Boost.Log.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SUPPORT_XPRESSIVE_HPP_INCLUDED_
#define BOOST_LOG_SUPPORT_XPRESSIVE_HPP_INCLUDED_

#include <boost/mpl/bool.hpp>
#include <boost/xpressive/basic_regex.hpp>
#include <boost/xpressive/regex_constants.hpp>
#include <boost/xpressive/regex_algorithms.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/functional.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

//! The trait verifies if the type can be converted to a Boost.Xpressive regex
template< typename T >
struct is_xpressive_regex< T, true >
{
private:
    typedef char yes_type;
    struct no_type { char dummy[2]; };

    template< typename U >
    static yes_type check(xpressive::basic_regex< U > const&);
    static no_type check(...);
    static T& get_T();

public:
    enum { value = sizeof(check(get_T())) == sizeof(yes_type) };
    typedef mpl::bool_< value > type;
};

//! The regex matching functor implementation
template< >
struct matches_fun_impl< boost_xpressive_expression_tag >
{
    template< typename StringT, typename T >
    static bool matches(
        StringT const& str,
        xpressive::basic_regex< T > const& expr,
        xpressive::regex_constants::match_flag_type flags = xpressive::regex_constants::match_default)
    {
        return xpressive::regex_match(str, expr, flags);
    }
};

} // namespace aux

} // namespace log

} // namespace boost

#endif // BOOST_LOG_SUPPORT_XPRESSIVE_HPP_INCLUDED_
