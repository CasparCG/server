/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   current_thread_id.hpp
 * \author Andrey Semashev
 * \date   12.09.2009
 *
 * The header contains implementation of a current thread id attribute
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_ATTRIBUTES_CURRENT_THREAD_ID_HPP_INCLUDED_
#define BOOST_LOG_ATTRIBUTES_CURRENT_THREAD_ID_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>

#if defined(BOOST_LOG_NO_THREADS)
#error Boost.Log: The current_thread_id attribute is only available in multithreaded builds
#endif

#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread/thread.hpp>
#include <boost/log/attributes/basic_attribute_value.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace attributes {

/*!
 * \brief A class of an attribute that always returns the current thread identifier
 *
 * \note This attribute may be registered globally, it will still return the correct
 *       thread identifier no matter which thread emits the log record.
 */
class current_thread_id :
    public attribute,
    public attribute_value,
    public enable_shared_from_this< current_thread_id >
{
public:
    //! A held attribute value type
    typedef thread::id held_type;

public:
    virtual bool dispatch(type_dispatcher& dispatcher)
    {
        register type_visitor< held_type >* visitor =
            dispatcher.get_visitor< held_type >();
        if (visitor)
        {
            visitor->visit(this_thread::get_id());
            return true;
        }
        else
            return false;
    }

    virtual shared_ptr< attribute_value > get_value()
    {
        return this->shared_from_this();
    }

    virtual shared_ptr< attribute_value > detach_from_thread()
    {
        typedef basic_attribute_value< held_type > detached_value;
        return boost::make_shared< detached_value >(this_thread::get_id());
    }
};

} // namespace attributes

} // namespace log

} // namespace boost

#endif // BOOST_LOG_ATTRIBUTES_CURRENT_THREAD_ID_HPP_INCLUDED_
