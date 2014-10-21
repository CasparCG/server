/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   date_time_types.hpp
 * \author Andrey Semashev
 * \date   13.03.2008
 *
 * The header contains definition of date and time-related types supported by the library by default.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_DATE_TIME_TYPES_HPP_INCLUDED_
#define BOOST_LOG_DATE_TIME_TYPES_HPP_INCLUDED_

#include <boost/compatibility/cpp_c_headers/ctime>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/joint_view.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/local_time/local_time_types.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/detail/prologue.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

/*!
 * An MPL-sequence of natively supported date and time types of attributes
 */
typedef mpl::vector<
    std::time_t,
    std::tm
>::type native_date_time_types;

/*!
 * An MPL-sequence of Boost date and time types of attributes
 */
typedef mpl::vector<
    posix_time::ptime,
    local_time::local_date_time
>::type boost_date_time_types;

/*!
 * An MPL-sequence with the complete list of the supported date and time types
 */
typedef mpl::joint_view<
    native_date_time_types,
    boost_date_time_types
>::type date_time_types;

/*!
 * An MPL-sequence of natively supported date types of attributes
 */
typedef native_date_time_types native_date_types;

/*!
 * An MPL-sequence of Boost date types of attributes
 */
typedef mpl::push_back<
    boost_date_time_types,
    gregorian::date
>::type boost_date_types;

/*!
 * An MPL-sequence with the complete list of the supported date types
 */
typedef mpl::joint_view<
    native_date_types,
    boost_date_types
>::type date_types;

/*!
 * An MPL-sequence of natively supported time types
 */
typedef native_date_time_types native_time_types;

//! An MPL-sequence of Boost time types
typedef boost_date_time_types boost_time_types;

/*!
 * An MPL-sequence with the complete list of the supported time types
 */
typedef date_time_types time_types;

/*!
 * An MPL-sequence of natively supported time duration types of attributes
 */
typedef mpl::vector<
    double // result of difftime
>::type native_time_duration_types;

/*!
 * An MPL-sequence of Boost time duration types of attributes
 */
typedef mpl::vector<
    posix_time::time_duration,
    gregorian::date_duration
>::type boost_time_duration_types;

/*!
 * An MPL-sequence with the complete list of the supported time duration types
 */
typedef mpl::joint_view<
    native_time_duration_types,
    boost_time_duration_types
>::type time_duration_types;

/*!
 * An MPL-sequence of Boost time duration types of attributes
 */
typedef mpl::vector<
    posix_time::time_period,
    local_time::local_time_period,
    gregorian::date_period
>::type boost_time_period_types;

/*!
 * An MPL-sequence with the complete list of the supported time period types
 */
typedef boost_time_period_types time_period_types;

} // namespace log

} // namespace boost

#endif // BOOST_LOG_DATE_TIME_TYPES_HPP_INCLUDED_
