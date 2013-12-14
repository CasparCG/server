/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   formatters.hpp
 * \author Andrey Semashev
 * \date   13.07.2009
 *
 * This header includes other Boost.Log headers with all formatters.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_FORMATTERS_HPP_INCLUDED_
#define BOOST_LOG_FORMATTERS_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>

#include <boost/log/formatters/basic_formatters.hpp>
#include <boost/log/formatters/attr.hpp>
#include <boost/log/formatters/c_decorator.hpp>
#include <boost/log/formatters/chain.hpp>
#include <boost/log/formatters/char_decorator.hpp>
#include <boost/log/formatters/csv_decorator.hpp>
#include <boost/log/formatters/date_time.hpp>
#include <boost/log/formatters/format.hpp>
#include <boost/log/formatters/if.hpp>
#include <boost/log/formatters/message.hpp>
#include <boost/log/formatters/named_scope.hpp>
#include <boost/log/formatters/stream.hpp>
#include <boost/log/formatters/wrappers.hpp>
#include <boost/log/formatters/xml_decorator.hpp>

#endif // BOOST_LOG_FORMATTERS_HPP_INCLUDED_
