/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   basic_attribute_value.hpp
 * \author Andrey Semashev
 * \date   24.06.2007
 *
 * The header contains an implementation of an attribute value base class.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_ATTRIBUTES_BASIC_ATTRIBUTE_VALUE_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_BASIC_ATTRIBUTE_VALUE_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/detail/templated_shared_from_this.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace attributes {

/*!
 * \brief Basic attribute value class
 *
 * This class can be used as a boilerplate for simple attribute values. The class implements all needed
 * interfaces of attribute values and allows to store a single value of the type specified as a template parameter.
 * The stored value can be dispatched with type dispatching mechanism.
 */
template< typename T >
class basic_attribute_value :
    public attribute_value,
    public boost::log::aux::templated_shared_from_this
{
public:
    //! Value type
    typedef T held_type;

private:
    //! Attribute value
    held_type m_Value;

public:
    /*!
     * Constructor with initialization of the stored value
     */
    explicit basic_attribute_value(held_type const& v) : m_Value(v) {}

    virtual bool dispatch(type_dispatcher& dispatcher)
    {
        register type_visitor< held_type >* visitor =
            dispatcher.get_visitor< held_type >();
        if (visitor)
        {
            visitor->visit(m_Value);
            return true;
        }
        else
            return false;
    }

    virtual shared_ptr< attribute_value > detach_from_thread()
    {
        return this->shared_from_this< basic_attribute_value< held_type > >();
    }

    /*!
     * \return Reference to the contained value.
     */
    held_type const& get() const { return m_Value; }
};

} // namespace attributes

} // namespace log

} // namespace boost

#endif // BOOST_LOG_ATTRIBUTES_BASIC_ATTRIBUTE_VALUE_HPP_INCLUDED_
