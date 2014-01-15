/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   light_rw_mutex.hpp
 * \author Andrey Semashev
 * \date   24.03.2009
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#ifndef BOOST_LOG_DETAIL_LIGHT_RW_MUTEX_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_LIGHT_RW_MUTEX_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>

#ifndef BOOST_LOG_NO_THREADS

#if defined(BOOST_THREAD_POSIX) // This one can be defined by users, so it should go first
#define BOOST_LOG_LWRWMUTEX_USE_PTHREAD
#elif defined(BOOST_WINDOWS) && defined(BOOST_LOG_USE_WINNT6_API)
#define BOOST_LOG_LWRWMUTEX_USE_SRWLOCK
#elif defined(BOOST_HAS_PTHREADS)
#define BOOST_LOG_LWRWMUTEX_USE_PTHREAD
#endif

#if defined(BOOST_LOG_LWRWMUTEX_USE_SRWLOCK)

#if defined(BOOST_USE_WINDOWS_H)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 // _WIN32_WINNT_LONGHORN
#endif

#include <windows.h>

#else // defined(BOOST_USE_WINDOWS_H)

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

extern "C" {

struct SRWLOCK { void* p; };
__declspec(dllimport) void __stdcall InitializeSRWLock(SRWLOCK*);
__declspec(dllimport) void __stdcall ReleaseSRWLockExclusive(SRWLOCK*);
__declspec(dllimport) void __stdcall ReleaseSRWLockShared(SRWLOCK*);
__declspec(dllimport) void __stdcall AcquireSRWLockExclusive(SRWLOCK*);
__declspec(dllimport) void __stdcall AcquireSRWLockShared(SRWLOCK*);

} // extern "C"

} // namespace aux

} // namespace log

} // namespace boost

#endif // BOOST_USE_WINDOWS_H

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

    //! A light read/write mutex that uses WinNT 6 and later APIs
    class light_rw_mutex
    {
        SRWLOCK m_Mutex;

    public:
        light_rw_mutex()
        {
            InitializeSRWLock(&m_Mutex);
        }
        void lock_shared()
        {
            AcquireSRWLockShared(&m_Mutex);
        }
        void unlock_shared()
        {
            ReleaseSRWLockShared(&m_Mutex);
        }
        void lock()
        {
            AcquireSRWLockExclusive(&m_Mutex);
        }
        void unlock()
        {
            ReleaseSRWLockExclusive(&m_Mutex);
        }

    private:
        // Noncopyable
        light_rw_mutex(light_rw_mutex const&);
        light_rw_mutex& operator= (light_rw_mutex const&);
    };

} // namespace aux

} // namespace log

} // namespace boost

#elif defined(BOOST_LOG_LWRWMUTEX_USE_PTHREAD)

#include <pthread.h>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

    //! A light read/write mutex that maps directly onto POSIX threading library
    class light_rw_mutex
    {
        pthread_rwlock_t m_Mutex;

    public:
        light_rw_mutex()
        {
            pthread_rwlock_init(&m_Mutex, NULL);
        }
        ~light_rw_mutex()
        {
            pthread_rwlock_destroy(&m_Mutex);
        }
        void lock_shared()
        {
            pthread_rwlock_rdlock(&m_Mutex);
        }
        void unlock_shared()
        {
            pthread_rwlock_unlock(&m_Mutex);
        }
        void lock()
        {
            pthread_rwlock_wrlock(&m_Mutex);
        }
        void unlock()
        {
            pthread_rwlock_unlock(&m_Mutex);
        }

    private:
        // Noncopyable
        light_rw_mutex(light_rw_mutex const&);
        light_rw_mutex& operator= (light_rw_mutex const&);
    };

} // namespace aux

} // namespace log

} // namespace boost

#else

#include <boost/thread/shared_mutex.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

    typedef shared_mutex light_rw_mutex;

} // namespace aux

} // namespace log

} // namespace boost

#endif

#endif // BOOST_LOG_NO_THREADS

#endif // BOOST_LOG_DETAIL_LIGHT_RW_MUTEX_HPP_INCLUDED_
