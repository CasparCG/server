/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   c_decorator.hpp
 * \author Andrey Semashev
 * \date   07.06.2009
 *
 * The header contains implementation of C-style decorators.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_FORMATTERS_C_DECORATOR_HPP_INCLUDED_
#define BOOST_LOG_FORMATTERS_C_DECORATOR_HPP_INCLUDED_

#include <boost/limits.hpp>
#include <boost/cstdint.hpp>
#include <boost/io/ios_state.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/snprintf.hpp>
#include <boost/log/detail/cleanup_scope_guard.hpp>
#include <boost/log/detail/attachable_sstream_buf.hpp>
#include <boost/log/formatters/char_decorator.hpp>
#include <boost/log/formatters/basic_formatters.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace formatters {

namespace aux {

    template< typename >
    struct c_decorator_traits;

#ifdef BOOST_LOG_USE_CHAR
    template< >
    struct c_decorator_traits< char >
    {
        static boost::iterator_range< const char* const* > get_patterns()
        {
            static const char* const patterns[] =
            {
                "\\", "\a", "\b", "\f", "\n", "\r", "\t", "\v", "'", "\"", "?"
            };
            return boost::make_iterator_range(patterns);
        }
        static boost::iterator_range< const char* const* > get_replacements()
        {
            static const char* const replacements[] =
            {
                "\\\\", "\\a", "\\b", "\\f", "\\n", "\\r", "\\t", "\\v", "\\'", "\\\"", "\\?"
            };
            return boost::make_iterator_range(replacements);
        }
        template< unsigned int N >
        static std::size_t print_escaped(char (&buf)[N], char c)
        {
            return static_cast< std::size_t >(
                boost::log::aux::snprintf(buf, N, "\\x%0.2X", static_cast< unsigned int >(static_cast< uint8_t >(c))));
        }
    };
#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T
    template< >
    struct c_decorator_traits< wchar_t >
    {
        static boost::iterator_range< const wchar_t* const* > get_patterns()
        {
            static const wchar_t* const patterns[] =
            {
                L"\\", L"\a", L"\b", L"\f", L"\n", L"\r", L"\t", L"\v", L"'", L"\"", L"?"
            };
            return boost::make_iterator_range(patterns);
        }
        static boost::iterator_range< const wchar_t* const* > get_replacements()
        {
            static const wchar_t* const replacements[] =
            {
                L"\\\\", L"\\a", L"\\b", L"\\f", L"\\n", L"\\r", L"\\t", L"\\v", L"\\'", L"\\\"", L"\\?"
            };
            return boost::make_iterator_range(replacements);
        }
        template< unsigned int N >
        static std::size_t print_escaped(wchar_t (&buf)[N], wchar_t c)
        {
            const wchar_t* format;
            register unsigned int val;
            if (sizeof(wchar_t) == 1)
            {
                format = L"\\x%0.2X";
                val = static_cast< uint8_t >(c);
            }
            else if (sizeof(wchar_t) == 2)
            {
                format = L"\\x%0.4X";
                val = static_cast< uint16_t >(c);
            }
            else
            {
                format = L"\\x%0.8X";
                val = static_cast< uint32_t >(c);
            }

            return static_cast< std::size_t >(
                boost::log::aux::swprintf(buf, N, format, val));
        }
    };
#endif // BOOST_LOG_USE_WCHAR_T

    struct fmt_c_decorator_gen
    {
        template< typename FormatterT >
        fmt_char_decorator< FormatterT > operator[] (FormatterT const& fmt) const
        {
            typedef fmt_char_decorator< FormatterT > decorator;
            typedef c_decorator_traits< typename decorator::char_type > traits_t;
            return decorator(
                fmt,
                traits_t::get_patterns(),
                traits_t::get_replacements());
        }
    };

} // namespace aux

#ifndef BOOST_LOG_DOXYGEN_PASS

const aux::fmt_c_decorator_gen c_dec = {};

#else // BOOST_LOG_DOXYGEN_PASS

/*!
 * C-style decorator generator object. The decorator replaces characters with specific meaning in C
 * language with the corresponding escape sequences. The generator provides <tt>operator[]</tt> that
 * can be used to construct the actual decorator. For example:
 *
 * <code>
 * c_dec[ attr< std::string >("MyAttr") ]
 * </code>
 */
implementation_defined c_dec;

#endif // BOOST_LOG_DOXYGEN_PASS

template< typename FormatterT >
class fmt_c_ascii_decorator :
    public basic_formatter< typename FormatterT::char_type, fmt_c_ascii_decorator< FormatterT > >
{
private:
    //! Base type
    typedef basic_formatter< typename FormatterT::char_type, fmt_c_ascii_decorator< FormatterT > > base_type;

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! Decorated formatter type
    typedef FormatterT formatter_type;
    //! String type
    typedef typename base_type::string_type string_type;
    //! Stream type
    typedef typename base_type::ostream_type ostream_type;
    //! Log record type
    typedef typename base_type::record_type record_type;

private:
    //! Stream buffer type
    typedef boost::log::aux::basic_ostringstreambuf< char_type > streambuf_type;
    //! Streambuffer saver type
    typedef io::basic_ios_rdbuf_saver<
        typename ostream_type::char_type,
        typename ostream_type::traits_type
    > rdbuf_saver;
    //! Traits with the string constants
    typedef aux::c_decorator_traits< char_type > traits_t;

private:
    //! Decorated formatter
    fmt_char_decorator< formatter_type > m_Formatter;
    //! Formatted string
    mutable string_type m_Storage;
    //! Stream buffer to fill the storage
    mutable streambuf_type m_StreamBuf;

public:
    /*!
     * Initializing constructor
     */
    explicit fmt_c_ascii_decorator(formatter_type const& fmt) :
        m_Formatter(fmt, traits_t::get_patterns(), traits_t::get_replacements()),
        m_StreamBuf(m_Storage)
    {
    }
    /*!
     * Copy constructor
     */
    fmt_c_ascii_decorator(fmt_c_ascii_decorator const& that) :
        m_Formatter(that.m_Formatter),
        m_StreamBuf(m_Storage)
    {
    }

    /*!
     * Formatting operator. Invokes the decorated formatter, then sequentially applies all
     * decorations to the output. The resulting string is the output of the decorator.
     *
     * \param strm A reference to the stream, where the final text of the logging record is composed
     * \param record A logging record
     */
    void operator() (ostream_type& strm, record_type const& record) const
    {
        typedef aux::c_decorator_traits< char_type > traits_t;
        boost::log::aux::cleanup_guard< string_type > cleanup1(m_Storage);
        boost::log::aux::cleanup_guard< streambuf_type > cleanup2(m_StreamBuf);

        // Perform formatting
        rdbuf_saver cleanup3(strm, &m_StreamBuf);
        m_Formatter(strm, record);
        strm.flush();

        // Apply decorations
        typedef typename string_type::iterator string_iterator;
        for (string_iterator it = m_Storage.begin(), end = m_Storage.end(); it != end; ++it)
        {
            char_type c = *it;
            if (c < 0x20 || c > 0x7e)
            {
                char_type buf[(std::numeric_limits< char_type >::digits + 3) / 4 + 3];
                std::size_t n = traits_t::print_escaped(buf, c);
                std::size_t pos = it - m_Storage.begin();
                m_Storage.replace(pos, 1, buf, n);
                it = m_Storage.begin() + n - 1;
                end = m_Storage.end();
            }
        }

        // Put the final string into the stream
        strm.write(m_Storage.data(), static_cast< std::streamsize >(m_Storage.size()));
    }

private:
    //! Assignment is closed
    fmt_c_ascii_decorator& operator= (fmt_c_ascii_decorator const&);
};

namespace aux {

    struct fmt_c_ascii_decorator_gen
    {
        template< typename FormatterT >
        fmt_c_ascii_decorator< FormatterT > operator[] (FormatterT const& fmt) const
        {
            typedef fmt_c_ascii_decorator< FormatterT > decorator;
            return decorator(fmt);
        }
    };

} // namespace aux

#ifndef BOOST_LOG_DOXYGEN_PASS

const aux::fmt_c_ascii_decorator_gen c_ascii_dec = {};

#else // BOOST_LOG_DOXYGEN_PASS

/*!
 * C-style decorator generator object. Acts similarly to \c c_dec, except that \c c_ascii_dec also
 * converts all non-ASCII and non-printable ASCII characters, except for space character, into
 * C-style hexadecimal escape sequences. The generator provides <tt>operator[]</tt> that
 * can be used to construct the actual decorator. For example:
 *
 * <code>
 * c_ascii_dec[ attr< std::string >("MyAttr") ]
 * </code>
 */
implementation_defined c_ascii_dec;

#endif // BOOST_LOG_DOXYGEN_PASS

} // namespace formatters

} // namespace log

} // namespace boost

#endif // BOOST_LOG_FORMATTERS_C_DECORATOR_HPP_INCLUDED_
