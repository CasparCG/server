/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   support/spirit_classic.hpp
 * \author Andrey Semashev
 * \date   19.07.2009
 *
 * This header enables Boost.Spirit (classic) support for Boost.Log.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SUPPORT_SPIRIT_CLASSIC_HPP_INCLUDED_
#define BOOST_LOG_SUPPORT_SPIRIT_CLASSIC_HPP_INCLUDED_

#include <boost/mpl/bool.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/functional.hpp>

#if !defined(BOOST_LOG_NO_THREADS) && !defined(BOOST_SPIRIT_THREADSAFE)
/*
 * As Boost.Log filters may be called in multiple threads concurrently,
 * this may lead to using Boost.Spirit parsers in a multithreaded context.
 * In order to protect parsers properly, BOOST_SPIRIT_THREADSAFE macro should
 * be defined.
 *
 * If we got here, it means that the user did not define that macro and we
 * have to define it ourselves. However, it may also lead to ODR violations
 * or even total ignorance of this macro, if the user has included Boost.Spirit
 * headers before including this header, or uses Boost.Spirit without the macro
 * in other translation units. The only reliable way to settle this problem is to
 * define the macro for the whole project (i.e. all translation units).
 */
#warning Boost.Log: Boost.Spirit requires BOOST_SPIRIT_THREADSAFE macro to be defined if parsers are used in a multithreaded context. It is strongly recommended to define this macro project-wide.
#define BOOST_SPIRIT_THREADSAFE 1
#endif // !defined(BOOST_LOG_NO_THREADS) && !defined(BOOST_SPIRIT_THREADSAFE)

#include <boost/spirit/include/classic_parser.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

//! The trait verifies if the type can be converted to a Boost.Spirit (classic) parser
template< typename T >
struct is_spirit_classic_parser< T, true >
{
private:
    typedef char yes_type;
    struct no_type { char dummy[2]; };

    template< typename U >
    static yes_type check(spirit::classic::parser< U > const&);
    static no_type check(...);
    static T& get_T();

public:
    enum { value = sizeof(check(get_T())) == sizeof(yes_type) };
    typedef mpl::bool_< value > type;
};

//! The matching functor implementation
template< >
struct matches_fun_impl< boost_spirit_classic_expression_tag >
{
    template< typename StringT, typename ParserT >
    static bool matches(
        StringT const& str,
        ParserT const& expr)
    {
        typedef typename StringT::const_iterator const_iterator;
        spirit::classic::parse_info< const_iterator > info =
            spirit::classic::parse(str.begin(), str.end(), expr);
        return info.full;
    }
};

} // namespace aux

} // namespace log

} // namespace boost

#endif // BOOST_LOG_SUPPORT_SPIRIT_CLASSIC_HPP_INCLUDED_
