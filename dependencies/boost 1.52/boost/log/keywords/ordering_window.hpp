/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   keywords/ordering_window.hpp
 * \author Andrey Semashev
 * \date   23.08.2009
 *
 * The header contains the \c ordering_window keyword declaration.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_KEYWORDS_ORDERING_WINDOW_HPP_INCLUDED_
#define BOOST_LOG_KEYWORDS_ORDERING_WINDOW_HPP_INCLUDED_

#include <boost/parameter/keyword.hpp>
#include <boost/log/detail/prologue.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace keywords {

    //! The keyword allows to pass the ordering window to sink frontends
    BOOST_PARAMETER_KEYWORD(tag, ordering_window)

} // namespace keywords

} // namespace log

} // namespace boost

#endif // BOOST_LOG_KEYWORDS_ORDERING_WINDOW_HPP_INCLUDED_
