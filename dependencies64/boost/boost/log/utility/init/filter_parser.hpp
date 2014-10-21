/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   filter_parser.hpp
 * \author Andrey Semashev
 * \date   31.03.2008
 *
 * The header contains definition of a filter parser function.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_UTILITY_INIT_FILTER_PARSER_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_INIT_FILTER_PARSER_HPP_INCLUDED_

#include <new> // std::nothrow
#include <string>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/function/function1.hpp>
#include <boost/log/detail/setup_prologue.hpp>
#include <boost/log/exceptions.hpp>
#include <boost/log/attributes/attribute_values_view.hpp>
#include <boost/log/filters/attr.hpp>
#include <boost/log/filters/has_attr.hpp>
#include <boost/log/core/core.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
// non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

//! The interface class for all filter factories
template< typename CharT >
struct filter_factory :
    private noncopyable
{
    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Attribute values view type
    typedef basic_attribute_values_view< char_type > values_view_type;
    //! Filter function type
    typedef function1< bool, values_view_type const& > filter_type;

    //! Virtual destructor
    virtual ~filter_factory() {}

    //! The callback for filter for the attribute existence test
    virtual filter_type on_exists_test(string_type const& name)
    {
        return filter_type(filters::has_attr(name));
    }

    //! The callback for equality relation filter
    virtual filter_type on_equality_relation(string_type const& name, string_type const& arg)
    {
        BOOST_LOG_THROW_DESCR(parse_error, "The equality attribute value relation is not supported");
    }
    //! The callback for inequality relation filter
    virtual filter_type on_inequality_relation(string_type const& name, string_type const& arg)
    {
        BOOST_LOG_THROW_DESCR(parse_error, "The inequality attribute value relation is not supported");
    }
    //! The callback for less relation filter
    virtual filter_type on_less_relation(string_type const& name, string_type const& arg)
    {
        BOOST_LOG_THROW_DESCR(parse_error, "The less attribute value relation is not supported");
    }
    //! The callback for greater relation filter
    virtual filter_type on_greater_relation(string_type const& name, string_type const& arg)
    {
        BOOST_LOG_THROW_DESCR(parse_error, "The greater attribute value relation is not supported");
    }
    //! The callback for less or equal relation filter
    virtual filter_type on_less_or_equal_relation(string_type const& name, string_type const& arg)
    {
        BOOST_LOG_THROW_DESCR(parse_error, "The less-or-equal attribute value relation is not supported");
    }
    //! The callback for greater or equal relation filter
    virtual filter_type on_greater_or_equal_relation(string_type const& name, string_type const& arg)
    {
        BOOST_LOG_THROW_DESCR(parse_error, "The greater-or-equal attribute value relation is not supported");
    }

    //! The callback for custom relation filter
    virtual filter_type on_custom_relation(string_type const& name, string_type const& rel, string_type const& arg)
    {
        BOOST_LOG_THROW_DESCR(parse_error, "The custom attribute value relation is not supported");
    }
};

//! The base class for filter factories
template< typename CharT, typename AttributeValueT >
class basic_filter_factory :
    public filter_factory< CharT >
{
    //! Base type
    typedef filter_factory< CharT > base_type;

public:
    //! The type(s) of the attribute value expected
    typedef AttributeValueT attribute_value_type;
    //  Type imports
    typedef typename base_type::string_type string_type;
    typedef typename base_type::filter_type filter_type;

    //! The callback for filter for the attribute existence test
    virtual filter_type on_exists_test(string_type const& name)
    {
        return filter_type(filters::has_attr< attribute_value_type >(name));
    }

    //! The callback for equality relation filter
    virtual filter_type on_equality_relation(string_type const& name, string_type const& arg)
    {
        return filter_type(filters::attr< attribute_value_type >(name, std::nothrow) == parse_argument(arg));
    }
    //! The callback for inequality relation filter
    virtual filter_type on_inequality_relation(string_type const& name, string_type const& arg)
    {
        return filter_type(filters::attr< attribute_value_type >(name, std::nothrow) != parse_argument(arg));
    }
    //! The callback for less relation filter
    virtual filter_type on_less_relation(string_type const& name, string_type const& arg)
    {
        return filter_type(filters::attr< attribute_value_type >(name, std::nothrow) < parse_argument(arg));
    }
    //! The callback for greater relation filter
    virtual filter_type on_greater_relation(string_type const& name, string_type const& arg)
    {
        return filter_type(filters::attr< attribute_value_type >(name, std::nothrow) > parse_argument(arg));
    }
    //! The callback for less or equal relation filter
    virtual filter_type on_less_or_equal_relation(string_type const& name, string_type const& arg)
    {
        return filter_type(filters::attr< attribute_value_type >(name, std::nothrow) <= parse_argument(arg));
    }
    //! The callback for greater or equal relation filter
    virtual filter_type on_greater_or_equal_relation(string_type const& name, string_type const& arg)
    {
        return filter_type(filters::attr< attribute_value_type >(name, std::nothrow) >= parse_argument(arg));
    }

    //! The callback for custom relation filter
    virtual filter_type on_custom_relation(string_type const& name, string_type const& rel, string_type const& arg)
    {
        BOOST_LOG_THROW_DESCR(parse_error, "The custom attribute value relation is not supported");
    }

    //! The function parses the argument value for a binary relation
    virtual attribute_value_type parse_argument(string_type const& arg)
    {
        return boost::lexical_cast< attribute_value_type >(arg);
    }
};

/*!
 * The function registers a filter factory object for the specified attribute name. The factory will be
 * used to construct a filter during parsing the filter string.
 *
 * \pre <tt>name != NULL && factory != NULL</tt>, <tt>name</tt> points to a zero-terminated string
 * \param name Attribute name to associate the factory with
 * \param factory The filter factory
 */
template< typename CharT >
BOOST_LOG_SETUP_EXPORT void register_filter_factory(
    const CharT* name, shared_ptr< filter_factory< CharT > > const& factory);

/*!
 * The function registers a filter factory object for the specified attribute name. The factory will be
 * used to construct a filter during parsing the filter string.
 *
 * \pre <tt>factory != NULL</tt>
 * \param name Attribute name to associate the factory with
 * \param factory The filter factory
 */
template< typename CharT, typename TraitsT, typename AllocatorT >
inline void register_filter_factory(
    std::basic_string< CharT, TraitsT, AllocatorT > const& name, shared_ptr< filter_factory< CharT > > const& factory)
{
    register_filter_factory(name.c_str(), factory);
}

/*!
 * The function registers a simple filter factory object for the specified attribute name. The factory will
 * support attribute values of type \c AttributeValueT, which must support all relation operations, such as
 * equality comparison and less/greater ordering.
 *
 * \pre <tt>name != NULL</tt>, <tt>name</tt> points to a zero-terminated string
 * \param name Attribute name to associate the factory with
 */
template< typename AttributeValueT, typename CharT >
inline void register_simple_filter_factory(const CharT* name)
{
    shared_ptr< filter_factory< CharT > > factory =
        boost::make_shared< basic_filter_factory< CharT, AttributeValueT > >();
    register_filter_factory(name, factory);
}

/*!
 * The function registers a simple filter factory object for the specified attribute name. The factory will
 * support attribute values of type \c AttributeValueT, which must support all relation operations, such as
 * equality comparison and less/greater ordering.
 *
 * \pre <tt>name != NULL</tt>, <tt>name</tt> points to a zero-terminated string
 * \param name Attribute name to associate the factory with
 */
template< typename AttributeValueT, typename CharT, typename TraitsT, typename AllocatorT >
inline void register_simple_filter_factory(std::basic_string< CharT, TraitsT, AllocatorT > const& name)
{
    register_simple_filter_factory< AttributeValueT >(name.c_str());
}

/*!
 * The function parses a filter from the sequence of characters
 *
 * \pre <tt>begin <= end</tt>, both pointers must not be NULL
 * \param begin Pointer to the first character of the sequence
 * \param end Pointer to the after-the-last character of the sequence
 * \return A function object that can be used as a filter.
 *
 * \b Throws: An <tt>std::exception</tt>-based exception, if a filter cannot be recognized in the character sequence.
 */
template< typename CharT >
BOOST_LOG_SETUP_EXPORT
#ifndef BOOST_LOG_BROKEN_TEMPLATE_DEFINITION_MATCHING
typename basic_core< CharT >::filter_type
#else
function1< bool, basic_attribute_values_view< CharT > const& >
#endif
parse_filter(const CharT* begin, const CharT* end);

/*!
 * The function parses a filter from the string
 *
 * \param str A string that contains filter description
 * \return A function object that can be used as a filter.
 *
 * \b Throws: An <tt>std::exception</tt>-based exception, if a filter cannot be recognized in the character sequence.
 */
template< typename CharT, typename TraitsT, typename AllocatorT >
inline typename basic_core< CharT >::filter_type
parse_filter(std::basic_string< CharT, TraitsT, AllocatorT > const& str)
{
    const CharT* p = str.c_str();
    return parse_filter(p, p + str.size());
}

/*!
 * The function parses a filter from the string
 *
 * \pre <tt>str != NULL</tt>, <tt>str</tt> points to a zero-terminated string.
 * \param str A string that contains filter description.
 * \return A function object that can be used as a filter.
 *
 * \b Throws: An <tt>std::exception</tt>-based exception, if a filter cannot be recognized in the character sequence.
 */
template< typename CharT >
inline typename basic_core< CharT >::filter_type parse_filter(const CharT* str)
{
    return parse_filter(str, str + std::char_traits< CharT >::length(str));
}

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_UTILITY_INIT_FILTER_PARSER_HPP_INCLUDED_
