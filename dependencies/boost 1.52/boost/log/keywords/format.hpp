/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   keywords/format.hpp
 * \author Andrey Semashev
 * \date   14.03.2009
 *
 * The header contains the \c format keyword declaration.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_KEYWORDS_FORMAT_HPP_INCLUDED_
#define BOOST_LOG_KEYWORDS_FORMAT_HPP_INCLUDED_

#include <boost/parameter/keyword.hpp>
#include <boost/log/detail/prologue.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace keywords {

    //! The keyword for passing format specifiers to functions
    BOOST_PARAMETER_KEYWORD(tag, format)

} // namespace keywords

} // namespace log

} // namespace boost

#endif // BOOST_LOG_KEYWORDS_FORMAT_HPP_INCLUDED_
