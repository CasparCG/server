/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   mutable_constant.hpp
 * \author Andrey Semashev
 * \date   06.11.2007
 *
 * The header contains implementation of a mutable constant attribute.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_ATTRIBUTES_MUTABLE_CONSTANT_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_MUTABLE_CONSTANT_HPP_INCLUDED_

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_void.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/basic_attribute_value.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace attributes {

/*!
 * \brief A class of an attribute that holds a single constant value with ability to change it
 *
 * The mutable_constant attribute stores a single value of type, specified as the first template argument.
 * A copy of this value is returned on each attribute value acquision.
 *
 * The attribute also allows to modify the stored value, even if the attibute is registered in an attribute set.
 * In order to ensure thread safety of such modifications the mutable_constant class is also parametrized
 * with three additional template arguments: mutex type, scoped write and scoped read lock types. By default
 * no synchronization is done.
 */
template< typename T, typename MutexT = void, typename ScopedWriteLockT = void, typename ScopedReadLockT = ScopedWriteLockT >
class mutable_constant :
    public attribute
{
public:
    //! Mutex type
    typedef MutexT mutex_type;
    //! Shared lock type
    typedef ScopedReadLockT scoped_read_lock;
    //! Exclusive lock type
    typedef ScopedWriteLockT scoped_write_lock;
    BOOST_STATIC_ASSERT(!(is_void< mutex_type >::value || is_void< scoped_read_lock >::value || is_void< scoped_write_lock >::value));

    //! A held constant type
    typedef T held_type;

private:
    //! Attribute value type
    typedef basic_attribute_value< held_type > mutable_constant_value;

private:
    //! The actual value
    held_type m_Value;
    //! Thread protection mutex
    mutex_type m_Mutex;

public:
    /*!
     * Constructor with the stored value initialization
     */
    explicit mutable_constant(held_type const& value) : m_Value(value) {}

    /*!
     * The method returns the actual attribute value. It shall not return NULL.
     * The implementation acquires a shared lock of the mutex in order to protect the
     * copying of the stored object into the attribute value.
     */
    shared_ptr< attribute_value > get_value()
    {
        scoped_read_lock _(m_Mutex);
        return boost::make_shared< mutable_constant_value >(m_Value);
    }

    /*!
     * The method sets a new attribute value. The implementation exclusively locks the mutex in order
     * to protect the value assignment.
     */
    void set_value(held_type const& v)
    {
        scoped_write_lock _(m_Mutex);
        m_Value = v;
    }
};

/*!
 * \brief Specialization for unlocked case
 *
 * This version of attribute does not perform thread synchronization to access the stored value.
 */
template< typename T >
class mutable_constant< T, void, void, void > :
    public attribute
{
public:
    //! Mutex type
    typedef void mutex_type;
    //! Shared lock type
    typedef void scoped_read_lock;
    //! Exclusive lock type
    typedef void scoped_write_lock;

    //! A held constant type
    typedef T held_type;

private:
    //! Attribute value
    typedef basic_attribute_value< held_type > mutable_constant_value;

private:
    //! The actual value
    held_type m_Value;

public:
    /*!
     * Constructor with the stored value initialization
     */
    explicit mutable_constant(held_type const& value) : m_Value(value) {}

    shared_ptr< attribute_value > get_value()
    {
        return boost::make_shared< mutable_constant_value >(m_Value);
    }

    /*!
     * The method sets a new attribute value.
     */
    void set_value(held_type const& v)
    {
        m_Value = v;
    }
};

} // namespace attributes

} // namespace log

} // namespace boost

#endif // BOOST_LOG_ATTRIBUTES_MUTABLE_CONSTANT_HPP_INCLUDED_
