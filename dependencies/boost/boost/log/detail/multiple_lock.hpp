/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   multiple_lock.hpp
 * \author Andrey Semashev
 * \date   18.04.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_DETAIL_MULTIPLE_LOCK_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_MULTIPLE_LOCK_HPP_INCLUDED_

#include <functional>
#include <boost/utility/addressof.hpp>
#include <boost/log/detail/prologue.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

//! A deadlock-safe lock type that locks two mutexes
template< typename T1, typename T2 >
class multiple_unique_lock2
{
    T1* m_p1;
    T2* m_p2;

public:
    multiple_unique_lock2(T1& m1, T2& m2) :
        m_p1(boost::addressof(m1)),
        m_p2(boost::addressof(m2))
    {
        std::less< void* > order;
        if (order(m_p1, m_p2))
        {
            m_p1->lock();
            m_p2->lock();
        }
        else
        {
            m_p2->lock();
            m_p1->lock();
        }
    }
    ~multiple_unique_lock2()
    {
        m_p2->unlock();
        m_p1->unlock();
    }
};

} // namespace aux

} // namespace log

} // namespace boost

#endif // BOOST_LOG_DETAIL_MULTIPLE_LOCK_HPP_INCLUDED_
