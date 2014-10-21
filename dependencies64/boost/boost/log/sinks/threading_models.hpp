/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   sinks/threading_models.hpp
 * \author Andrey Semashev
 * \date   22.04.2007
 *
 * The header contains definition of threading models. These models are used to ensure
 * thread safety protocol between sink backends and frontents.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SINKS_THREADING_MODELS_HPP_INCLUDED_
#define BOOST_LOG_SINKS_THREADING_MODELS_HPP_INCLUDED_

#include <boost/type_traits/is_base_of.hpp>
#include <boost/log/detail/prologue.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace sinks {

//! Base tag for all other tags
struct threading_model_tag {};

#if !defined(BOOST_LOG_NO_THREADS)

//! The sink backend requires to be called in a single thread (IOW, no other threads EVER are allowed to write to the backend)
struct single_thread_tag : threading_model_tag {};
//! The sink backend expects pre-synchronized calls, all needed synchronization is implemented in the frontend (IOW, only one thread is writing to the backend concurrently, but is is possible for several threads to write sequentially)
struct frontend_synchronization_tag : single_thread_tag {};
//! The sink backend ensures all needed synchronization, it is capable to handle multithreaded calls
struct backend_synchronization_tag : frontend_synchronization_tag {};

#else // !defined(BOOST_LOG_NO_THREADS)

//  If multithreading is disabled, threading models become redundant
struct single_thread_tag : threading_model_tag {};
typedef single_thread_tag backend_synchronization_tag;
typedef single_thread_tag frontend_synchronization_tag;

#endif // !defined(BOOST_LOG_NO_THREADS)

//! A helper metafunction to check if a threading model is supported
template< typename TestedT, typename RequiredT >
struct is_model_supported :
    public is_base_of< RequiredT, TestedT >
{
};

} // namespace sinks

} // namespace log

} // namespace boost

#endif // BOOST_LOG_SINKS_THREADING_MODELS_HPP_INCLUDED_
