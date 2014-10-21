/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   async_frontend.hpp
 * \author Andrey Semashev
 * \date   14.07.2009
 *
 * The header contains implementation of asynchronous sink frontent.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SINKS_ASYNC_FRONTEND_HPP_INCLUDED_
#define BOOST_LOG_SINKS_ASYNC_FRONTEND_HPP_INCLUDED_

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/locking_ptr.hpp>
#include <boost/log/detail/parameter_tools.hpp>
#include <boost/log/sinks/basic_sink_frontend.hpp>
#include <boost/log/sinks/threading_models.hpp>
#include <boost/log/keywords/start_thread.hpp>

#if defined(BOOST_LOG_NO_THREADS)
#error Boost.Log: Asynchronous sink frontend is only supported in multithreaded environment
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

    //! Asynchronous sink frontend implementation
    template< typename CharT >
    class asynchronous_frontend :
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
        BOOST_LOG_EXPORT asynchronous_frontend(shared_ptr< void > const& backend, consume_trampoline_t consume_tramp, bool start_thread);
        BOOST_LOG_EXPORT ~asynchronous_frontend();
        BOOST_LOG_EXPORT shared_ptr< void > const& get_backend() const;
        BOOST_LOG_EXPORT boost::log::aux::locking_ptr_counter_base& get_backend_locker() const;
        BOOST_LOG_EXPORT void consume(record_type const& record);
        BOOST_LOG_EXPORT bool try_consume(record_type const& record);

    public:
        //! The method starts record feeding loop
        BOOST_LOG_EXPORT void run();
        //! The method softly interrupts record feeding loop
        BOOST_LOG_EXPORT bool stop();
        //! The method feeds log records that may have been buffered to the backend and returns
        BOOST_LOG_EXPORT void feed_records();
    };

} // namespace aux

//! \cond
#define BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL(z, n, types)\
    template< BOOST_PP_ENUM_PARAMS(n, typename T) >\
    explicit asynchronous_sink(BOOST_PP_ENUM_BINARY_PARAMS(n, T, const& arg)) :\
        base_type(boost::make_shared< sink_backend_type >(BOOST_PP_ENUM_PARAMS(n, arg)),\
                  &asynchronous_sink::consume_trampoline,\
                  (BOOST_PP_ENUM_PARAMS(n, arg))[keywords::start_thread | true])\
    {}
//! \endcond

/*!
 * \brief Asynchronous logging sink frontend
 *
 * The frontend starts a separate thread on construction. All logging records are passed
 * to the backend in this dedicated thread only.
 */
template< typename SinkBackendT >
class asynchronous_sink :
    public aux::asynchronous_frontend< typename SinkBackendT::char_type >
{
    typedef aux::asynchronous_frontend< typename SinkBackendT::char_type > base_type;

public:
    //! Sink implementation type
    typedef SinkBackendT sink_backend_type;
    //! \cond
    BOOST_MPL_ASSERT((is_model_supported< typename sink_backend_type::threading_model, single_thread_tag >));
    //! \endcond

    typedef typename base_type::char_type char_type;
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
     *
     * \param start_thread If \c true, the frontend creates a thread to feed
     *                     log records to the backend. Otherwise no thread is
     *                     started and it is assumed that the user will call
     *                     either \c run or \c feed_records himself.
     */
    asynchronous_sink(bool start_thread = true) :
        base_type(boost::make_shared< sink_backend_type >(), &asynchronous_sink::consume_trampoline, start_thread)
    {
    }
    /*!
     * Constructor attaches user-constructed backend instance
     *
     * \param backend Pointer to the backend instance.
     * \param start_thread If \c true, the frontend creates a thread to feed
     *                     log records to the backend. Otherwise no thread is
     *                     started and it is assumed that the user will call
     *                     either \c run or \c feed_records himself.
     *
     * \pre \a backend is not \c NULL.
     */
    explicit asynchronous_sink(shared_ptr< sink_backend_type > const& backend, bool start_thread = true) :
        base_type(backend, &asynchronous_sink::consume_trampoline, start_thread)
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

#ifdef BOOST_LOG_DOXYGEN_PASS
    /*!
     * The method starts record feeding loop and effectively blocks until either of this happens:
     *
     * \li the thread is interrupted due to either standard thread interruption or a call to \c stop
     * \li an exception is thrown while processing a log record in the backend, and the exception is
     *     not terminated by the exception handler, if one is installed
     *
     * \pre The sink frontend must be constructed without spawning a dedicated thread
     */
    void run();

    /*!
     * The method softly interrupts record feeding loop. This method must be called when the \c run
     * method execution has to be interrupted. Unlike regular thread interruption, calling
     * \c stop will not interrupt the record processing in the middle. Instead, the sink frontend
     * will attempt to finish its business (including feeding the rest of the buffered records to the
     * backend) and return afterwards. This method can be called either if the sink was created
     * with a dedicated thread, or if the \c run method was called by user.
     *
     * \retval true If execution has been cancelled
     * \retval false If no execution was in process
     *
     * \note Returning from this method does not guarantee that there are no records left buffered
     *       in the sink frontend. It is possible that log records keep coming during and after this
     *       method is called. At some point of execution of this method log records stop being processed,
     *       and all records that come after this point are put into the queue. These records will be
     *       processed upon further calls to \c run or \c feed_records.
     */
    bool stop();

    /*!
     * The method feeds log records that may have been buffered to the backend and returns
     *
     * \pre The sink frontend must be constructed without spawning a dedicated thread
     */
    void feed_records();
#endif // BOOST_LOG_DOXYGEN_PASS

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

#endif // BOOST_LOG_SINKS_ASYNC_FRONTEND_HPP_INCLUDED_
