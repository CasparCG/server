/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   chain.hpp
 * \author Andrey Semashev
 * \date   22.04.2007
 *
 * The header contains implementation of formatter chaining mechanism.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_FORMATTERS_CHAIN_HPP_INCLUDED_
#define BOOST_LOG_FORMATTERS_CHAIN_HPP_INCLUDED_

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/formatters/basic_formatters.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace formatters {

/*!
 * \brief A formatter compound that encapsulates two other formatters
 *
 * The formatting lambda expressions are decomposed by compiler into
 * a sequence of binary operators. This formatter does nothing but to
 * connect two other formatters that participate in such an expression.
 * When invoked, the formatter simply calls its first (or left-hand)
 * aggregated formatter and then calls its second (right-hand) formatter.
 *
 * Chaining formatters can be composed into trees which allows to construct
 * arbitrary-sized formatting lambda expressions.
 */
template< typename CharT, typename LeftFmtT, typename RightFmtT >
class fmt_chain :
    public basic_formatter< CharT, fmt_chain< CharT, LeftFmtT, RightFmtT > >
{
private:
    //! Base type
    typedef basic_formatter<
        CharT,
        fmt_chain< CharT, LeftFmtT, RightFmtT >
    > base_type;

public:
    //! Stream type
    typedef typename base_type::ostream_type ostream_type;
    //! Log record type
    typedef typename base_type::record_type record_type;

private:
    //! Left formatter
    LeftFmtT m_Left;
    //! Right formatter
    RightFmtT m_Right;

public:
    /*!
     * Constructor
     *
     * \param left Left-hand formatter
     * \param right Right-hand formatter
     */
    template< typename RightT >
    fmt_chain(LeftFmtT const& left, RightT const& right) : m_Left(left), m_Right(right) {}

    /*!
     * Formatting operator. Passes control to the left and right formatters.
     *
     * \param strm A reference to the stream, where the final text of the logging record is composed
     * \param record The logging record
     */
    void operator() (ostream_type& strm, record_type const& record) const
    {
        m_Left(strm, record);
        m_Right(strm, record);
    }
};

} // namespace formatters

} // namespace log

} // namespace boost

#endif // BOOST_LOG_FORMATTERS_CHAIN_HPP_INCLUDED_
