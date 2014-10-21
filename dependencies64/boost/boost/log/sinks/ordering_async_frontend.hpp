/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   ordering_async_frontend.hpp
 * \author Andrey Semashev
 * \date   23.08.2009
 *
 * The header contains implementation of asynchronous sink frontent with log record ordering capability.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SINKS_ORDERING_ASYNC_FRONTEND_HPP_INCLUDED_
#define BOOST_LOG_SINKS_ORDERING_ASYNC_FRONTEND_HPP_INCLUDED_

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/function/function2.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/locking_ptr.hpp>
#include <boost/log/detail/parameter_tools.hpp>
#include <boost/log/sinks/basic_sink_frontend.hpp>
#include <boost/log/sinks/threading_models.hpp>
#include <boost/log/keywords/start_thread.hpp>
#include <boost/log/keywords/order.hpp>
#include <boost/log/keywords/ordering_window.hpp>

#if defined(BOOST_LOG_NO_THREADS)
#error Boost.Log: Ordering asynchronous sink frontend is only supported in multithreaded environment
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

    //! Ordering asynchronous sink frontend implementation
    template< typename CharT >
    class ordering_asynchronous_frontend :
        public basic_sink_frontend< CharT >
    {
        typedef basic_sink_frontend< CharT > base_type;

    public:
        typedef typename base_type::record_type record_type;
        typedef function2< bool, record_type const&, record_type const& > order_type;

    protected:
        typedef void (*consume_trampoline_t)(void*, record_type const&);

    private:
        struct implementation;

    protected:
        BOOST_LOG_EXPORT ordering_asynchronous_frontend(
            shared_ptr< void > const& backend,
            consume_trampoline_t consume_tramp,
            bool start_thread,
            order_type const& order,
            posix_time::time_duration const& ordering_window);
        BOOST_LOG_EXPORT ~ordering_asynchronous_frontend();
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
        void feed_records()
        {
            feed_records(get_ordering_window());
        }
        //! The method feeds log records that may have been buffered to the backend and returns
        BOOST_LOG_EXPORT void feed_records(posix_time::time_duration ordering_window);
        //! Ordering window size
        BOOST_LOG_EXPORT posix_time::time_duration get_ordering_window() const;
        //! Default ordering window size
        BOOST_LOG_EXPORT static posix_time::time_duration get_default_ordering_window();
    };

} // namespace aux

//! \cond
#define BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL(z, n, types)\
    template< BOOST_PP_ENUM_PARAMS(n, typename T) >\
    explicit ordering_asynchronous_sink(BOOST_PP_ENUM_BINARY_PARAMS(n, T, const& arg)) :\
        base_type(boost::make_shared< sink_backend_type >(BOOST_PP_ENUM_PARAMS(n, arg)),\
                  &ordering_asynchronous_sink::consume_trampoline,\
                  (BOOST_PP_ENUM_PARAMS(n, arg))[keywords::start_thread | true],\
                  (BOOST_PP_ENUM_PARAMS(n, arg))[keywords::order],\
                  (BOOST_PP_ENUM_PARAMS(n, arg))[keywords::ordering_window | posix_time::milliseconds(30)])\
    {}\
    template< BOOST_PP_ENUM_PARAMS(n, typename T) >\
    explicit ordering_asynchronous_sink(shared_ptr< sink_backend_type > const& backend, BOOST_PP_ENUM_BINARY_PARAMS(n, T, const& arg)) :\
        base_type(backend,\
                  &ordering_asynchronous_sink::consume_trampoline,\
                  (BOOST_PP_ENUM_PARAMS(n, arg))[keywords::start_thread | true],\
                  (BOOST_PP_ENUM_PARAMS(n, arg))[keywords::order],\
                  (BOOST_PP_ENUM_PARAMS(n, arg))[keywords::ordering_window || &base_type::get_default_ordering_window])\
    {}
//! \endcond

/*!
 * \brief Asynchronous logging sink frontend
 *
 * The frontend starts a separate thread on construction. All logging records are passed
 * to the backend in this dedicated thread only.
 */
template< typename SinkBackendT >
class ordering_asynchronous_sink :
    public aux::ordering_asynchronous_frontend< typename SinkBackendT::char_type >
{
    typedef aux::ordering_asynchronous_frontend< typename SinkBackendT::char_type > base_type;

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
#ifdef BOOST_LOG_DOXYGEN_PASS

    /*!
     * Constructor attaches a user-constructed backend instance
     *
     * \param backend Pointer to the backend instance.
     * \param args One or several additional named parameters for the frontend. The following parameters are supported:
     *             \li \c order - An ordering predicate that will be used to order log records. The predicate
     *                            should accept two objects of type \c record_type and return \c bool. This
     *                            parameter is mandatory.
     *             \li \c ordering_window - A time latency for which log records can be delayed within the
     *                                      frontend in order to apply ordering. Should be specified as a Boost.DateTime
     *                                      <tt>posix_time::time_duration</tt> value. Optional parameter, some predefined
     *                                      system-specific value is used if not specified.
     *             \li \c start_thread - If \c true, the frontend will start a dedicated thread to feed log records to
     *                                   the backend. Otherwise the user is expected to call either \c run or \c feed_records
     *                                   to feed records.
     *
     * \pre \a backend is not \c NULL.
     */
    template< typename... ArgsT >
    explicit ordering_asynchronous_sink(shared_ptr< sink_backend_type > const& backend, ArgsT... const& args);

    /*!
     * Constructor creates the frontend and the backend with the specified parameters
     *
     * \param args One or several additional construction named parameters. These parameters will be
     *             forwarded to the backend constructor. However, the frontend will use the following ones of them:
     *             \li \c order - An ordering predicate that will be used to order log records. The predicate
     *                            should accept two objects of type \c record_type and return \c bool. This
     *                            parameter is mandatory.
     *             \li \c ordering_window - A time latency for which log records can be delayed within the
     *                                      frontend in order to apply ordering. Should be specified as a Boost.DateTime
     *                                      <tt>posix_time::time_duration</tt> value. Optional parameter, some predefined
     *                                      system-specific value is used if not specified.
     *             \li \c start_thread - If \c true, the frontend will start a dedicated thread to feed log records to
     *                                   the backend. Otherwise the user is expected to call either \c run or \c feed_records
     *                                   to feed records.
     */
    template< typename... ArgsT >
    explicit ordering_asynchronous_sink(ArgsT... const& args);

#else

    // Constructors that pass arbitrary parameters to the backend constructor
    BOOST_LOG_PARAMETRIZED_CONSTRUCTORS_GEN(BOOST_LOG_SINK_CTOR_FORWARD_INTERNAL, ~)

#endif // BOOST_LOG_DOXYGEN_PASS

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
     * \param ordering_window The function will not feed records that arrived to the frontend
     *                        within the specified ordering window time duration.
     *
     * \pre The sink frontend must be constructed without spawning a dedicated thread
     *
     * \note In order to perform a full flush the \a ordering_window parameter should be specified as 0.
     */
    void feed_records(posix_time::time_duration ordering_window = get_ordering_window());

    /*!
     * Returns ordering window size specified during initialization
     */
    posix_time::time_duration get_ordering_window() const;

    /*!
     * Returns default ordering window size.
     * The default window size is specific to the operating system thread scheduling mechanism.
     */
    static posix_time::time_duration get_default_ordering_window();
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

#endif // BOOST_LOG_SINKS_ORDERING_ASYNC_FRONTEND_HPP_INCLUDED_
