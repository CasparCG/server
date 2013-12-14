/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   sync_frontend.hpp
 * \author Andrey Semashev
 * \date   14.07.2009
 *
 * The header contains implementation of synchronous sink frontend.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SINKS_SYNC_FRONTEND_HPP_INCLUDED_
#define BOOST_LOG_SINKS_SYNC_FRONTEND_HPP_INCLUDED_

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/locking_ptr.hpp>
#include <boost/log/detail/parameter_tools.hpp>
#include <boost/log/sinks/basic_sink_frontend.hpp>
#include <boost/log/sinks/threading_models.hpp>

#if defined(BOOST_LOG_NO_THREADS)
#error Boost.Log: Synchronous sink frontend is only supported in multithreaded environment
#endif

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

namespace aux {

    //! Synchronous sink frontend implementation
    template< typename CharT >
    class BOOST_LOG_NO_VTABLE synchronous_frontend :
        public basic_sink_frontend< CharT >
    {
        typedef basic_sink_frontend< CharT > base_type;

    public:
        typedef typename base_type::record_type record_type;

    protected:
        typedef void (*consume_trampoline_t)(void*, record_type const&);

    private:
        struct implementation;

    protected:
        BOOST_LOG_EXPORT synchronous_frontend(shared_ptr< void > const& backend, consume_trampoline_t consume_tramp);
        BOOST_LOG_EXPORT shared_ptr< void > const& get_backend() const;
        BOOST_LOG_EXPORT boost::log::aux::locking_ptr_counter_base& get_backend_locker() const;
        BOOST_LOG_EXPORT void consume(record_type const& record);
        BOOST_LOG_EXPORT bool try_consume(record_type const& record);
    };

} // namespace aux

//! \cond
#define BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL(z, n, data)\
    template< BOOST_PP_ENUM_PARAMS(n, typename T) >\
    explicit synchronous_sink(BOOST_PP_ENUM_BINARY_PARAMS(n, T, const& arg)) :\
        base_type(boost::make_shared< sink_backend_type >(BOOST_PP_ENUM_PARAMS(n, arg)), &synchronous_sink::consume_trampoline) {}
//! \endcond

/*!
 * \brief Synchronous logging sink frontend
 *
 * The sink frontend serializes threads before passing logging records to the backend
 */
template< typename SinkBackendT >
class synchronous_sink :
    public aux::synchronous_frontend< typename SinkBackendT::char_type >
{
    typedef aux::synchronous_frontend< typename SinkBackendT::char_type > base_type;

public:
    //! Sink implementation type
    typedef SinkBackendT sink_backend_type;
    //! \cond
    BOOST_MPL_ASSERT((is_model_supported< typename sink_backend_type::threading_model, frontend_synchronization_tag >));
    //! \endcond

    typedef typename base_type::record_type record_type;
    typedef typename base_type::string_type string_type;

#ifndef BOOST_LOG_DOXYGEN_PASS

    //! A pointer type that locks the backend until it's destroyed
    typedef boost::log::aux::locking_ptr< sink_backend_type > locked_backend_ptr;

#else // BOOST_LOG_DOXYGEN_PASS

    //! A pointer type that locks the backend until it's destroyed
    typedef implementation_defined locked_backend_ptr;

#endif // BOOST_LOG_DOXYGEN_PASS

public:
    /*!
     * Default constructor. Constructs the sink backend instance.
     * Requires the backend to be default-constructible.
     */
    synchronous_sink() :
        base_type(boost::make_shared< sink_backend_type >(), &synchronous_sink::consume_trampoline)
    {
    }
    /*!
     * Constructor attaches user-constructed backend instance
     *
     * \param backend Pointer to the backend instance
     *
     * \pre \a backend is not \c NULL.
     */
    explicit synchronous_sink(shared_ptr< sink_backend_type > const& backend) :
        base_type(backend, &synchronous_sink::consume_trampoline)
    {
    }

    // Constructors that pass arbitrary parameters to the backend constructor
    BOOST_LOG_PARAMETRIZED_CONSTRUCTORS_GEN(BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL, ~)

    /*!
     * Locking accessor to the attached backend
     */
    locked_backend_ptr locked_backend() const
    {
        return locked_backend_ptr(
            boost::static_pointer_cast< sink_backend_type >(base_type::get_backend()),
            base_type::get_backend_locker());
    }

private:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! The method puts logging message to the sink
    static void consume_trampoline(void* backend, record_type const& record)
    {
        static_cast< sink_backend_type* >(backend)->consume(record);
    }
#endif // BOOST_LOG_DOXYGEN_PASS
};

#undef BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL

} // namespace sinks

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_SINKS_SYNC_FRONTEND_HPP_INCLUDED_
