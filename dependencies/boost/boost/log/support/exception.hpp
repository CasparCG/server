/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   support/exception.hpp
 * \author Andrey Semashev
 * \date   18.07.2009
 *
 * This header enables Boost.Exception support for Boost.Log.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SUPPORT_EXCEPTION_HPP_INCLUDED_
#define BOOST_LOG_SUPPORT_EXCEPTION_HPP_INCLUDED_

#include <boost/exception/info.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/attributes/named_scope.hpp>
#if defined(BOOST_LOG_USE_WCHAR_T)
#include <ostream>
#include <boost/log/detail/code_conversion.hpp>
#endif // defined(BOOST_LOG_USE_WCHAR_T)

namespace boost {

namespace BOOST_LOG_NAMESPACE {

struct current_scope_info_tag;

/*!
 * The function returns an error information object that contains current stack of scopes.
 * This information can then be attached to an exception and extracted at the catch site.
 * The extracted scope list won't be affected by any scope changes that may happen during
 * the exception propagation.
 *
 * \note See the \c basic_named_scope attribute documentation on how to maintain scope list.
 */
template< typename CharT >
inline error_info<
    current_scope_info_tag,
    attributes::basic_named_scope_list< CharT >
> current_scope()
{
    typedef error_info<
        current_scope_info_tag,
        attributes::basic_named_scope_list< CharT >
    > info_t;
    return info_t(attributes::basic_named_scope< CharT >::get_scopes());
}

#if defined(BOOST_LOG_USE_CHAR)

//! Convenience typedef for narrow-character logging
typedef error_info<
    current_scope_info_tag,
    attributes::basic_named_scope_list< char >
> current_scope_info;

/*!
 * Convenience forwarder for narrow-character logging.
 */
inline current_scope_info current_scope()
{
    return current_scope< char >();
}

#endif // defined(BOOST_LOG_USE_CHAR)

#if defined(BOOST_LOG_USE_WCHAR_T)

//! Convenience typedef for wide-character logging
typedef error_info<
    current_scope_info_tag,
    attributes::basic_named_scope_list< wchar_t >
> wcurrent_scope_info;

/*!
 * Convenience forwarder for wide-character logging.
 */
inline wcurrent_scope_info wcurrent_scope()
{
    return current_scope< wchar_t >();
}

namespace attributes {

/*!
 * An additional streaming operator to allow to compose diagnostic information
 * from wide-character scope lists.
 */
inline std::ostream& operator<< (std::ostream& strm, basic_named_scope_list< wchar_t > const& scopes)
{
    std::string buf;

    {
        boost::log::aux::converting_ostringstreambuf< wchar_t > stream_buf(buf);
        std::wostream wstrm(&stream_buf);
        wstrm << scopes;
        wstrm.flush();
        if (!wstrm.good())
            buf = "[current scope]";
    }

    strm.write(buf.data(), static_cast< std::streamsize >(buf.size()));
    return strm;
}

} // namespace attributes

#endif // defined(BOOST_LOG_USE_WCHAR_T)

} // namespace log

} // namespace boost

#endif // BOOST_LOG_SUPPORT_EXCEPTION_HPP_INCLUDED_
