/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   code_conversion.hpp
 * \author Andrey Semashev
 * \date   08.11.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_DETAIL_CODE_CONVERSION_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_CODE_CONVERSION_HPP_INCLUDED_

#include <locale>
#include <string>
#include <ostream>
#include <streambuf>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/attachable_sstream_buf.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

//! The stream buffer that converts wide character to narrow characters or vise versa on writing
template<
    typename CharT,
    typename TraitsT = std::char_traits< CharT >
>
class BOOST_LOG_EXPORT converting_ostringstreambuf :
    public std::basic_streambuf< CharT, TraitsT >
{
    //! Base type
    typedef std::basic_streambuf< CharT, TraitsT > base_type;

    //! Buffers size
    enum { buffer_size = 256 };

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! Traits type
    typedef typename base_type::traits_type traits_type;
    //! Int type
    typedef typename base_type::int_type int_type;
    //! Target character type
    typedef typename mpl::if_<
        is_same< char_type, char >,
        wchar_t,
        char
    >::type target_char_type;
    //! String type
    typedef std::basic_string< target_char_type > string_type;
    //! Code conversion facet
    typedef std::codecvt< wchar_t, char, std::mbstate_t > facet_type;

private:
    //! A reference to the string that will be filled
    string_type& m_Storage;
    //! Character conversion state
    std::mbstate_t m_ConversionState;
    //! A buffer used to temporarily store output
    char_type m_Buffer[buffer_size];

public:
    //! Constructor
    explicit converting_ostringstreambuf(string_type& storage);
    //! Destructor
    ~converting_ostringstreambuf();

    //! Clears the buffer to the initial state
    void clear();

protected:
    //! Puts all buffered data to the string
    int sync();
    //! Puts an unbuffered character to the string
    int_type overflow(int_type c);
    //! Puts a character sequence to the string
    std::streamsize xsputn(const char_type* s, std::streamsize n);

private:
    //! The function writes the specified characters to the storage
    void write(const char_type*& pBase, const char_type* pPtr);
};


//! The function converts one string to the character type of another
inline void code_convert(std::string const& str1, std::string& str2)
{
    str2 = str1;
}
//! The function converts one string to the character type of another
inline void code_convert(std::wstring const& str1, std::string& str2)
{
    converting_ostringstreambuf< wchar_t > buf(str2);
    buf.sputn(str1.data(), static_cast< std::streamsize >(str1.size()));
    buf.pubsync();
}
//! The function converts one string to the character type of another
inline void code_convert(std::wstring const& str1, std::wstring& str2)
{
    str2 = str1;
}
//! The function converts one string to the character type of another
inline void code_convert(std::string const& str1, std::wstring& str2)
{
    converting_ostringstreambuf< char > buf(str2);
    buf.sputn(str1.data(), static_cast< std::streamsize >(str1.size()));
    buf.pubsync();
}

//! The function converts the passed string to the narrow-character encoding
inline std::string const& to_narrow(std::string const& str)
{
    return str;
}

//! The function converts the passed string to the narrow-character encoding
inline std::string to_narrow(std::wstring const& str)
{
    std::string res;
    aux::code_convert(str, res);
    return res;
}

//! The function converts the passed string to the wide-character encoding
inline std::wstring const& to_wide(std::wstring const& str)
{
    return str;
}

//! The function converts the passed string to the wide-character encoding
inline std::wstring to_wide(std::string const& str)
{
    std::wstring res;
    aux::code_convert(str, res);
    return res;
}

} // namespace aux

} // namespace log

} // namespace boost

#endif // BOOST_LOG_DETAIL_CODE_CONVERSION_HPP_INCLUDED_
