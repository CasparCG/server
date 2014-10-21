/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   constant.hpp
 * \author Andrey Semashev
 * \date   15.04.2007
 *
 * The header contains implementation of a constant attribute.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_ATTRIBUTES_CONSTANT_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_CONSTANT_HPP_INCLUDED_

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/basic_attribute_value.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace attributes {

/*!
 * \brief A class of an attribute that holds a single constant value
 *
 * The constant is a simpliest and one of the most frequently types of attributes.
 * It stores a constant value, which it eventually returns as its value each time
 * requested.
 *
 * \internal The attribute attempts to optimize memory allocations and implements both
 *           attribute and \c attribute_value interfaces. However, the value can be detached from
 *           the attribute if \c detach_from_thread is called.
 */
template< typename T >
class constant :
    public attribute,
    public basic_attribute_value< T >
{
    //! Base type
    typedef basic_attribute_value< T > base_type;

public:
    //! A held constant type
    typedef typename base_type::held_type held_type;

public:
    /*!
     * Constructor with the stored value initialization
     */
    explicit constant(held_type const& value) : base_type(value) {}

    shared_ptr< attribute_value > get_value()
    {
        return this->BOOST_NESTED_TEMPLATE shared_from_this< base_type >();
    }

    shared_ptr< attribute_value > detach_from_thread()
    {
        // We have to create a copy of the constant because the attribute object
        // can be created on the stack and get destroyed even if there are shared_ptrs that point to it.
        return boost::make_shared< base_type >(static_cast< base_type const& >(*this));
    }
};

} // namespace attributes

} // namespace log

} // namespace boost

#endif // BOOST_LOG_ATTRIBUTES_CONSTANT_HPP_INCLUDED_
