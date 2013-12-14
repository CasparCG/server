/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   type_dispatcher.hpp
 * \author Andrey Semashev
 * \date   15.04.2007
 *
 * The header contains definition of generic type dispatcher interfaces.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_TYPE_DISPATCHER_HPP_INCLUDED_
#define BOOST_LOG_TYPE_DISPATCHER_HPP_INCLUDED_

#include <typeinfo>
#include <boost/mpl/aux_/lambda_support.hpp>
#include <boost/log/detail/prologue.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

/*!
 * \brief An interface to the concrete type visitor
 *
 * This interface is used by type dispatchers to consume the dispatched value.
 */
template< typename T >
struct BOOST_LOG_NO_VTABLE type_visitor
{
    //! The type, which the visitor is able to consume
    typedef T supported_type;

    /*!
     * Virtual destructor
     */
    virtual ~type_visitor() {}

    /*!
     * The method invokes the visitor-specific logic with the given value
     *
     * \param value The dispatched value
     */
    virtual void visit(T const& value) = 0;

#ifndef BOOST_LOG_DOXYGEN_PASS
    BOOST_MPL_AUX_LAMBDA_SUPPORT(1, type_visitor, (T))
#endif // BOOST_LOG_DOXYGEN_PASS
};

/*!
 * \brief A type dispatcher interface
 *
 * All type dispatchers support this interface. It is used to acquire the
 * visitor interface for the requested type.
 */
struct BOOST_LOG_NO_VTABLE type_dispatcher
{
public:
    /*!
     * Virtual destructor
     */
    virtual ~type_dispatcher() {}

    /*!
     * The method requests a type visitor for a value of type \c T
     *
     * \return The type-specific visitor or NULL, if the type is not supported
     */
    template< typename T >
    type_visitor< T >* get_visitor()
    {
        return reinterpret_cast< type_visitor< T >* >(
            this->get_visitor(typeid(T)));
    }

private:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! The get_visitor method implementation
    virtual void* get_visitor(std::type_info const& type) = 0;
#endif // BOOST_LOG_DOXYGEN_PASS
};

} // namespace log

} // namespace boost

#endif // BOOST_LOG_TYPE_DISPATCHER_HPP_INCLUDED_
