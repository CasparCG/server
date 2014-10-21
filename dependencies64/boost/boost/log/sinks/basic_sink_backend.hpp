/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   basic_sink_backend.hpp
 * \author Andrey Semashev
 * \date   04.11.2007
 *
 * The header contains implementation of base classes for sink backends.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SINKS_BASIC_SINK_BACKEND_HPP_INCLUDED_
#define BOOST_LOG_SINKS_BASIC_SINK_BACKEND_HPP_INCLUDED_

#include <string>
#include <locale>
#include <ostream>
#include <boost/noncopyable.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/or.hpp>
#include <boost/mpl/assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/function/function2.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/cleanup_scope_guard.hpp>
#include <boost/log/detail/code_conversion.hpp>
#include <boost/log/sinks/threading_models.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/attributes/attribute_values_view.hpp>
#include <boost/log/detail/attachable_sstream_buf.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace sinks {

/*!
 * \brief Base class for a logging sink backend
 *
 * The \c basic_sink_backend class template defines a number of types that
 * all sink backends are required to define. All sink backends have to derive from the class.
 */
template< typename CharT, typename ThreadingModelTagT >
struct basic_sink_backend : noncopyable
{
    //! Character type
    typedef CharT char_type;
    //! String type to be used as a message text holder
    typedef std::basic_string< char_type > string_type;
    //! Attribute values view type
    typedef basic_attribute_values_view< char_type > values_view_type;
    //! Log record type
    typedef basic_record< char_type > record_type;

    //! Threading model tag
    typedef ThreadingModelTagT threading_model;
};

/*!
 * \brief A base class for a logging sink backend with message formatting support
 *
 * The \c basic_formatting_sink_backend class template implements logging record
 * formatting. Formatting requires storing auxiliary data, such as formatter and
 * formatting stream. This requires thread synchronization to be done in sink frontend.
 *
 * The class also supports performing encoding conversion in case if the sink backend
 * requires the formatted string in some fixed encoding (e.g. if underlying API
 * supports only narrow or wide characters). In order to perform conversion one
 * should specify the desired final character type in the \c TargetCharT template
 * parameter.
 */
template<
    typename CharT,
    typename TargetCharT = CharT,
    typename ThreadingModelTagT = frontend_synchronization_tag
>
class BOOST_LOG_NO_VTABLE basic_formatting_sink_backend :
    public basic_sink_backend< CharT, ThreadingModelTagT >
{
    typedef basic_sink_backend< CharT, ThreadingModelTagT > base_type;

public:
    //  Type imports from the base class
    typedef typename base_type::char_type char_type;
    typedef typename base_type::string_type string_type;
    typedef typename base_type::values_view_type values_view_type;
    typedef typename base_type::threading_model threading_model;
    typedef typename base_type::record_type record_type;

    //! Output stream type
    typedef std::basic_ostream< char_type > stream_type;
    //! Target character type
    typedef TargetCharT target_char_type;
    //! Target string type
    typedef std::basic_string< target_char_type > target_string_type;

private:
    //  This type of sink backends require synchronization on the frontend side
    BOOST_MPL_ASSERT((mpl::or_<
        is_model_supported< threading_model, single_thread_tag >,
        is_model_supported< threading_model, frontend_synchronization_tag >
    >));

    //! Stream buffer type
    typedef typename mpl::if_<
        is_same< char_type, target_char_type >,
        boost::log::aux::basic_ostringstreambuf< char_type >,
        boost::log::aux::converting_ostringstreambuf< char_type >
    >::type streambuf_type;

private:
    //! Formatted log record storage
    target_string_type m_FormattedRecord;
    //! Stream buffer to fill the storage
    streambuf_type m_StreamBuf;
    //! Formatting stream
    stream_type m_FormattingStream;

    //! Formatter functor
    boost::function2<
        void,
        stream_type&,
        record_type const&
    > m_Formatter;

public:
    /*!
     * Default constructor
     *
     * \post The sink backend base class is initialized. The formatter is not set.
     */
    basic_formatting_sink_backend() :
        m_StreamBuf(m_FormattedRecord),
        m_FormattingStream(&m_StreamBuf)
    {
        m_FormattingStream.exceptions(std::ios_base::badbit | std::ios_base::failbit);
    }
    /*!
     * Virtual destructor
     */
    virtual ~basic_formatting_sink_backend() {}

    /*!
     * The method sets formatter functional object
     *
     * \param fmt Formatter object
     */
    template< typename T >
    void set_formatter(T const& fmt)
    {
        m_Formatter = fmt;
    }
    /*!
     * The method resets the formatter. If the formatter is not set, the result of formatting
     * is equivalent to the log record message text.
     */
    void reset_formatter()
    {
        m_Formatter.clear();
    }

    /*!
     * The method returns the current locale used for formatting
     */
    std::locale getloc() const
    {
        return m_FormattingStream.getloc();
    }
    /*!
     * The method sets the locale used for formatting
     */
    std::locale imbue(std::locale const& loc)
    {
        return m_FormattingStream.imbue(loc);
    }

    /*!
     * The method formats the message and passes it to the to the sink implementation
     * by calling \c do_consume.
     *
     * \note Do not override in derived classes. Use \c do_consume method to process
     *       the formatted message in a sink-specific manner.
     *
     * \param record Log record to consume
     */
    void consume(record_type const& record)
    {
        boost::log::aux::cleanup_guard< stream_type > cleanup1(m_FormattingStream);
        boost::log::aux::cleanup_guard< streambuf_type > cleanup2(m_StreamBuf);
        boost::log::aux::cleanup_guard< target_string_type > cleanup3(m_FormattedRecord);

        // Perform the formatting
        if (!m_Formatter.empty())
            m_Formatter(m_FormattingStream, record);
        else
            m_FormattingStream << record.message();

        m_FormattingStream.flush();

        // Pass the formatted string to the backend implementation
        do_consume(record, m_FormattedRecord);
    }

protected:
    /*!
     * A backend-defined implementation of the formatted message processing
     *
     * \param record The original log record
     * \param formatted_message Formatted log record
     */
    virtual void do_consume(record_type const& record, target_string_type const& formatted_message) = 0;
};

} // namespace sinks

} // namespace log

} // namespace boost

#endif // BOOST_LOG_SINKS_BASIC_SINK_BACKEND_HPP_INCLUDED_
