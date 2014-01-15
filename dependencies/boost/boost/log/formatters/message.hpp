/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   message.hpp
 * \author Andrey Semashev
 * \date   22.04.2007
 *
 * The header contains implementation of a message placeholder in formatters.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_FORMATTERS_MESSAGE_HPP_INCLUDED_
#define BOOST_LOG_FORMATTERS_MESSAGE_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>
#include <boost/log/formatters/basic_formatters.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace formatters {

/*!
 * \brief Message formatter class
 *
 * The formatter simply puts the log record message to the resulting stream
 */
template< typename CharT >
class fmt_message :
    public basic_formatter< CharT, fmt_message< CharT > >
{
private:
    //! Base type
    typedef basic_formatter< CharT, fmt_message< CharT > > base_type;

public:
    //! Stream type
    typedef typename base_type::ostream_type ostream_type;
    //! Log record type
    typedef typename base_type::record_type record_type;

public:
    /*!
     * Formatting operator. Puts message text from the log record \a record to the \a strm stream.
     *
     * \param strm A reference to the stream, where the final text of the logging record is composed
     * \param record A logging record
     */
    void operator() (ostream_type& strm, record_type const& record) const
    {
        strm << record.message();
    }
};

/*!
 * Formatter generator. Constructs the \c fmt_message formatter object.
 */
template< typename CharT >
inline fmt_message< CharT > message()
{
    return fmt_message< CharT >();
}

#ifdef BOOST_LOG_USE_CHAR

/*!
 * Formatter generator. Constructs the \c fmt_message formatter object for narrow-character logging.
 */
inline fmt_message< char > message()
{
    return fmt_message< char >();
}

#endif

#ifdef BOOST_LOG_USE_WCHAR_T

/*!
 * Formatter generator. Constructs the \c fmt_message formatter object for wide-character logging.
 */
inline fmt_message< wchar_t > wmessage()
{
    return fmt_message< wchar_t >();
}

#endif

} // namespace formatters

} // namespace log

} // namespace boost

#endif // BOOST_LOG_FORMATTERS_MESSAGE_HPP_INCLUDED_
