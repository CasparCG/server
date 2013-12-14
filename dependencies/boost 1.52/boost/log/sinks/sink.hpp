/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   sink.hpp
 * \author Andrey Semashev
 * \date   22.04.2007
 *
 * The header contains an interface declaration for all sinks. This interface is used by the
 * logging core to feed log records to sinks.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SINKS_SINK_HPP_INCLUDED_
#define BOOST_LOG_SINKS_SINK_HPP_INCLUDED_

#include <string>
#include <boost/noncopyable.hpp>
#include <boost/function/function0.hpp>
#include <boost/function/function1.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/attributes/attribute_values_view.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
// non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace sinks {

//! A base class for a logging sink frontend
template< typename CharT >
class BOOST_LOG_NO_VTABLE sink : noncopyable
{
public:
    //! Character type
    typedef CharT char_type;
    //! String type to be used as a message text holder
    typedef std::basic_string< char_type > string_type;
    //! Log record type
    typedef basic_record< char_type > record_type;
    //! Attribute values view type
    typedef basic_attribute_values_view< char_type > values_view_type;
    //! Filter function type
    typedef function1< bool, values_view_type const& > filter_type;
    //! An exception handler type
    typedef function0< void > exception_handler_type;

public:
    /*!
     * Virtual destructor
     */
    virtual ~sink() {}

    /*!
     * The method returns \c true if no filter is set or the attribute values pass the filter
     *
     * \param attributes A set of attribute values of a logging record
     */
    virtual bool will_consume(values_view_type const& attributes) = 0;

    /*!
     * The method puts logging message to the sink
     *
     * \param record Logging record to consume
     */
    virtual void consume(record_type const& record) = 0;

    /*!
     * The method attempts to put logging message to the sink. The method may be used by the
     * core in order to determine the most efficient order of sinks to feed records to in
     * case of heavy contention. Sink implementations may implement try/backoff logic in
     * order to improve overall logging throughput.
     *
     * \param record Logging record to consume
     * \return \c true, if the record was consumed, \c false, if not.
     */
    virtual bool try_consume(record_type const& record)
    {
        consume(record);
        return true;
    }
};

} // namespace sinks

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_SINKS_SINK_HPP_INCLUDED_
