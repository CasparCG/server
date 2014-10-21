/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   has_attr.hpp
 * \author Andrey Semashev
 * \date   22.04.2007
 *
 * The header contains implementation of a filter that checks presence of an attribute in a log record.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_FILTERS_HAS_ATTR_HPP_INCLUDED_
#define BOOST_LOG_FILTERS_HAS_ATTR_HPP_INCLUDED_

#include <string>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/functional.hpp>
#include <boost/log/utility/attribute_value_extractor.hpp>
#include <boost/log/filters/basic_filters.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace filters {

/*!
 * \brief A filter that detects if there is an attribute with given name and type in the attribute values view
 *
 * The filter can be instantiated either with one particular attribute value type or with a sequence of types.
 */
template< typename CharT, typename AttributeValueTypesT = void >
class flt_has_attr :
    public basic_filter< CharT, flt_has_attr< CharT, AttributeValueTypesT > >
{
private:
    //! Base type
    typedef basic_filter< CharT, flt_has_attr< CharT, AttributeValueTypesT > > base_type;
    //! Attribute value extractor type
    typedef attribute_value_extractor< CharT, AttributeValueTypesT > extractor;

public:
    //! Attribute values container type
    typedef typename base_type::values_view_type values_view_type;
    //! Char type
    typedef typename base_type::char_type char_type;
    //! String type
    typedef typename base_type::string_type string_type;

private:
    //! Attribute extractor
    extractor m_Extractor;

public:
    /*!
     * Constructs the filter
     *
     * \param name Attribute name
     */
    explicit flt_has_attr(string_type const& name) : m_Extractor(name) {}

    /*!
     * Applies the filter
     *
     * \param values A set of attribute values of a single log record
     * \return true if the log record contains the sought attribute value, false otherwise
     */
    bool operator() (values_view_type const& values) const
    {
        return m_Extractor(values, boost::log::aux::nop());
    }
};

/*!
 * \brief A filter that detects if there is an attribute with given name in the complete attribute view
 *
 * The specialization is used when an attribute value of any type is sought.
 */
template< typename CharT >
class flt_has_attr< CharT, void > :
    public basic_filter< CharT, flt_has_attr< CharT, void > >
{
private:
    //! Base type
    typedef basic_filter< CharT, flt_has_attr< CharT, void > > base_type;

public:
    //! Attribute values container type
    typedef typename base_type::values_view_type values_view_type;
    //! Char type
    typedef typename base_type::char_type char_type;
    //! String type
    typedef typename base_type::string_type string_type;

private:
    //! Attribute name
    string_type m_AttributeName;

public:
    /*!
     * Constructs the filter
     *
     * \param name Attribute name
     */
    explicit flt_has_attr(string_type const& name) : m_AttributeName(name) {}

    /*!
     * Applies the filter
     *
     * \param values A set of attribute values of a single log record
     * \return true if the log record contains the sought attribute value, false otherwise
     */
    bool operator() (values_view_type const& values) const
    {
        return (values.find(m_AttributeName) != values.end());
    }
};

#ifdef BOOST_LOG_USE_CHAR

/*!
 * Filter generator
 */
inline flt_has_attr< char > has_attr(std::basic_string< char > const& name)
{
    return flt_has_attr< char >(name);
}

/*!
 * Filter generator
 */
template< typename AttributeValueTypesT >
inline flt_has_attr< char, AttributeValueTypesT > has_attr(std::basic_string< char > const& name)
{
    return flt_has_attr< char, AttributeValueTypesT >(name);
}

#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T

/*!
 * Filter generator
 */
inline flt_has_attr< wchar_t > has_attr(std::basic_string< wchar_t > const& name)
{
    return flt_has_attr< wchar_t >(name);
}

/*!
 * Filter generator
 */
template< typename AttributeValueTypesT >
inline flt_has_attr< wchar_t, AttributeValueTypesT > has_attr(std::basic_string< wchar_t > const& name)
{
    return flt_has_attr< wchar_t, AttributeValueTypesT >(name);
}

#endif // BOOST_LOG_USE_WCHAR_T

} // namespace filters

} // namespace log

} // namespace boost

#endif // BOOST_LOG_FILTERS_HAS_ATTR_HPP_INCLUDED_
