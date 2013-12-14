/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   no_unused_warnings.hpp
 * \author Andrey Semashev
 * \date   10.05.2008
 *
 * The header contains definition of a macro to suppress compiler warnings about unused variables.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_UTILITY_NO_UNUSED_WARNINGS_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_NO_UNUSED_WARNINGS_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

template< typename T >
inline void no_unused_warnings(T const&) {}

} // namespace aux

} // namespace log

} // namespace boost

//! The macro suppresses compiler warnings for \c var being unused
#define BOOST_LOG_NO_UNUSED_WARNINGS(var) ::boost::log::aux::no_unused_warnings(var)

#endif // BOOST_LOG_UTILITY_NO_UNUSED_WARNINGS_HPP_INCLUDED_
