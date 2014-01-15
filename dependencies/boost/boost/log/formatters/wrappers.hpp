/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   wrappers.hpp
 * \author Andrey Semashev
 * \date   22.04.2007
 *
 * The header contains implementation of wrappers that are used to construct lambda
 * expressions of formatters. These wrappers are used to convert third-party
 * objects (like string literals, for instance) into valid formatters.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_FORMATTERS_WRAPPERS_HPP_INCLUDED_
#define BOOST_LOG_FORMATTERS_WRAPPERS_HPP_INCLUDED_

#include <boost/ref.hpp>
#include <boost/mpl/not.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/formatters/basic_formatters.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace formatters {

/*!
 * \brief Formatter wrapper to output objects into streams
 *
 * The formatter aggregates some object and provides formatter interface. Upon formatting
 * the wrapper puts the wrapped object into the formatting stream.
 */
template< typename CharT, typename T >
class fmt_wrapper :
    public basic_formatter< CharT, fmt_wrapper< CharT, T > >
{
private:
    //! Base type
    typedef basic_formatter< CharT, fmt_wrapper< CharT, T > > base_type;

public:
    //! Stream type
    typedef typename base_type::ostream_type ostream_type;
    //! Log record type
    typedef typename base_type::record_type record_type;

private:
    //! Object to be output
    T m_T;

public:
    /*!
     * Constructor
     *
     * \param obj Object to be aggregated
     */
    explicit fmt_wrapper(T const& obj) : m_T(obj) {}

    /*!
     * Formatting operator. Puts the aggregated object into the \a strm stream.
     *
     * \param strm A reference to the stream, where the final text of the logging record is composed
     */
    void operator() (ostream_type& strm, record_type const&) const
    {
        strm << m_T;
    }
};

/*!
 * \brief Specialization to put objects into streams by reference
 *
 * The specialization allows to put into the formatting stream external (with regard to
 * the formatter) objects.
 */
template< typename CharT, typename T >
class fmt_wrapper< CharT, reference_wrapper< T > > :
    public basic_formatter< CharT, fmt_wrapper< CharT, reference_wrapper< T > > >
{
private:
    //! Base type
    typedef basic_formatter< CharT, fmt_wrapper< CharT, reference_wrapper< T > > > base_type;

public:
    //! Stream type
    typedef typename base_type::ostream_type ostream_type;
    //! Log record type
    typedef typename base_type::record_type record_type;

private:
    //! Reference to object to be output
    T& m_T;

public:
    /*!
     * Constructor
     *
     * \param obj Object to be referenced
     */
    explicit fmt_wrapper(reference_wrapper< T > const& obj) : m_T(obj.get()) {}

    /*!
     * Formatting operator. Puts the referenced object into the \a strm stream.
     *
     * \param strm A reference to the stream, where the final text of the logging record is composed
     */
    void operator() (ostream_type& strm, record_type const&) const
    {
        strm << m_T;
    }
};

//! A convenience class that conditionally wraps the type into a formatter
template< typename CharT, typename T, bool >
struct wrap_if_c
{
    typedef fmt_wrapper< CharT, T > type;
};

template< typename CharT, typename T >
struct wrap_if_c< CharT, T, false >
{
    typedef T type;
};

//! A convenience class that conditionally wraps the type into a formatter
template< typename CharT, typename T, typename PredT >
struct wrap_if :
    public wrap_if_c< CharT, T, PredT::value >
{
};

//! A convenience class that wraps the type into a formatter if it is not a formatter yet
template< typename CharT, typename T >
struct wrap_if_not_formatter :
    public wrap_if< CharT, T, mpl::not_< is_formatter< T > > >
{
};

} // namespace formatters

} // namespace log

} // namespace boost

#endif // BOOST_LOG_FORMATTERS_WRAPPERS_HPP_INCLUDED_
