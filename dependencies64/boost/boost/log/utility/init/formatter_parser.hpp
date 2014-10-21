/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   formatter_parser.hpp
 * \author Andrey Semashev
 * \date   07.04.2008
 *
 * The header contains definition of a formatter parser function, along with facilities to
 * add support for custom formatters.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_UTILITY_INIT_FORMATTER_PARSER_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_INIT_FORMATTER_PARSER_HPP_INCLUDED_

#include <new> // std::nothrow
#include <iosfwd>
#include <map>
#include <string>
#include <boost/function/function2.hpp>
#include <boost/log/detail/setup_prologue.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/formatters/attr.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
// non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

/*!
 * \brief Auxiliary formatter traits
 *
 * The structure generates commonly used types related to formatters and formatter factories.
 */
template< typename CharT >
struct formatter_types
{
    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Output stream type
    typedef std::basic_ostream< char_type > ostream_type;
    //! Log record type
    typedef basic_record< char_type > record_type;
    //! The formatter function object
    typedef function2< void, ostream_type&, record_type const& > formatter_type;

    /*!
     * Type of the map of formatter factory arguments [argument name -> argument value].
     * This type of maps will be passed to formatter factories on attempt to create a formatter.
     */
    typedef std::map< string_type, string_type > formatter_factory_args;
    /*!
     * \brief The type of a function object that constructs formatter instance
     * \param name Attribute name
     * \param args Formatter arguments
     * \return The constructed formatter. The formatter must not be empty.
     *
     * \b Throws: An <tt>std::exception</tt>-based If an exception is thrown from the method,
     *        the exception is propagated to the parse_formatter caller
     */
    typedef function2< formatter_type, string_type const&, formatter_factory_args const& > formatter_factory;
    //! Map of formatter factory function objects
    typedef std::map< string_type, formatter_factory > factories_map;
};

namespace aux {

    /*!
     * A simple formatter factory function. Provides an easy way to add user-defined types support to
     * the formatter parser. The factory does not consider any formatter arguments, if specified,
     * but produces the formatter that uses the native \c operator<< to format the attribute value.
     *
     * \param attr_name Attribute name to create formatter for.
     */
    template< typename CharT, typename AttributeValueT >
    typename formatter_types< CharT >::formatter_type make_simple_formatter(
        typename formatter_types< CharT >::string_type const& attr_name,
        typename formatter_types< CharT >::formatter_factory_args const&)
    {
        typedef typename formatter_types< CharT >::formatter_type formatter_type;
        return formatter_type(boost::log::formatters::attr< AttributeValueT >(attr_name, std::nothrow));
    }

} // namespace aux

/*!
 * \brief The function registers a user-defined formatter factory
 *
 * The function registers a user-defined formatter factory. The registered factory function will be
 * called when the formatter parser detects the specified attribute name in the formatter string.
 *
 * \pre <tt>attr_name != NULL && !factory.empty()</tt>, \c attr_name must point to a zero-terminated sequence of characters.
 *
 * \param attr_name Attribute name
 * \param factory Formatter factory function
 */
template< typename CharT >
BOOST_LOG_SETUP_EXPORT void register_formatter_factory(
    const CharT* attr_name,
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    typename formatter_types< CharT >::formatter_factory const& factory
#else
    function2<
        function2< void, std::basic_ostream< CharT >&, basic_record< CharT > const& >,
        std::basic_string< CharT > const&,
        std::map< std::basic_string< CharT >, std::basic_string< CharT > > const&
    > const& factory
#endif // BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
    );

/*!
 * \brief The function registers a user-defined formatter factory
 *
 * The function registers a user-defined formatter factory. The registered factory function will be
 * called when the formatter parser detects the specified attribute name in the formatter string.
 *
 * \pre <tt>!factory.empty()</tt>
 *
 * \param attr_name Attribute name
 * \param factory Formatter factory function
 */
template< typename CharT, typename TraitsT, typename AllocatorT >
inline void register_formatter_factory(
    std::basic_string< CharT, TraitsT, AllocatorT > const& attr_name,
    typename formatter_types< CharT >::formatter_factory const& factory)
{
    register_formatter_factory(attr_name.c_str(), factory);
}

/*!
 * \brief The function registers a simple formatter factory
 *
 * The function registers a simple formatter factory. The registered factory will generate formatters
 * that will be equivalent to the <tt>log::formatters::attr</tt> formatter (i.e. that will use the
 * native \c operator<< to format the attribute value). The factory does not use any arguments,
 * if specified.
 *
 * \pre <tt>attr_name != NULL</tt>, \c attr_name must point to a zero-terminated sequence of characters.
 *
 * \param attr_name Attribute name
 */
template< typename AttributeValueT, typename CharT >
inline void register_simple_formatter_factory(const CharT* attr_name)
{
    register_formatter_factory(attr_name, &boost::log::aux::make_simple_formatter< CharT, AttributeValueT >);
}

/*!
 * \brief The function registers a simple formatter factory
 *
 * The function registers a simple formatter factory. The registered factory will generate formatters
 * that will be equivalent to the <tt>log::formatters::attr</tt> formatter (i.e. that will use the
 * native \c operator<< to format the attribute value). The factory does not use any arguments,
 * if specified.
 *
 * \param attr_name Attribute name
 */
template< typename AttributeValueT, typename CharT, typename TraitsT, typename AllocatorT >
inline void register_simple_formatter_factory(std::basic_string< CharT, TraitsT, AllocatorT > const& attr_name)
{
    register_formatter_factory(attr_name.c_str(), &boost::log::aux::make_simple_formatter< CharT, AttributeValueT >);
}

/*!
 * The function parses a formatter from the sequence of characters
 *
 * \pre <tt>begin <= end</tt>, both pointers must not be NULL
 * \param begin Pointer to the first character of the sequence
 * \param end Pointer to the after-the-last character of the sequence
 * \return A function object that can be used as a formatter.
 *
 * \b Throws: An <tt>std::exception</tt>-based exception, if a formatter cannot be recognized in the character sequence.
 */
template< typename CharT >
BOOST_LOG_SETUP_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
typename formatter_types< CharT >::formatter_type
#else
function2< void, std::basic_ostream< CharT >&, basic_record< CharT > const& >
#endif
parse_formatter(const CharT* begin, const CharT* end);

/*!
 * The function parses a formatter from the string
 *
 * \param str A string that contains format description
 * \return A function object that can be used as a formatter.
 *
 * \b Throws: An <tt>std::exception</tt>-based exception, if a formatter cannot be recognized in the character sequence.
 */
template< typename CharT, typename TraitsT, typename AllocatorT >
inline typename formatter_types< CharT >::formatter_type
parse_formatter(std::basic_string< CharT, TraitsT, AllocatorT > const& str)
{
    const CharT* p = str.c_str();
    return parse_formatter(p, p + str.size());
}

/*!
 * The function parses a formatter from the string
 *
 * \pre <tt>str != NULL</tt>, <tt>str</tt> points to a zero-terminated string
 * \param str A string that contains format description.
 * \return A function object that can be used as a formatter.
 *
 * \b Throws: An <tt>std::exception</tt>-based exception, if a formatter cannot be recognized in the character sequence.
 */
template< typename CharT >
inline typename formatter_types< CharT >::formatter_type
parse_formatter(const CharT* str)
{
    return parse_formatter(str, str + std::char_traits< CharT >::length(str));
}

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_UTILITY_INIT_FORMATTER_PARSER_HPP_INCLUDED_
