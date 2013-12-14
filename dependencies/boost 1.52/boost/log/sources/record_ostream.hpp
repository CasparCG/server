/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   record_ostream.hpp
 * \author Andrey Semashev
 * \date   09.03.2009
 *
 * This header contains a wrapper class around a logging record that allows to compose the
 * record message with a streaming expression.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SOURCES_RECORD_OSTREAM_HPP_INCLUDED_
#define BOOST_LOG_SOURCES_RECORD_OSTREAM_HPP_INCLUDED_

#include <new>
#include <string>
#include <locale>
#include <ostream>
#include <boost/utility/addressof.hpp>
#include <boost/utility/base_from_member.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/native_typeof.hpp>
#include <boost/log/detail/unspecified_bool.hpp>
#include <boost/log/detail/attachable_sstream_buf.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/utility/unique_identifier_name.hpp>

#ifdef _MSC_VER
#pragma warning(push)
 // non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

/*!
 * \brief Logging record adapter with a streaming capability
 *
 * This class allows to compose the logging record message by streaming operations. It
 * aggregates the log record and provides the standard output stream interface.
 */
template< typename CharT, typename TraitsT = std::char_traits< CharT > >
class basic_record_ostream :
    private base_from_member< boost::log::aux::basic_ostringstreambuf< CharT, TraitsT > >,
    public std::basic_ostream< CharT, TraitsT >
{
    //! Self type
    typedef basic_record_ostream< CharT, TraitsT > this_type;
    //! Base class that contains the stream buffer
    typedef base_from_member< boost::log::aux::basic_ostringstreambuf< CharT, TraitsT > > ostream_buf_base_type;

public:
    //! Stream type
    typedef std::basic_ostream< CharT, TraitsT > ostream_type;
    //! Log record type
    typedef basic_record< CharT > record_type;
    //! Character type
    typedef typename record_type::char_type char_type;
    //! String type to be used as a message text holder
    typedef typename record_type::string_type string_type;

private:
    //! Log record
    record_type m_Record;

public:
    /*!
     * Default constructor. Creates an empty record that is equivalent to the invalid record handle.
     * The stream capability is not available after construction.
     *
     * \post <tt>!*this == true</tt>
     */
    basic_record_ostream() : ostream_type(boost::addressof(ostream_buf_base_type::member))
    {
        ostream_type::clear(ostream_type::badbit);
    }

    /*!
     * Conversion from a record handle. Adopts the record referenced by the handle.
     *
     * \pre The handle, if valid, have been issued by the logging core with the same character type as the record being constructed.
     * \post <tt>this->handle() == rec</tt>
     * \param rec The record handle being adopted
     */
    explicit basic_record_ostream(record_handle const& rec) :
        ostream_type(boost::addressof(ostream_buf_base_type::member)),
        m_Record(rec)
    {
        ostream_type::clear(ostream_type::badbit);
        init_stream();
    }
    /*!
     * Conversion from a record object. Adopts the record referenced by the object.
     *
     * \pre The handle, if valid, have been issued by the logging core with the same character type as the record being constructed.
     * \post <tt>this->handle() == rec</tt>
     * \param rec The record handle being adopted
     */
    basic_record_ostream(record_type const& rec) :
        ostream_type(boost::addressof(ostream_buf_base_type::member)),
        m_Record(rec)
    {
        ostream_type::clear(ostream_type::badbit);
        init_stream();
    }

    /*!
     * Destructor. Destroys the record, releases any sinks and attribute values that were involved in processing this record.
     */
    ~basic_record_ostream()
    {
        detach_from_record();
    }

    /*!
     * Conversion to an unspecified boolean type
     *
     * \return \c true, if stream is valid and ready for formatting, \c false, if the stream is not valid. The latter also applies to
     *         the case when the stream is not attached to a log record.
     */
    BOOST_LOG_OPERATOR_UNSPECIFIED_BOOL()

    /*!
     * Inverted conversion to an unspecified boolean type
     *
     * \return \c false, if stream is valid and ready for formatting, \c true, if the stream is not valid. The latter also applies to
     *         the case when the stream is not attached to a log record.
     */
    bool operator! () const
    {
        return (!m_Record || ostream_type::fail());
    }

    /*!
     * Flushes internal buffers to complete all pending formatting operations and returns the aggregated log record
     *
     * \return The aggregated record object
     */
    record_type const& record() const
    {
        const_cast< this_type* >(this)->flush();
        return m_Record;
    }

    /*!
     * If the stream is attached to a log record, flushes internal buffers to complete all pending formatting operations.
     * Then reattaches the stream to another log record.
     *
     * \param rec New log record to attach to
     */
    void record(record_type rec)
    {
        detach_from_record();
        m_Record.swap(rec);
        init_stream();
    }

private:
    //  Copy and assignment are closed
    basic_record_ostream(basic_record_ostream const&);
    basic_record_ostream& operator= (basic_record_ostream const&);

    //! The function initializes the stream and the stream buffer
    void init_stream()
    {
        if (!!m_Record)
        {
            ostream_buf_base_type::member.attach(m_Record.message());
            ostream_type::clear(ostream_type::goodbit);
            ostream_type::flags(
                ostream_type::dec |
                ostream_type::skipws |
                ostream_type::boolalpha // this differs from the default stream flags but makes logs look better
            );
            ostream_type::width(0);
            ostream_type::precision(6);
            ostream_type::fill(static_cast< char_type >(' '));
            ostream_type::imbue(std::locale());
        }
    }
    //! The function resets the stream into a detached (default initialized) state
    void detach_from_record()
    {
        if (!!m_Record)
        {
            ostream_buf_base_type::member.detach();
            ostream_type::exceptions(ostream_type::goodbit);
            ostream_type::clear(ostream_type::badbit);
        }
    }
};


#ifdef BOOST_LOG_USE_CHAR
typedef basic_record_ostream< char > record_ostream;        //!< Convenience typedef for narrow-character logging
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
typedef basic_record_ostream< wchar_t > wrecord_ostream;    //!< Convenience typedef for wide-character logging
#endif

namespace aux {

//! Internal class that provides formatting streams for record pumps
template< typename CharT >
struct stream_provider
{
    //! Character type
    typedef CharT char_type;
    //! Record type
    typedef basic_record< char_type > record_type;

    //! Formatting stream compound
    struct stream_compound
    {
        stream_compound* next;

        //! Log record stream adapter
        basic_record_ostream< char_type > stream;

        //! Initializing constructor
        explicit stream_compound(record_type const& rec) : next(NULL), stream(rec) {}
    };

    //! The method returns an allocated stream compound
    BOOST_LOG_EXPORT static stream_compound* allocate_compound(record_type const& rec);
    //! The method releases a compound
    BOOST_LOG_EXPORT static void release_compound(stream_compound* compound) /* throw() */;

private:
    //  Non-constructible, non-copyable, non-assignable
    stream_provider();
    stream_provider(stream_provider const&);
    stream_provider& operator= (stream_provider const&);
};


/*!
 * \brief Logging record pump implementation
 *
 * The pump is used to format the logging record message text and then
 * push it to the logging core. It is constructed on each attempt to write
 * a log record and destroyed afterwards.
 *
 * The pump class template is instantiated on the logger type.
 */
template< typename LoggerT >
class record_pump
{
private:
    //! Logger type
    typedef LoggerT logger_type;
    //! Character type
    typedef typename logger_type::char_type char_type;
    //! Log record type
    typedef typename logger_type::record_type record_type;
    //! Stream compound provider
    typedef stream_provider< char_type > stream_provider_type;
    //! Stream compound type
    typedef typename stream_provider_type::stream_compound stream_compound;

    //! Stream compound release guard
    class auto_release;
    friend class auto_release;
    class auto_release
    {
        stream_compound* m_pCompound;

    public:
        explicit auto_release(stream_compound* p) : m_pCompound(p) {}
        ~auto_release() { stream_provider_type::release_compound(m_pCompound); }
    };

protected:
    //! A reference to the logger
    mutable logger_type* m_pLogger;
    //! Stream compound
    mutable stream_compound* m_pStreamCompound;

public:
    //! Constructor
    explicit record_pump(logger_type& lg, record_type const& rec) :
        m_pLogger(boost::addressof(lg)),
        m_pStreamCompound(stream_provider_type::allocate_compound(rec))
    {
    }
    //! Copy constructor (implemented as move)
    record_pump(record_pump const& that) :
        m_pLogger(that.m_pLogger),
        m_pStreamCompound(that.m_pStreamCompound)
    {
        that.m_pLogger = 0;
        that.m_pStreamCompound = 0;
    }
    //! Destructor. Pushes the composed message to log.
    ~record_pump()
    {
        if (m_pLogger)
        {
            auto_release _(m_pStreamCompound); // destructor doesn't throw
            if (!std::uncaught_exception())
                m_pLogger->push_record(m_pStreamCompound->stream.record());
        }
    }

    //! Returns the stream to be used for message text formatting
    std::basic_ostream< char_type >& stream() const
    {
        BOOST_ASSERT(m_pStreamCompound != 0);
        return m_pStreamCompound->stream;
    }

private:
    //! Closed assignment
    record_pump& operator= (record_pump const&);
};

template< typename LoggerT >
BOOST_LOG_FORCEINLINE record_pump< LoggerT > make_pump_stream(LoggerT& lg, typename LoggerT::record_type const& rec)
{
    return record_pump< LoggerT >(lg, rec);
}

template< typename LoggerT >
BOOST_LOG_FORCEINLINE record_pump< LoggerT > make_pump_stream(LoggerT& lg, record_handle const& rec)
{
    typedef typename LoggerT::record_type record_type;
    return record_pump< LoggerT >(lg, record_type(rec));
}

} // namespace aux

//! \cond

#ifdef BOOST_LOG_AUTO

#define BOOST_LOG_STREAM_INTERNAL(logger, rec_var)\
    for (BOOST_LOG_AUTO(rec_var, (logger).open_record()); !!rec_var; rec_var.reset())\
        ::boost::log::aux::make_pump_stream((logger), rec_var).stream()

#define BOOST_LOG_STREAM_WITH_PARAMS_INTERNAL(logger, rec_var, params_seq)\
    for (BOOST_LOG_AUTO(rec_var, (logger).open_record((BOOST_PP_SEQ_ENUM(params_seq)))); !!rec_var; rec_var.reset())\
        ::boost::log::aux::make_pump_stream((logger), rec_var).stream()

#else // BOOST_LOG_AUTO

#define BOOST_LOG_STREAM_INTERNAL(logger, rec_var)\
    for (::boost::log::record_handle rec_var = (logger).open_record().handle(); !!rec_var; rec_var.reset())\
        ::boost::log::aux::make_pump_stream((logger), rec_var).stream()

#define BOOST_LOG_STREAM_WITH_PARAMS_INTERNAL(logger, rec_var, params_seq)\
    for (::boost::log::record_handle rec_var = (logger).open_record((BOOST_PP_SEQ_ENUM(params_seq))).handle(); !!rec_var; rec_var.reset())\
        ::boost::log::aux::make_pump_stream((logger), rec_var).stream()

#endif // BOOST_LOG_AUTO

//! \endcond

//! The macro writes a record to the log
#define BOOST_LOG_STREAM(logger)\
    BOOST_LOG_STREAM_INTERNAL(logger, BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_record_))

//! The macro writes a record to the log and allows to pass additional named arguments to the logger
#define BOOST_LOG_STREAM_WITH_PARAMS(logger, params_seq)\
    BOOST_LOG_STREAM_WITH_PARAMS_INTERNAL(logger, BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_record_), params_seq)

#ifndef BOOST_LOG_NO_SHORTHAND_NAMES

//! An equivalent to BOOST_LOG_STREAM(logger)
#define BOOST_LOG(logger) BOOST_LOG_STREAM(logger)

//! An equivalent to BOOST_LOG_STREAM_WITH_PARAMS(logger, params_seq)
#define BOOST_LOG_WITH_PARAMS(logger, params_seq) BOOST_LOG_STREAM_WITH_PARAMS(logger, params_seq)

#endif // BOOST_LOG_NO_SHORTHAND_NAMES

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_SOURCES_RECORD_OSTREAM_HPP_INCLUDED_
