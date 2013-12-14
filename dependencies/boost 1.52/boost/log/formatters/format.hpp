/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   formatters/format.hpp
 * \author Andrey Semashev
 * \date   16.03.2008
 *
 * The header contains implementation of a hook for Boost.Format-like formatters.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_FORMATTERS_FORMAT_HPP_INCLUDED_
#define BOOST_LOG_FORMATTERS_FORMAT_HPP_INCLUDED_

#include <vector>
#include <ostream>
#include <boost/format.hpp>
#include <boost/io/ios_state.hpp>
#include <boost/function/function2.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/formatters/basic_formatters.hpp>
#include <boost/log/formatters/chain.hpp>
#include <boost/log/formatters/wrappers.hpp>
#include <boost/log/detail/cleanup_scope_guard.hpp>
#include <boost/log/detail/attachable_sstream_buf.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace formatters {

/*!
 * \brief Formatter to output objects into a Boost.Format object
 *
 * The \c fmt_format class template is a hook that allows to construct formatters
 * with format strings. The formatter basically accumulates formatters of the formatting
 * expression and invokes them sequentially when called to format a log record. The results
 * of the aggregated formatters are fed to a Boost.Format formatter instance. The final result
 * of formatting is put into the resulting stream.
 */
template< typename CharT >
class fmt_format :
    public basic_formatter< CharT, fmt_format< CharT > >
{
private:
    //! Base type
    typedef basic_formatter< CharT, fmt_format< CharT > > base_type;

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! String type
    typedef typename base_type::string_type string_type;
    //! Stream type
    typedef typename base_type::ostream_type ostream_type;
    //! Boost.Format type
    typedef basic_format< char_type > format_type;
    //! Log record type
    typedef typename base_type::record_type record_type;

private:
    //! Streambuffer saver type
    typedef io::basic_ios_rdbuf_saver<
        typename ostream_type::char_type,
        typename ostream_type::traits_type
    > rdbuf_saver;
    //! Formatter function object type
    typedef function2< void, ostream_type&, record_type const& > formatter_type;
    //! Sequence of formatters
    typedef std::vector< formatter_type > formatters;
    //! Stream buffer type
    typedef boost::log::aux::basic_ostringstreambuf< char_type > streambuf_type;

private:
    //! Boost.Format object
    mutable format_type m_Format;
    //! Other formatters
    formatters m_Formatters;

    //! Formatting buffer
    mutable string_type m_Buffer;
    //! Stream buffer
    mutable streambuf_type m_StreamBuf;

public:
    /*!
     * Constructor with formatter initialization
     *
     * \param fmt Boost.Format formatter instance
     */
    explicit fmt_format(format_type const& fmt) : m_Format(fmt), m_StreamBuf(m_Buffer)
    {
    }
    /*!
     * Copy constructor
     */
    fmt_format(fmt_format const& that) :
        m_Format(that.m_Format),
        m_Formatters(that.m_Formatters),
        m_StreamBuf(m_Buffer)
    {
    }

    /*!
     * Formatting operator. Invokes all aggregated formatters, collects their formatting results
     * and composes them according to the format string passed on construction.
     *
     * \param strm A reference to the stream, where the final text of the logging record is composed
     * \param record A logging record
     */
    void operator() (ostream_type& strm, record_type const& record) const
    {
        boost::log::aux::cleanup_guard< format_type > cleanup1(m_Format);
        boost::log::aux::cleanup_guard< streambuf_type > cleanup2(m_StreamBuf);
        {
            rdbuf_saver cleanup3(strm, &m_StreamBuf);
            for (typename formatters::const_iterator it = m_Formatters.begin(), end = m_Formatters.end(); it != end; ++it)
            {
                boost::log::aux::cleanup_guard< string_type > cleanup4(m_Buffer);
                (*it)(strm, record);
                strm.flush();
                m_Format % m_Buffer;
            }
        }

        strm << m_Format.str();
    }

    /*!
     * Composition operator. Consumes the \a fmt formatter.
     *
     * \param fmt A Boost.Log formatter
     */
    template< typename T >
    fmt_format< char_type >& operator% (T const& fmt)
    {
        typedef typename wrap_if_not_formatter< char_type, T >::type result_type;
        m_Formatters.push_back(formatter_type(result_type(fmt)));
        return *this;
    }

private:
    //! Assignment prohibited
    fmt_format& operator= (fmt_format const& that);
};

#ifdef BOOST_LOG_USE_CHAR

/*!
 * Formatter generator. Constructs the formatter instance with the specified format string.
 *
 * \param fmt A format string. Must be a zero-terminated sequence of characters, must not be NULL.
 */
inline fmt_format< char > format(const char* fmt)
{
    typedef fmt_format< char >::format_type format_type;
    return fmt_format< char >(format_type(fmt));
}

/*!
 * Formatter generator. Constructs the formatter instance with the specified format string.
 *
 * \param fmt A format string
 */
inline fmt_format< char > format(std::basic_string< char > const& fmt)
{
    typedef fmt_format< char >::format_type format_type;
    return fmt_format< char >(format_type(fmt));
}

#endif

#ifdef BOOST_LOG_USE_WCHAR_T

/*!
 * Formatter generator. Constructs the formatter instance with the specified format string.
 *
 * \param fmt A format string. Must be a zero-terminated sequence of characters, must not be NULL.
 */
inline fmt_format< wchar_t > format(const wchar_t* fmt)
{
    typedef fmt_format< wchar_t >::format_type format_type;
    return fmt_format< wchar_t >(format_type(fmt));
}

/*!
 * Formatter generator. Constructs the formatter instance with the specified format string.
 *
 * \param fmt A format string
 */
inline fmt_format< wchar_t > format(std::basic_string< wchar_t > const& fmt)
{
    typedef fmt_format< wchar_t >::format_type format_type;
    return fmt_format< wchar_t >(format_type(fmt));
}

#endif

} // namespace formatters

} // namespace log

} // namespace boost

#endif // BOOST_LOG_FORMATTERS_BASIC_FORMATTERS_HPP_INCLUDED_
