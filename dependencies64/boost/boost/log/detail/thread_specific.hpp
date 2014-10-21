/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   thread_specific.hpp
 * \author Andrey Semashev
 * \date   01.03.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_DETAIL_THREAD_SPECIFIC_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_THREAD_SPECIFIC_HPP_INCLUDED_

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_pod.hpp>
#include <boost/log/detail/prologue.hpp>

#if !defined(BOOST_LOG_NO_THREADS)

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
// non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

//! Base class for TLS to hide platform-specific storage management
class thread_specific_base
{
private:
    union key_storage
    {
        void* as_pointer;
        unsigned int as_dword;
    };

    key_storage m_Key;

protected:
    BOOST_LOG_EXPORT thread_specific_base();
    BOOST_LOG_EXPORT ~thread_specific_base();
    BOOST_LOG_EXPORT void* get_content() const;
    BOOST_LOG_EXPORT void set_content(void* value) const;

private:
    //  Copying prohibited
    thread_specific_base(thread_specific_base const&);
    thread_specific_base& operator= (thread_specific_base const&);
};

//! A TLS wrapper for small POD types with least possible overhead
template< typename T >
class thread_specific :
    public thread_specific_base
{
    BOOST_STATIC_ASSERT(sizeof(T) <= sizeof(void*) && is_pod< T >::value);

    //! Union to perform type casting
    union value_storage
    {
        void* as_pointer;
        T as_value;
    };

public:
    //! Default constructor
    thread_specific() {}
    //! Initializing constructor
    thread_specific(T const& value)
    {
        set(value);
    }
    //! Assignment
    thread_specific& operator= (T const& value)
    {
        set(value);
        return *this;
    }

    //! Accessor
    T get() const
    {
        value_storage cast;
        cast.as_pointer = thread_specific_base::get_content();
        return cast.as_value;
    }

    //! Setter
    void set(T const& value)
    {
        value_storage cast;
        cast.as_value = value;
        thread_specific_base::set_content(cast.as_pointer);
    }
};

} // namespace aux

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // !defined(BOOST_LOG_NO_THREADS)

#endif // BOOST_LOG_DETAIL_THREAD_SPECIFIC_HPP_INCLUDED_
