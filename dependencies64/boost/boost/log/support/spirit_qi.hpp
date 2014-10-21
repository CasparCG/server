/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   support/spirit_qi.hpp
 * \author Andrey Semashev
 * \date   19.07.2009
 *
 * This header enables Boost.Spirit.Qi support for Boost.Log.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SUPPORT_SPIRIT_QI_HPP_INCLUDED_
#define BOOST_LOG_SUPPORT_SPIRIT_QI_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/functional.hpp>
#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi_domain.hpp>
#include <boost/spirit/include/support_component.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

//! The trait verifies if the type can be converted to a Boost.Spirit.Qi parser
template< typename T >
struct is_spirit_qi_parser< T, true > :
    public spirit::traits::is_component< spirit::qi::domain, T >
{
};

//! The matching functor implementation
template< >
struct matches_fun_impl< boost_spirit_qi_expression_tag >
{
    template< typename StringT, typename ParserT >
    static bool matches(
        StringT const& str,
        ParserT const& expr)
    {
        typedef typename StringT::const_iterator const_iterator;
        const_iterator it = str.begin(), end = str.end();
        return (spirit::qi::parse(it, end, expr) && it == end);
    }
};

} // namespace aux

} // namespace log

} // namespace boost

#endif // BOOST_LOG_SUPPORT_SPIRIT_QI_HPP_INCLUDED_
