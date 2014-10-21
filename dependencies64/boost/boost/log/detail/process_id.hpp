/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   process_id.hpp
 * \author Andrey Semashev
 * \date   12.09.2009
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_DETAIL_PROCESS_ID_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_PROCESS_ID_HPP_INCLUDED_

#include <iosfwd>
#include <boost/cstdint.hpp>
#include <boost/log/detail/prologue.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

    //! The process class (currently used as a namespace)
    class process
    {
    public:
        class id;
    };

    namespace this_process {

        //! The function returns current process identifier
        BOOST_LOG_EXPORT process::id get_id();

    } // namespace this_process

    template< typename CharT, typename TraitsT >
    BOOST_LOG_EXPORT std::basic_ostream< CharT, TraitsT >&
    operator<< (std::basic_ostream< CharT, TraitsT >& strm, process::id const& pid);

    //! Process identifier class
    class process::id
    {
        friend class process;

        friend id this_process::get_id();

        template< typename CharT, typename TraitsT >
        friend BOOST_LOG_EXPORT std::basic_ostream< CharT, TraitsT >&
        operator<< (std::basic_ostream< CharT, TraitsT >& strm, process::id const& pid);

    private:
        uintmax_t m_NativeID;

    public:
        id() : m_NativeID(0) {}

        bool operator== (id const& that) const
        {
            return (m_NativeID == that.m_NativeID);
        }
        bool operator!= (id const& that) const
        {
            return (m_NativeID != that.m_NativeID);
        }
        bool operator< (id const& that) const
        {
            return (m_NativeID < that.m_NativeID);
        }
        bool operator> (id const& that) const
        {
            return (m_NativeID > that.m_NativeID);
        }
        bool operator<= (id const& that) const
        {
            return (m_NativeID <= that.m_NativeID);
        }
        bool operator>= (id const& that) const
        {
            return (m_NativeID >= that.m_NativeID);
        }

    private:
        explicit id(uintmax_t native) : m_NativeID(native) {}
    };

} // namespace aux

} // namespace log

} // namespace boost

#endif // BOOST_LOG_DETAIL_PROCESS_ID_HPP_INCLUDED_
