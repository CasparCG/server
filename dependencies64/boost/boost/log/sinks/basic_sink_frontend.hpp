/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   basic_sink_frontend.hpp
 * \author Andrey Semashev
 * \date   14.07.2009
 *
 * The header contains implementation of a base class for sink frontends.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SINKS_BASIC_SINK_FRONTEND_HPP_INCLUDED_
#define BOOST_LOG_SINKS_BASIC_SINK_FRONTEND_HPP_INCLUDED_

#include <boost/function/function0.hpp>
#include <boost/function/function1.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/sinks/sink.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
// non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace sinks {

//! A base class for a logging sink frontend
template< typename CharT >
class BOOST_LOG_NO_VTABLE basic_sink_frontend :
    public sink< CharT >
{
    //! Base type
    typedef sink< CharT > base_type;

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! String type to be used as a message text holder
    typedef typename base_type::string_type string_type;
    //! Log record type
    typedef typename base_type::record_type record_type;
    //! Attribute values view type
    typedef typename base_type::values_view_type values_view_type;
    //! Filter function type
    typedef function1< bool, values_view_type const& > filter_type;
    //! An exception handler type
    typedef function0< void > exception_handler_type;

protected:
    //! Sink frontend implementation
    struct implementation;

private:
    //! Pointer to the frontend implementation
    implementation* m_pImpl;

protected:
    //! The constructor installs the pointer to the frontend implementation
    BOOST_LOG_EXPORT explicit basic_sink_frontend(implementation* p);

public:
    /*!
     * Destructor
     */
    BOOST_LOG_EXPORT ~basic_sink_frontend();

    /*!
     * The method sets sink-specific filter functional object
     */
    BOOST_LOG_EXPORT void set_filter(filter_type const& filter);
    /*!
     * The method resets the filter
     */
    BOOST_LOG_EXPORT void reset_filter();

    /*!
     * The method sets an exception handler function
     */
    BOOST_LOG_EXPORT void set_exception_handler(exception_handler_type const& handler);

    /*!
     * The method returns \c true if no filter is set or the attribute values pass the filter
     *
     * \param attributes A set of attribute values of a logging record
     */
    BOOST_LOG_EXPORT bool will_consume(values_view_type const& attributes);

protected:
    //! Returns pointer to the frontend implementation data
    template< typename T >
    BOOST_LOG_FORCEINLINE T* get_impl() const
    {
        BOOST_LOG_ASSUME(m_pImpl != NULL);
        return static_cast< T* >(m_pImpl);
    }
};

} // namespace sinks

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_SINKS_SINK_HPP_INCLUDED_
