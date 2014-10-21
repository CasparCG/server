/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   clock.hpp
 * \author Andrey Semashev
 * \date   01.12.2007
 *
 * The header contains wall clock attribute implementation and typedefs.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_ATTRIBUTES_CLOCK_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_CLOCK_HPP_INCLUDED_

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/basic_attribute_value.hpp>
#include <boost/log/attributes/time_traits.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace attributes {

/*!
 * \brief A class of an attribute that makes an attribute value of the current date and time
 *
 * The attribute generates current time stamp as a value. The type of the attribute value
 * is determined with time traits passed to the class template as a template parameter.
 * The time traits provided by the library use \c boost::posix_time::ptime as the time type.
 *
 * Time traits also determine the way time is acquired. There are two types of time traits
 * provided by th library: \c utc_time_traits and \c local_time_traits. The first returns UTC time,
 * the second returns local time.
 */
template< typename TimeTraitsT >
class basic_clock :
    public attribute
{
public:
    //! Time storage type
    typedef typename TimeTraitsT::time_type time_type;

private:
    //! Attribute value type
    typedef basic_attribute_value< time_type > result_value;

public:
    shared_ptr< attribute_value > get_value()
    {
        return boost::make_shared< result_value >(TimeTraitsT::get_clock());
    }
};

//! Attribute that returns current UTC time
typedef basic_clock< utc_time_traits > utc_clock;
//! Attribute that returns current local time
typedef basic_clock< local_time_traits > local_clock;

} // namespace attributes

} // namespace log

} // namespace boost

#endif // BOOST_LOG_ATTRIBUTES_CLOCK_HPP_INCLUDED_
