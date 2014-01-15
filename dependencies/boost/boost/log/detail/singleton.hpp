/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   singleton.hpp
 * \author Andrey Semashev
 * \date   20.04.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#ifndef BOOST_LOG_DETAIL_SINGLETON_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_SINGLETON_HPP_INCLUDED_

#include <boost/noncopyable.hpp>
#include <boost/log/detail/prologue.hpp>
#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/thread/once.hpp>
#else
#include <boost/log/utility/no_unused_warnings.hpp>
#endif

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

//! A base class for singletons, constructed on-demand
template< typename DerivedT, typename StorageT = DerivedT >
class lazy_singleton : noncopyable
{
public:
    //! Returns the singleton instance
    static StorageT& get()
    {
#if !defined(BOOST_LOG_NO_THREADS)
        static once_flag flag = BOOST_ONCE_INIT;
        boost::call_once(flag, &DerivedT::init_instance);
#else
        static const bool initialized = (DerivedT::init_instance(), true);
        BOOST_LOG_NO_UNUSED_WARNINGS(initialized);
#endif
        return get_instance();
    }

    //! Initializes the singleton instance
    static void init_instance()
    {
        get_instance();
    }

protected:
    //! Returns the singleton instance (not thread-safe)
    static StorageT& get_instance()
    {
        static StorageT instance;
        return instance;
    }
};

//! A base class for singletons, constructed on namespace scope initialization stage
template< typename DerivedT, typename StorageT = DerivedT >
class singleton :
    public lazy_singleton< DerivedT, StorageT >
{
public:
    static StorageT& instance;
};

template< typename DerivedT, typename StorageT >
StorageT& singleton< DerivedT, StorageT >::instance =
    lazy_singleton< DerivedT, StorageT >::get();

} // namespace aux

} // namespace log

} // namespace boost

#endif // BOOST_LOG_DETAIL_SINGLETON_HPP_INCLUDED_
