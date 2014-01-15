/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   basic_formatters.hpp
 * \author Andrey Semashev
 * \date   22.04.2007
 *
 * The header contains implementation of basic facilities for auto-generated formatters,
 * including the base class for formatters.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_FORMATTERS_BASIC_FORMATTERS_HPP_INCLUDED_
#define BOOST_LOG_FORMATTERS_BASIC_FORMATTERS_HPP_INCLUDED_

#include <iosfwd>
#include <string>
#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/attributes/attribute_values_view.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace formatters {

//! A base class for every formatter
struct formatter_base {};

/*!
 * \brief A type trait to detect formatters
 *
 * The \c is_formatter class is a metafunction that returns \c true if it is instantiated with
 * a formatter type and \c false otherwise.
 */
template< typename T >
struct is_formatter : public is_base_and_derived< formatter_base, T > {};

/*!
 * \brief A base class for formatters
 *
 * The \c basic_formatter class defines standard types that most formatters use and
 * have to provide in order to be valid functors. This class also enables
 * support for the \c is_formatter type trait, which allows the formatter
 * to take part in lambda expressions.
 */
template< typename CharT, typename DerivedT >
struct basic_formatter : public formatter_base
{
    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Stream type
    typedef std::basic_ostream< char_type > ostream_type;
    //! Attribute values set type
    typedef basic_attribute_values_view< char_type > values_view_type;
    //! Log record type
    typedef basic_record< char_type > record_type;

    //! Functor result type
    typedef void result_type;
};

} // namespace formatters

} // namespace log

} // namespace boost

#endif // BOOST_LOG_FORMATTERS_BASIC_FORMATTERS_HPP_INCLUDED_
