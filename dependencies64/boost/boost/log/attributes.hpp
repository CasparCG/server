/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attributes.hpp
 * \author Andrey Semashev
 * \date   13.07.2009
 *
 * This header includes other Boost.Log headers with all attributes.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_ATTRIBUTES_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>

#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/clock.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/counter.hpp>
#include <boost/log/attributes/functor.hpp>
#include <boost/log/attributes/mutable_constant.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/attributes/timer.hpp>
#include <boost/log/attributes/current_process_id.hpp>
#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/log/attributes/current_thread_id.hpp>
#endif

#endif // BOOST_LOG_ATTRIBUTES_HPP_INCLUDED_
