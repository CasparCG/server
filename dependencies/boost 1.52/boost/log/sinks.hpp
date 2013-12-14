/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   sinks.hpp
 * \author Andrey Semashev
 * \date   13.07.2009
 *
 * This header includes other Boost.Log headers with all sinks.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SINKS_HPP_INCLUDED_
#define BOOST_LOG_SINKS_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>

#include <boost/log/sinks/sink.hpp>

#include <boost/log/sinks/unlocked_frontend.hpp>
#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/ordering_async_frontend.hpp>
#endif // !defined(BOOST_LOG_NO_THREADS)

#include <boost/log/sinks/syslog_backend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_multifile_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#ifdef BOOST_WINDOWS
#include <boost/log/sinks/debug_output_backend.hpp>
#include <boost/log/sinks/event_log_backend.hpp>
#endif // BOOST_WINDOWS

#endif // BOOST_LOG_SINKS_HPP_INCLUDED_
