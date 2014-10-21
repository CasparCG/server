/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   slim_string.hpp
 * \author Andrey Semashev
 * \date   08.09.2007
 *
 * The header contains implementation of a lightweight read-only STL-string analogue.
 * Slim strings are used to contain attribute names.
 */

#if defined(_MSC_VER) && _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_UTILITY_SLIM_STRING_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_SLIM_STRING_HPP_INCLUDED_

#include <iosfwd>
#include <string>
#include <memory>
#include <iterator>
#include <algorithm>
#include <boost/compatibility/cpp_c_headers/cstddef>
#include <boost/log/detail/prologue.hpp>

#ifdef _MSC_VER
#pragma warning(push)
 // non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

/*!
 * \brief A slim string class with shared data and read-only access
 *
 * The slim string class is used within the library to optimize attribute names storage
 * and copying. The \c basic_slim_string class template provides interface similar
 * to <tt>std::basic_string</tt> with regard to read-only capabilities.
 */
template<
    typename CharT,
    typename TraitsT = std::char_traits< CharT >
>
class basic_slim_string :
    private std::allocator< char >
{
public:
    //! Character type
    typedef CharT char_type;
    //! Character traits type
    typedef TraitsT traits_type;
    //! Underlying allocator type
    typedef std::allocator< char_type > allocator_type;
    //! Compatible string type
    typedef std::basic_string< char_type, traits_type > string_type;

    //  Standard typedefs
    typedef typename allocator_type::value_type value_type;
    typedef typename allocator_type::pointer pointer;
    typedef typename allocator_type::const_pointer const_pointer;
    typedef typename allocator_type::reference reference;
    typedef typename allocator_type::const_reference const_reference;
    typedef typename allocator_type::size_type size_type;
    typedef typename allocator_type::difference_type difference_type;
    typedef const_pointer const_iterator;
    typedef std::reverse_iterator< const_iterator > const_reverse_iterator;

    enum { npos = ~0U };

private:
    //! Internal allocator
    typedef std::allocator< char > internal_allocator_type;

    //! Internal implementation class
    struct implementation;
    friend struct implementation;
    //! Pointer to the shared implementation
    implementation* m_pImpl;

public:
    /*!
     * Default constructor
     *
     * \post <tt>empty() == true</tt>
     */
    BOOST_LOG_EXPORT basic_slim_string();
    /*!
     * Copy constructor
     *
     * \post <tt>*this == that</tt>
     * \param that Source string to copy from
     */
    BOOST_LOG_EXPORT basic_slim_string(basic_slim_string const& that);

    /*!
     * Constructs a slim string from an STL string.
     *
     * \post <tt>*this == that</tt>
     * \param that Source string to copy from
     */
    BOOST_LOG_EXPORT explicit basic_slim_string(string_type const& that);
    /*!
     * Constructs a slim string from an STL string part.
     *
     * \post <tt>*this == that.substr(pos, n)</tt>
     * \param s Source string to copy from
     * \param pos Starting position to begin copying from
     * \param n Number of symbols to copy
     */
    BOOST_LOG_EXPORT basic_slim_string(string_type const& s, size_type pos, size_type n = npos);
    /*!
     * Constructs a slim string from a slim string part.
     *
     * \post <tt>*this == that.substr(pos, n)</tt>
     * \param that Source string to copy from
     * \param pos Starting position to begin copying from
     * \param n Number of symbols to copy
     */
    BOOST_LOG_EXPORT basic_slim_string(basic_slim_string const& that, size_type pos, size_type n = npos);
    /*!
     * Constructs a slim string from a C-style string
     *
     * \post <tt>*this == string_type(s)</tt>
     * \param s Source string. Must be a zero-terminated sequence of characters, must not be NULL.
     */
    BOOST_LOG_EXPORT basic_slim_string(const_pointer s);
    /*!
     * Constructs a slim string from a C-style string
     *
     * \post <tt>*this == string_type(s, n)</tt>
     * \param s Source string. Must be a sequence of characters, must not be NULL.
     * \param n Number of characters in sequence \a s
     */
    BOOST_LOG_EXPORT basic_slim_string(const_pointer s, size_type n);
    /*!
     * Constructs a slim string from a number of characters
     *
     * \post <tt>*this == string_type(n, c)</tt>
     * \param n Number of characters
     * \param c Filler character
     */
    BOOST_LOG_EXPORT basic_slim_string(size_type n, char_type c);

    /*!
     * Destructor
     */
    BOOST_LOG_EXPORT ~basic_slim_string();

    /*!
     * Assignment operator
     *
     * \post <tt>*this == that</tt>
     * \param that Source string to copy from
     */
    basic_slim_string& operator= (basic_slim_string const& that)
    {
        if (this != &that)
        {
            basic_slim_string tmp(that);
            swap(tmp);
        }
        return *this;
    }
    /*!
     * Assignment operator
     *
     * \post <tt>*this == basic_slim_string(that)</tt>
     * \param that Source string to copy from
     */
    template< typename T >
    basic_slim_string& operator= (T const& that)
    {
        basic_slim_string tmp(that);
        swap(tmp);
        return *this;
    }

    /*!
     * Indexing operator
     *
     * \pre \a n <tt>&lt; size()</tt>
     * \param n Index of the character requested
     * \return Constant reference to the requested character
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    const_reference
#else
    CharT const&
#endif
    operator[] (size_type n) const;

    /*!
     * Lexicographical comparison operator (equality).
     * \param that Comparand
     */
    template< typename T >
    bool operator== (T const& that) const { return (compare(that) == 0); }
    /*!
     * Lexicographical comparison operator (inequality).
     * \param that Comparand
     */
    template< typename T >
    bool operator!= (T const& that) const { return (compare(that) != 0); }
    /*!
     * Lexicographical comparison operator (less).
     * \param that Comparand
     */
    template< typename T >
    bool operator< (T const& that) const { return (compare(that) < 0); }
    /*!
     * Lexicographical comparison operator (greater).
     * \param that Comparand
     */
    template< typename T >
    bool operator> (T const& that) const { return (compare(that) > 0); }
    /*!
     * Lexicographical comparison operator (less or equal).
     * \param that Comparand
     */
    template< typename T >
    bool operator<= (T const& that) const { return (compare(that) <= 0); }
    /*!
     * Lexicographical comparison operator (greater or equal).
     * \param that Comparand
     */
    template< typename T >
    bool operator>= (T const& that) const { return (compare(that) >= 0); }

    /*!
     * Checked accessor to a character within the string
     *
     * \param n Character index
     * \return Constant reference to the requested character
     *
     * \b Throws: An <tt>std::exception</tt>-based exception to indicate that \a n is out of range.
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    const_reference
#else
    CharT const&
#endif
    at(size_type n) const;
    /*!
     * \return A constant pointer to the beginning of the string
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    const_pointer
#else
    CharT const*
#endif
    data() const;
    /*!
     * \return A constant pointer to the beginning of the string
     */
    const_pointer c_str() const { return data(); }
    /*!
     * \return The stored string length
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    size() const;
    /*!
     * \return The stored string length
     */
    size_type length() const { return size(); }
    /*!
     * \return \c true if the stored string is empty, \c false otherwise
     */
    bool empty() const { return (size() == 0); }
    /*!
     * \return <tt>size()</tt>
     */
    size_type capacity() const { return size(); }
    /*!
     * \return <tt>size()</tt>
     */
    size_type max_size() const { return size(); }

    /*!
     * \return Constant iterator to the beginning of the string
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    const_iterator
#else
    CharT const*
#endif
    begin() const;
    /*!
     * \return Constant iterator to the end of the string
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    const_iterator
#else
    CharT const*
#endif
    end() const;
    /*!
     * \return Constant reverse iterator to the end of the string
     */
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
    /*!
     * \return Constant reverse iterator to the beginning of the string
     */
    const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

    /*!
     * Copies a part of the stored string into external buffer
     *
     * \param s Pointer to the beginning of external buffer. Must not be NULL.
     *          The buffer must have enough capacity to accommodate up to \a n characters.
     * \param n Maximum number of characters to copy
     * \param pos Starting position in the string to begin copying from
     * \return The number of copied symbols
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    copy(pointer s, size_type n, size_type pos = 0) const;
    /*!
     * Creates another slim string containing a part of the stored string
     *
     * \param pos Starting position of the substring
     * \param n Length of the substring
     * \return The constructed substring
     */
    basic_slim_string substr(size_type pos = 0, size_type n = npos) const
    {
        return basic_slim_string(*this, pos, n);
    }

    /*!
     * Swaps two strings
     */
    void swap(basic_slim_string& that) { std::swap(m_pImpl, that.m_pImpl); }

    /*!
     * Searches the string for a substring
     *
     * \param that The sought substring
     * \param pos Search starting position
     * \return Starting position of the found substring within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find(basic_slim_string const& that, size_type pos = 0) const;
    /*!
     * Searches the string for a substring
     *
     * \param s The sought substring
     * \param pos Search starting position
     * \return Starting position of the found substring within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find(string_type const& s, size_type pos = 0) const;
    /*!
     * Searches the string for a substring
     *
     * \param s The sought substring. Must point to a zero-terminated sequence of characters, must not be NULL.
     * \param pos Search starting position
     * \return Starting position of the found substring within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find(const_pointer s, size_type pos = 0) const;
    /*!
     * Searches the string for a substring
     *
     * \param s The sought substring. Must point to a sequence of characters, must not be NULL.
     * \param pos Search starting position
     * \param n Number of characters in the substring \a s
     * \return Starting position of the found substring within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find(const_pointer s, size_type pos, size_type n) const;
    /*!
     * Searches the string for a character
     *
     * \param c The sought character
     * \param pos Search starting position
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find(char_type c, size_type pos = 0) const;

    /*!
     * Searches the string for a substring in the reverse direction
     *
     * \param that The sought substring
     * \param pos Search starting position
     * \return Starting position of the found substring within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    rfind(basic_slim_string const& that, size_type pos = npos) const;
    /*!
     * Searches the string for a substring in the reverse direction
     *
     * \param s The sought substring
     * \param pos Search starting position
     * \return Starting position of the found substring within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    rfind(string_type const& s, size_type pos = npos) const;
    /*!
     * Searches the string for a substring in the reverse direction
     *
     * \param s The sought substring. Must point to a zero-terminated sequence of characters, must not be NULL.
     * \param pos Search starting position
     * \return Starting position of the found substring within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    rfind(const_pointer s, size_type pos = npos) const;
    /*!
     * Searches the string for a substring in the reverse direction
     *
     * \param s The sought substring. Must point to a sequence of characters, must not be NULL.
     * \param pos Search starting position
     * \param n Number of characters in the substring \a s
     * \return Starting position of the found substring within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    rfind(const_pointer s, size_type pos, size_type n) const;
    /*!
     * Searches the string for a character in the reverse direction
     *
     * \param c The sought character
     * \param pos Search starting position
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    rfind(char_type c, size_type pos = npos) const;

    /*!
     * Searches the string for one of the specified characters
     *
     * \param that The set of characters being sought
     * \param pos Search starting position
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_first_of(basic_slim_string const& that, size_type pos = 0) const;
    /*!
     * Searches the string for one of the specified characters
     *
     * \param s The set of characters being sought
     * \param pos Search starting position
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_first_of(string_type const& s, size_type pos = 0) const;
    /*!
     * Searches the string for one of the specified characters
     *
     * \param s The set of characters being sought. Must point to a zero-terminated sequence of characters, must not be NULL.
     * \param pos Search starting position
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_first_of(const_pointer s, size_type pos = 0) const;
    /*!
     * Searches the string for one of the specified characters
     *
     * \param s The set of characters being sought. Must point to a sequence of characters, must not be NULL.
     * \param pos Search starting position
     * \param n Number of characters in the sequence \a s
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_first_of(const_pointer s, size_type pos, size_type n) const;
    /*!
     * Searches the string for a character
     *
     * \param c The sought character
     * \param pos Search starting position
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_first_of(char_type c, size_type pos = 0) const;

    /*!
     * Searches the string for one of the specified characters in the reverse direction
     *
     * \param that The set of characters being sought
     * \param pos Search starting position
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_last_of(basic_slim_string const& that, size_type pos = npos) const;
    /*!
     * Searches the string for one of the specified characters in the reverse direction
     *
     * \param s The set of characters being sought
     * \param pos Search starting position
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_last_of(string_type const& s, size_type pos = npos) const;
    /*!
     * Searches the string for one of the specified characters in the reverse direction
     *
     * \param s The set of characters being sought. Must point to a zero-terminated sequence of characters, must not be NULL.
     * \param pos Search starting position
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_last_of(const_pointer s, size_type pos = npos) const;
    /*!
     * Searches the string for one of the specified characters in the reverse direction
     *
     * \param s The set of characters being sought. Must point to a sequence of characters, must not be NULL.
     * \param pos Search starting position
     * \param n Number of characters in the sequence \a s
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_last_of(const_pointer s, size_type pos, size_type n) const;
    /*!
     * Searches the string for a character in the reverse direction in the reverse direction
     *
     * \param c The sought character
     * \param pos Search starting position
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_last_of(char_type c, size_type pos = npos) const;

    /*!
     * Searches the string for one of the characters, other than specified in the sequence
     *
     * \param that The set of characters, that are not being sought
     * \param pos Search starting position
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_first_not_of(basic_slim_string const& that, size_type pos = 0) const;
    /*!
     * Searches the string for one of the characters, other than specified in the sequence
     *
     * \param s The set of characters, that are not being sought
     * \param pos Search starting position
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_first_not_of(string_type const& s, size_type pos = 0) const;
    /*!
     * Searches the string for one of the characters, other than specified in the sequence
     *
     * \param s The set of characters, that are not being sought. Must point to a zero-terminated sequence of characters, must not be NULL.
     * \param pos Search starting position
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_first_not_of(const_pointer s, size_type pos = 0) const;
    /*!
     * Searches the string for one of the characters, other than specified in the sequence
     *
     * \param s The set of characters, that are not being sought. Must point to a sequence of characters, must not be NULL.
     * \param pos Search starting position
     * \param n Number of characters in the sequence \a s
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_first_not_of(const_pointer s, size_type pos, size_type n) const;
    /*!
     * Searches the string for one of the characters, other than specified
     *
     * \param c The character, that is not being sought
     * \param pos Search starting position
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_first_not_of(char_type c, size_type pos = 0) const;

    /*!
     * Searches the string for one of the characters, other than specified in the sequence, in the reverse direction
     *
     * \param that The set of characters, that are not being sought
     * \param pos Search starting position
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_last_not_of(basic_slim_string const& that, size_type pos = npos) const;
    /*!
     * Searches the string for one of the characters, other than specified in the sequence, in the reverse direction
     *
     * \param s The set of characters, that are not being sought
     * \param pos Search starting position
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_last_not_of(string_type const& s, size_type pos = npos) const;
    /*!
     * Searches the string for one of the characters, other than specified in the sequence, in the reverse direction
     *
     * \param s The set of characters, that are not being sought. Must point to a zero-terminated sequence of characters, must not be NULL.
     * \param pos Search starting position
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_last_not_of(const_pointer s, size_type pos = npos) const;
    /*!
     * Searches the string for one of the characters, other than specified in the sequence, in the reverse direction
     *
     * \param s The set of characters, that are not being sought. Must point to a sequence of characters, must not be NULL.
     * \param pos Search starting position
     * \param n Number of characters in the sequence \a s
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_last_not_of(const_pointer s, size_type pos, size_type n) const;
    /*!
     * Searches the string for one of the characters, other than specified, in the reverse direction
     *
     * \param c The character, that is not being sought
     * \param pos Search starting position
     * \return Position of the found character within the string, if lookup was successful, \c npos otherwise
     */
    BOOST_LOG_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    size_type
#else
    std::size_t
#endif
    find_last_not_of(char_type c, size_type pos = npos) const;

    /*!
     * Lexicographically compares the argument string to this string
     *
     * \param that Comparand
     * \return Zero if the comparand equals this string, a negative value if this string is less than the comparand,
     *         a positive value if this string is greater than the comparand.
     */
    BOOST_LOG_EXPORT int compare(basic_slim_string const& that) const;
    /*!
     * Lexicographically compares the argument string to this string
     *
     * \param s Comparand
     * \return Zero if the comparand equals this string, a negative value if this string is less than the comparand,
     *         a positive value if this string is greater than the comparand.
     */
    BOOST_LOG_EXPORT int compare(string_type const& s) const;
    /*!
     * Lexicographically compares the argument string to a part of this string
     *
     * \pre <tt>pos1 + n1 < size()</tt>
     * \param pos1 Starting position within this string to perform comparison to
     * \param n1 Length of the substring of this string to perform comparison to
     * \param that Comparand
     * \return Zero if the comparand equals this string, a negative value if this string is less than the comparand,
     *         a positive value if this string is greater than the comparand.
     *
     * \b Throws: An <tt>std::exception</tt>-based exception if arguments are out of range.
     */
    BOOST_LOG_EXPORT int compare(size_type pos1, size_type n1, basic_slim_string const& that) const;
    /*!
     * Lexicographically compares the argument string to a part of this string
     *
     * \pre <tt>pos1 + n1 < size()</tt>
     * \param pos1 Starting position within this string to perform comparison to
     * \param n1 Length of the substring of this string to perform comparison to
     * \param s Comparand
     * \return Zero if the comparand equals this string, a negative value if this string is less than the comparand,
     *         a positive value if this string is greater than the comparand.
     *
     * \b Throws: An <tt>std::exception</tt>-based exception if arguments are out of range.
     */
    BOOST_LOG_EXPORT int compare(size_type pos1, size_type n1, string_type const& s) const;
    /*!
     * Lexicographically compares substring of the argument string to a part of this string
     *
     * \pre <tt>pos1 + n1 < size() && pos2 + n2 < that.size()</tt>
     * \param pos1 Starting position within this string to perform comparison to
     * \param n1 Length of the substring of this string to perform comparison to
     * \param that Comparand full string
     * \param pos2 Starting position within the comparand to perform comparison to
     * \param n2 Length of the substring of the comparand to perform comparison to
     * \return Zero if the comparand equals this string, a negative value if this string is less than the comparand,
     *         a positive value if this string is greater than the comparand.
     *
     * \b Throws: An <tt>std::exception</tt>-based exception if arguments are out of range.
     */
    BOOST_LOG_EXPORT int compare(size_type pos1, size_type n1, basic_slim_string const& that, size_type pos2, size_type n2) const;
    /*!
     * Lexicographically compares substring of the argument string to a part of this string
     *
     * \pre <tt>pos1 + n1 < size() && pos2 + n2 < that.size()</tt>
     * \param pos1 Starting position within this string to perform comparison to
     * \param n1 Length of the substring of this string to perform comparison to
     * \param s Comparand full string
     * \param pos2 Starting position within the comparand to perform comparison to
     * \param n2 Length of the substring of the comparand to perform comparison to
     * \return Zero if the comparand equals this string, a negative value if this string is less than the comparand,
     *         a positive value if this string is greater than the comparand.
     *
     * \b Throws: An <tt>std::exception</tt>-based exception if arguments are out of range.
     */
    BOOST_LOG_EXPORT int compare(size_type pos1, size_type n1, string_type const& s, size_type pos2, size_type n2) const;
    /*!
     * Lexicographically compares the argument string to this string
     *
     * \param s Comparand. Must point to a zero-terminated sequence of characters, must not be NULL.
     * \return Zero if the comparand equals this string, a negative value if this string is less than the comparand,
     *         a positive value if this string is greater than the comparand.
     */
    BOOST_LOG_EXPORT int compare(const_pointer s) const;
    /*!
     * Lexicographically compares the argument string to this string
     *
     * \param s Comparand. Must point to a sequence of characters, must not be NULL.
     * \param n2 Number of characters in the sequence \a s.
     * \return Zero if the comparand equals this string, a negative value if this string is less than the comparand,
     *         a positive value if this string is greater than the comparand.
     */
    BOOST_LOG_EXPORT int compare(const_pointer s, size_type n2) const;
    /*!
     * Lexicographically compares the argument string to a part of this string
     *
     * \pre <tt>pos1 + n1 < size()</tt>
     * \param pos1 Starting position within this string to perform comparison to
     * \param n1 Length of the substring of this string to perform comparison to
     * \param s Comparand. Must point to a sequence of characters, must not be NULL.
     * \return Zero if the comparand equals this string, a negative value if this string is less than the comparand,
     *         a positive value if this string is greater than the comparand.
     *
     * \b Throws: An <tt>std::exception</tt>-based exception if arguments are out of range.
     */
    BOOST_LOG_EXPORT int compare(size_type pos1, size_type n1, const_pointer s) const;
    /*!
     * Lexicographically compares the argument string to a part of this string
     *
     * \pre <tt>pos1 + n1 < size()</tt>
     * \param pos1 Starting position within this string to perform comparison to
     * \param n1 Length of the substring of this string to perform comparison to
     * \param s Comparand. Must point to a sequence of characters, must not be NULL.
     * \param n2 Number of characters in the sequence \a s.
     * \return Zero if the comparand equals this string, a negative value if this string is less than the comparand,
     *         a positive value if this string is greater than the comparand.
     *
     * \b Throws: An <tt>std::exception</tt>-based exception if arguments are out of range.
     */
    BOOST_LOG_EXPORT int compare(size_type pos1, size_type n1, const_pointer s, size_type n2) const;
};

//! Output operator
template< typename CharT, typename TraitsT >
inline std::basic_ostream< CharT, TraitsT >& operator<< (
    std::basic_ostream< CharT, TraitsT >& strm, basic_slim_string< CharT, TraitsT > const& s)
{
    strm.write(s.data(), s.size());
    return strm;
}

//! A free-standing swap for slim strings
template< typename CharT, typename TraitsT >
inline void swap(basic_slim_string< CharT, TraitsT >& left, basic_slim_string< CharT, TraitsT >& right)
{
    left.swap(right);
}

#ifdef BOOST_LOG_USE_CHAR
typedef basic_slim_string< char > slim_string;      //!< Convenience typedef for narrow-character strings
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
typedef basic_slim_string< wchar_t > slim_wstring;  //!< Convenience typedef for wide-character strings
#endif

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_UTILITY_SLIM_STRING_HPP_INCLUDED_
