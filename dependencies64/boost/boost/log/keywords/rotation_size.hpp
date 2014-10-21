/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   keywords/rotation_size.hpp
 * \author Andrey Semashev
 * \date   14.03.2009
 *
 * The header contains the \c rotation_size keyword declaration.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_KEYWORDS_ROTATION_SIZE_HPP_INCLUDED_
#define BOOST_LOG_KEYWORDS_ROTATION_SIZE_HPP_INCLUDED_

#include <boost/parameter/keyword.hpp>
#include <boost/log/detail/prologue.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace keywords {

    //! The keyword allows to pass maximum log file size to the file sink
    BOOST_PARAMETER_KEYWORD(tag, rotation_size)

} // namespace keywords

} // namespace log

} // namespace boost

#endif // BOOST_LOG_KEYWORDS_ROTATION_SIZE_HPP_INCLUDED_
