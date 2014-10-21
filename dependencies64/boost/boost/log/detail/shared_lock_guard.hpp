/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   shared_lock_guard.hpp
 * \author Andrey Semashev
 * \date   26.04.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_DETAIL_SHARED_LOCK_GUARD_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_SHARED_LOCK_GUARD_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

//! An analogue to the minimalistic lock_guard template
//! that locks shared_mutex with shared ownership
template< typename MutexT >
struct shared_lock_guard
{
    explicit shared_lock_guard(MutexT& m) : m_Mutex(m)
    {
        m.lock_shared();
    }
    ~shared_lock_guard()
    {
        m_Mutex.unlock_shared();
    }

private:
    shared_lock_guard(shared_lock_guard const&);
    shared_lock_guard& operator= (shared_lock_guard const&);

private:
    MutexT& m_Mutex;
};

} // namespace aux

} // namespace log

} // namespace boost

#endif // BOOST_LOG_DETAIL_SHARED_LOCK_GUARD_HPP_INCLUDED_
