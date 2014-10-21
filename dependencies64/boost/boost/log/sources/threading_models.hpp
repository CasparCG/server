/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   sources/threading_models.hpp
 * \author Andrey Semashev
 * \date   04.10.2008
 *
 * The header contains definition of threading models that can be used in loggers.
 * The header also provides a number of tags that can be used to express lock requirements
 * on a function callee.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SOURCES_THREADING_MODELS_HPP_INCLUDED_
#define BOOST_LOG_SOURCES_THREADING_MODELS_HPP_INCLUDED_

#include <boost/noncopyable.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/less.hpp>
#include <boost/mpl/if.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
#include <boost/preprocessor/repetition/enum_trailing.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/log/detail/prologue.hpp>
#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/mpl/bool.hpp>
#include <boost/thread/locks.hpp>
#include <boost/log/detail/shared_lock_guard.hpp>
#endif

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
#endif // _MSC_VER

#ifndef BOOST_LOG_STRICTEST_LOCK_LIMIT
#define BOOST_LOG_STRICTEST_LOCK_LIMIT 10
#endif // BOOST_LOG_STRICTEST_LOCK_LIMIT
#if BOOST_LOG_STRICTEST_LOCK_LIMIT < 3
#error The BOOST_LOG_STRICTEST_LOCK_LIMIT macro should not be less than 3
#endif

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace sources {

//! Single thread locking model
struct single_thread_model
{
    // We provide methods for the most advanced locking concept: UpgradeLockable
    void lock_shared() const {}
    bool try_lock_shared() const { return true; }
    template< typename TimeT >
    bool timed_lock_shared(TimeT const&) const { return true; }
    void unlock_shared() const {}
    void lock() const {}
    bool try_lock() const { return true; }
    template< typename TimeT >
    bool timed_lock(TimeT const&) const { return true; }
    void unlock() const {}
    void lock_upgrade() const {}
    bool try_lock_upgrade() const { return true; }
    template< typename TimeT >
    bool timed_lock_upgrade(TimeT const&) const { return true; }
    void unlock_upgrade() const {}
    void unlock_upgrade_and_lock() const {}
    void unlock_and_lock_upgrade() const {}
    void unlock_and_lock_shared() const {}
    void unlock_upgrade_and_lock_shared() const {}

    void swap(single_thread_model&) {}
};

#if !defined(BOOST_LOG_NO_THREADS)

//! Multi-thread locking model with maximum locking capabilities
template< typename MutexT >
struct multi_thread_model
{
    multi_thread_model() {}
    multi_thread_model(multi_thread_model const&) {}
    multi_thread_model& operator= (multi_thread_model const&) { return *this; }

    void lock_shared() const { m_Mutex.lock_shared(); }
    bool try_lock_shared() const { return m_Mutex.try_lock_shared(); }
    template< typename TimeT >
    bool timed_lock_shared(TimeT const& t) const { return m_Mutex.timed_lock_shared(t); }
    void unlock_shared() const { m_Mutex.unlock_shared(); }
    void lock() const { m_Mutex.lock(); }
    bool try_lock() const { return m_Mutex.try_lock(); }
    template< typename TimeT >
    bool timed_lock(TimeT const& t) const { return m_Mutex.timed_lock(t); }
    void unlock() const { m_Mutex.unlock(); }
    void lock_upgrade() const { m_Mutex.lock_upgrade(); }
    bool try_lock_upgrade() const { return m_Mutex.try_lock_upgrade(); }
    template< typename TimeT >
    bool timed_lock_upgrade(TimeT const& t) const { return m_Mutex.timed_lock_upgrade(t); }
    void unlock_upgrade() const { m_Mutex.unlock_upgrade(); }
    void unlock_upgrade_and_lock() const { m_Mutex.unlock_upgrade_and_lock(); }
    void unlock_and_lock_upgrade() const { m_Mutex.unlock_and_lock_upgrade(); }
    void unlock_and_lock_shared() const { m_Mutex.unlock_and_lock_shared(); }
    void unlock_upgrade_and_lock_shared() const { m_Mutex.unlock_upgrade_and_lock_shared(); }

    void swap(multi_thread_model&) {}

private:
    //! Synchronization primitive
    mutable MutexT m_Mutex;
};

#endif // !defined(BOOST_LOG_NO_THREADS)

//! An auxiliary pseudo-lock to express no locking requirements in logger features
struct no_lock : noncopyable
{
    /*!
     * Constructs the pseudo-lock with any type of mutex. The mutex is not affected during construction.
     */
    template< typename T >
    explicit no_lock(T&) {}
};

namespace aux {

    enum thread_access_mode
    {
        unlocked_access,
        shared_access,
        exclusive_access
    };

    template< typename LockT >
    struct thread_access_mode_of;

    template< >
    struct thread_access_mode_of< no_lock > : mpl::int_< unlocked_access >
    {
    };

#if !defined(BOOST_LOG_NO_THREADS)

    template< typename MutexT >
    struct thread_access_mode_of< lock_guard< MutexT > > : mpl::int_< exclusive_access >
    {
    };

    template< typename MutexT >
    struct thread_access_mode_of< unique_lock< MutexT > > : mpl::int_< exclusive_access >
    {
    };

    template< typename MutexT >
    struct thread_access_mode_of< shared_lock< MutexT > > : mpl::int_< shared_access >
    {
    };

    template< typename MutexT >
    struct thread_access_mode_of< upgrade_lock< MutexT > > : mpl::int_< shared_access >
    {
    };

    template< typename MutexT >
    struct thread_access_mode_of< boost::log::aux::shared_lock_guard< MutexT > > : mpl::int_< shared_access >
    {
    };

#endif // !defined(BOOST_LOG_NO_THREADS)

    //! The metafunction selects the most strict lock type of the two
    template< typename LeftLockT, typename RightLockT >
    struct strictest_lock_impl :
        mpl::if_<
            mpl::less< aux::thread_access_mode_of< LeftLockT >, aux::thread_access_mode_of< RightLockT > >,
            RightLockT,
            LeftLockT
        >
    {
    };

} // namespace aux

#if defined(BOOST_LOG_DOXYGEN_PASS)

/*!
 * \brief The metafunction selects the most strict lock type of the specified.
 *
 * The template supports all lock types provided by the Boost.Thread
 * library (except for \c upgrade_to_unique_lock), plus additional
 * pseudo-lock \c no_lock that indicates no locking at all.
 * Exclusive locks are considered the strictest, shared locks are weaker,
 * and \c no_lock is the weakest.
 */
template< typename... LocksT >
struct strictest_lock
{
    typedef implementation_defined type;
};

#else

#define BOOST_LOG_IDENTITY_INTERNAL(z, i, data) data

template<
    typename TT0,
    typename TT1,
    BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(BOOST_PP_SUB(BOOST_LOG_STRICTEST_LOCK_LIMIT, 2), typename T, void)
>
struct strictest_lock
{
    typedef
        BOOST_PP_ENUM_PARAMS(BOOST_PP_SUB(BOOST_LOG_STRICTEST_LOCK_LIMIT, 2), typename aux::strictest_lock_impl< T),
            typename aux::strictest_lock_impl< TT0, TT1 >::type
        BOOST_PP_REPEAT_FROM_TO(2, BOOST_LOG_STRICTEST_LOCK_LIMIT, BOOST_LOG_IDENTITY_INTERNAL, >::type)
    type;
};

#define BOOST_LOG_DEFINE_STRICTEST_LOG_SPEC_INTERNAL(z, i, data)\
    template< typename TT0, typename TT1 BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_SUB(i, 2), typename T) >\
    struct strictest_lock<\
        TT0, TT1\
        BOOST_PP_ENUM_TRAILING_PARAMS(BOOST_PP_SUB(i, 2), T)\
        BOOST_PP_ENUM_TRAILING(BOOST_PP_SUB(BOOST_LOG_STRICTEST_LOCK_LIMIT, i), BOOST_LOG_IDENTITY_INTERNAL, void)\
    >\
    {\
        typedef\
            BOOST_PP_ENUM_PARAMS(BOOST_PP_SUB(i, 2), typename aux::strictest_lock_impl< T)\
                BOOST_PP_COMMA_IF(BOOST_PP_SUB(i, 2)) typename aux::strictest_lock_impl< TT0, TT1 >::type\
            BOOST_PP_REPEAT_FROM_TO(2, i, BOOST_LOG_IDENTITY_INTERNAL, >::type)\
        type;\
    };

BOOST_PP_REPEAT_FROM_TO(2, BOOST_PP_SUB(BOOST_LOG_STRICTEST_LOCK_LIMIT, 1), BOOST_LOG_DEFINE_STRICTEST_LOG_SPEC_INTERNAL, ~)

#undef BOOST_LOG_DEFINE_STRICTEST_LOG_SPEC_INTERNAL
#undef BOOST_LOG_IDENTITY_INTERNAL

#endif // BOOST_LOG_DOXYGEN_PASS

} // namespace sources

} // namespace log

#if !defined(BOOST_LOG_NO_THREADS) && !defined(BOOST_LOG_DOXYGEN_PASS)

template< >
struct is_mutex_type< boost::log::sources::single_thread_model > : mpl::true_
{
};

template< typename T >
struct is_mutex_type< boost::log::sources::multi_thread_model< T > > : mpl::true_
{
};

#endif // !defined(BOOST_LOG_NO_THREADS)

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_SOURCES_THREADING_MODELS_HPP_INCLUDED_
