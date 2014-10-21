/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   setup_prologue.hpp
 * \author Andrey Semashev
 * \date   14.09.2009
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html. In this file
 *         internal configuration macros are defined.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_DETAIL_SETUP_PROLOGUE_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_SETUP_PROLOGUE_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>

#if !defined(BOOST_LOG_SETUP_BUILDING_THE_LIB)

// Detect if we're dealing with dll
#   if defined(BOOST_LOG_SETUP_DYN_LINK) || defined(BOOST_ALL_DYN_LINK)
#        define BOOST_LOG_SETUP_DLL
#   endif

#   if defined(BOOST_HAS_DECLSPEC) && defined(BOOST_LOG_SETUP_DLL)
#       define BOOST_LOG_SETUP_EXPORT __declspec(dllimport)
#   else
#       define BOOST_LOG_SETUP_EXPORT
#   endif // defined(BOOST_HAS_DECLSPEC)
//
// Automatically link to the correct build variant where possible.
//
#   if !defined(BOOST_ALL_NO_LIB) && !defined(BOOST_LOG_SETUP_NO_LIB)
#       define BOOST_LIB_NAME boost_log_setup
#       if defined(BOOST_LOG_SETUP_DLL)
#           define BOOST_DYN_LINK
#       endif
#       include <boost/config/auto_link.hpp>
#   endif  // auto-linking disabled

#else // !defined(BOOST_LOG_SETUP_BUILDING_THE_LIB)

#   if defined(BOOST_HAS_DECLSPEC) && defined(BOOST_LOG_SETUP_DLL)
#       define BOOST_LOG_SETUP_EXPORT __declspec(dllexport)
#   elif defined(__GNUC__) && __GNUC__ >= 4 && (defined(linux) || defined(__linux) || defined(__linux__))
#       define BOOST_LOG_SETUP_EXPORT __attribute__((visibility("default")))
#   else
#       define BOOST_LOG_SETUP_EXPORT
#   endif

#endif // !defined(BOOST_LOG_SETUP_BUILDING_THE_LIB)

#endif // BOOST_LOG_DETAIL_SETUP_PROLOGUE_HPP_INCLUDED_
