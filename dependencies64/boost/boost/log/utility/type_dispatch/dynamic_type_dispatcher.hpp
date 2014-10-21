/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   dynamic_type_dispatcher.hpp
 * \author Andrey Semashev
 * \date   15.04.2007
 *
 * The header contains implementation of the run-time type dispatcher.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_DYNAMIC_TYPE_DISPATCHER_HPP_INCLUDED_
#define BOOST_LOG_DYNAMIC_TYPE_DISPATCHER_HPP_INCLUDED_

#include <new>
#include <memory>
#include <map>
#include <boost/ref.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/utility/type_info_wrapper.hpp>
#include <boost/log/utility/type_dispatch/type_dispatcher.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

    //! A single type visitor implementation
    template< typename T, typename FunT >
    class dynamic_type_dispatcher_visitor :
        public type_visitor< T >
    {
        //! Underlying allocator type
        typedef std::allocator< dynamic_type_dispatcher_visitor > underlying_allocator;

    private:
        //! The delegate to be called to do the actual work
        FunT m_Fun;

    public:
        //! Constructor
        explicit dynamic_type_dispatcher_visitor(FunT const& fun) : m_Fun(fun) {}

        //! The method invokes the visitor-specific logic with the given value
        void visit(T const& value)
        {
            m_Fun(value);
        }
    };

} // namespace aux

/*!
 * \brief A dynamic type dispatcher
 *
 * The type dispatcher can be used to pass objects of arbitrary types from one
 * component to another. With regard to the library, the type dispatcher
 * can be used to extract attribute values.
 *
 * The dynamic type dispatcher can be initialized in run time and, therefore,
 * can support different types, depending on runtime conditions. Each
 * supported type is associated with a functional object that will be called
 * when an object of the type is dispatched.
 */
class dynamic_type_dispatcher :
    public type_dispatcher
{
private:
    //! The dispatching map
    typedef std::map< type_info_wrapper, shared_ptr< void > > dispatching_map;
    dispatching_map m_DispatchingMap;

public:
    /*!
     * The method registers a new type
     *
     * \param fun Function object that will be associated with the type \c T
     */
    template< typename T, typename FunT >
    void register_type(FunT const& fun)
    {
        boost::shared_ptr< type_visitor< T > > p(
            boost::make_shared< aux::dynamic_type_dispatcher_visitor< T, FunT > >(boost::cref(fun)));

        type_info_wrapper wrapper(typeid(T));
        m_DispatchingMap[wrapper] = p;
    }

    /*!
     * The method returns the number of registered types
     */
    dispatching_map::size_type registered_types_count() const
    {
        return m_DispatchingMap.size();
    }

private:
#ifndef BOOST_LOG_DOXYGEN_PASS
    void* get_visitor(std::type_info const& type)
    {
        type_info_wrapper wrapper(type);
        dispatching_map::iterator it = m_DispatchingMap.find(wrapper);
        if (it != m_DispatchingMap.end())
            return it->second.get();
        else
            return NULL;
    }
#endif // BOOST_LOG_DOXYGEN_PASS
};

} // namespace log

} // namespace boost

#endif // BOOST_LOG_DYNAMIC_TYPE_DISPATCHER_HPP_INCLUDED_
