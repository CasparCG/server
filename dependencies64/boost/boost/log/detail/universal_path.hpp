/*
 *          Copyright Andrey Semashev 2007 - 2011.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   universal_path.hpp
 * \author Andrey Semashev
 * \date   27.06.2009
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_DETAIL_UNIVERSAL_PATH_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_UNIVERSAL_PATH_HPP_INCLUDED_

#include <string>
#include <boost/filesystem/path.hpp>
#include <boost/log/detail/prologue.hpp>
#if !defined(BOOST_FILESYSTEM_VERSION) || BOOST_FILESYSTEM_VERSION <= 2
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/log/detail/code_conversion.hpp>
#endif

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

#if defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3

    // In Boost.Filesystem v3 path type is already unified for different character types
    typedef filesystem::path universal_path;

    inline universal_path const& to_universal_path(filesystem::path const& p)
    {
        return p;
    }

    template< typename CharT, typename TraitsT, typename AllocatorT >
    inline universal_path to_universal_path(std::basic_string< CharT, TraitsT, AllocatorT > const& p)
    {
        return universal_path(p);
    }

    template< typename CharT >
    inline universal_path to_universal_path(const CharT* p)
    {
        return universal_path(p);
    }

#else // defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3

    // Compatibility with Boost.Filesystem v2

    // Determine which path type is native:
    // 1. If no wide paths are supported then it's path
    // 2. If the native API only supports narrow paths, it's path, again.
    // 3. Otherwise, it's wpath.
    //
    // NOTE: Cygwin is considered to have narrow native paths, although it actually supports UTF-8
#if defined(BOOST_FILESYSTEM_NARROW_ONLY) || defined(__CYGWIN__)
    typedef filesystem::path universal_path;
#else
    typedef mpl::if_<
        is_same< filesystem::wpath::external_string_type, std::string >,
        filesystem::path,
        filesystem::wpath
    >::type universal_path;
#endif

    template< typename StringT, typename TraitsT >
    inline universal_path to_universal_path(filesystem::basic_path< StringT, TraitsT > const& p)
    {
        universal_path::string_type s;
        boost::log::aux::code_convert(p.string(), s);
        return universal_path(s);
    }

    template< typename CharT, typename TraitsT, typename AllocatorT >
    inline universal_path to_universal_path(std::basic_string< CharT, TraitsT, AllocatorT > const& p)
    {
        universal_path::string_type s;
        boost::log::aux::code_convert(p, s);
        return universal_path(s);
    }

    template< typename CharT >
    inline universal_path to_universal_path(const CharT* p)
    {
        return aux::to_universal_path(std::basic_string< CharT >(p));
    }

#endif // defined(BOOST_FILESYSTEM_VERSION) && BOOST_FILESYSTEM_VERSION >= 3

} // namespace aux

} // namespace log

} // namespace boost

#endif // BOOST_LOG_DETAIL_UNIVERSAL_PATH_HPP_INCLUDED_
