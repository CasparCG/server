/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   templated_shared_from_this.hpp
 * \author Andrey Semashev
 * \date   28.02.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_DETAIL_TEMPLATED_SHARED_FROM_THIS_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_TEMPLATED_SHARED_FROM_THIS_HPP_INCLUDED_

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/log/detail/prologue.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

    /*!
     *  \brief A type-neutral version for the \c enable_shared_from_this class
     *
     *  This class can be useful when several base classes in the hierarchy need to
     *  obtain a valid \c shared_ptr from \c this pointer.
     */
    class templated_shared_from_this :
        public enable_shared_from_this< templated_shared_from_this >
    {
    protected:
        //  These methods are intended to be used from the derived classes only.
        //  Therefore we are pretty sure the static_cast is always valid in the code below.
        template< typename T >
        shared_ptr< T > shared_from_this()
        {
            typedef enable_shared_from_this< templated_shared_from_this > base_type;
            return static_pointer_cast< T >(base_type::shared_from_this());
        }
        template< typename T >
        shared_ptr< T > shared_from_this() const
        {
            typedef enable_shared_from_this< templated_shared_from_this > base_type;
            return static_pointer_cast< T >(base_type::shared_from_this());
        }
    };

} // namespace aux

} // namespace log

} // namespace boost

#endif // BOOST_LOG_DETAIL_TEMPLATED_SHARED_FROM_THIS_HPP_INCLUDED_
