/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   nt6_event_log_backend.cpp
 * \author Andrey Semashev
 * \date   07.11.2008
 *
 * The header contains a logging sink backend that uses Windows NT 6 (Vista/2008 Server) API
 * for signaling application events.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SINKS_NT6_EVENT_LOG_BACKEND_HPP_INCLUDED_
#define BOOST_LOG_SINKS_NT6_EVENT_LOG_BACKEND_HPP_INCLUDED_

#include <guiddef.h>
#include <string>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function/function1.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/filters/basic_filters.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sinks/attribute_mapping.hpp>
#include <boost/log/sinks/nt6_event_log_constants.hpp>
#include <boost/log/keywords/provider_id.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
// non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace experimental {

namespace sinks {

namespace etw {

    /*!
     * \brief Straightforward severity level mapping
     *
     * This type of mapping assumes that attribute with a particular name always
     * provides values that map directly onto the native levels. The mapping
     * simply returns the extracted attribute value converted to the native severity level.
     */
    template< typename CharT, typename AttributeValueT = int >
    class basic_direct_severity_mapping :
        public boost::log::sinks::basic_direct_mapping< CharT, level_t, AttributeValueT >
    {
        //! Base type
        typedef boost::log::sinks::basic_direct_mapping< CharT, level_t, AttributeValueT > base_type;

    public:
        //! String type
        typedef typename base_type::string_type string_type;

    public:
        /*!
         * Constructor
         *
         * \param name Attribute name
         */
        explicit basic_direct_severity_mapping(string_type const& name) :
            base_type(name, log_always)
        {
        }
    };

    /*!
     * \brief Customizable severity level mapping
     *
     * The class allows to setup a custom mapping between an attribute and native severity levels.
     * The mapping should be initialized similarly to the standard \c map container, by using
     * indexing operator and assignment.
     */
    template< typename CharT, typename AttributeValueT = int >
    class basic_custom_severity_mapping :
        public boost::log::sinks::basic_custom_mapping< CharT, level_t, AttributeValueT >
    {
        //! Base type
        typedef boost::log::sinks::basic_custom_mapping< CharT, level_t, AttributeValueT > base_type;

    public:
        //! String type
        typedef typename base_type::string_type string_type;

    public:
        /*!
         * Constructor
         *
         * \param name Attribute name
         */
        explicit basic_custom_severity_mapping(string_type const& name) :
            base_type(name, log_always)
        {
        }
    };

#ifdef BOOST_LOG_USE_CHAR

    /*!
     * \brief Straightforward severity level mapping
     *
     * This is a convenience template typedef over \c basic_direct_severity_mapping
     * for narrow-character logging.
     */
    template< typename AttributeValueT = int >
    class direct_severity_mapping :
        public basic_direct_severity_mapping< char, AttributeValueT >
    {
        //! Base type
        typedef basic_direct_severity_mapping< char, AttributeValueT > base_type;

    public:
        //! String type
        typedef typename base_type::string_type string_type;

    public:
        /*!
         * Constructor
         *
         * \param name Attribute name
         */
        explicit direct_severity_mapping(string_type const& name) : base_type(name)
        {
        }
    };

    /*!
     * \brief Customizable severity level mapping
     *
     * This is a convenience template typedef over \c basic_custom_severity_mapping
     * for narrow-character logging.
     */
    template< typename AttributeValueT = int >
    class custom_severity_mapping :
        public basic_custom_severity_mapping< char, AttributeValueT >
    {
        //! Base type
        typedef basic_custom_severity_mapping< char, AttributeValueT > base_type;

    public:
        //! String type
        typedef typename base_type::string_type string_type;

    public:
        /*!
         * Constructor
         *
         * \param name Attribute name
         */
        explicit custom_severity_mapping(string_type const& name) : base_type(name)
        {
        }
    };

#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T

    /*!
     * \brief Straightforward severity level mapping
     *
     * This is a convenience template typedef over \c basic_direct_severity_mapping
     * for wide-character logging.
     */
    template< typename AttributeValueT = int >
    class wdirect_severity_mapping :
        public basic_direct_severity_mapping< wchar_t, AttributeValueT >
    {
        //! Base type
        typedef basic_direct_severity_mapping< wchar_t, AttributeValueT > base_type;

    public:
        //! String type
        typedef typename base_type::string_type string_type;

    public:
        /*!
         * Constructor
         *
         * \param name Attribute name
         */
        explicit wdirect_severity_mapping(string_type const& name) : base_type(name)
        {
        }
    };

    /*!
     * \brief Customizable severity level mapping
     *
     * This is a convenience template typedef over \c basic_custom_severity_mapping
     * for wide-character logging.
     */
    template< typename AttributeValueT = int >
    class wcustom_severity_mapping :
        public basic_custom_severity_mapping< wchar_t, AttributeValueT >
    {
        //! Base type
        typedef basic_custom_severity_mapping< wchar_t, AttributeValueT > base_type;

    public:
        //! String type
        typedef typename base_type::string_type string_type;

    public:
        /*!
         * Constructor
         *
         * \param name Attribute name
         */
        explicit wcustom_severity_mapping(string_type const& name) : base_type(name)
        {
        }
    };

#endif // BOOST_LOG_USE_WCHAR_T

} // namespace etw

/*!
 * \brief An implementation of a logging sink backend that emits events into Windows NT event log
 *
 * The sink uses Windows NT 6 (Vista/2008 Server) and later event log API to emit events
 * to an event log. The sink acts as an event provider, it automatically registers itself in the
 * Windows registry as an event source. The backend performs message text formatting and passes
 * the composed message to the log, so there is no need to create and translate event manifest
 * files in order to use this sink. Also, since this sink does not use event resources,
 * the location of the library module is not critical for the events to be displayer correctly.
 *
 * The backend allows to customize mapping of application severity levels to the native Windows event levels.
 * This allows to write portable code even if OS-specific sinks, such as this one, are used.
 *
 * Another remarkable feature is a specific filter that the backend can produce. This filter will
 * check whether the particular event is going to be actually consumed by the event logging API, or will it be
 * ignored for there are no subscribers for it. This allows to perform filtering with greater accuracy
 * and elide unnecessary formatting of event messages.
 */
template< typename CharT >
class basic_simple_nt6_event_log_backend :
    public boost::log::sinks::basic_formatting_sink_backend< CharT, wchar_t >
{
    //! Base type
    typedef boost::log::sinks::basic_formatting_sink_backend< CharT, wchar_t > base_type;
    //! Implementation type
    struct implementation;

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! String type
    typedef typename base_type::string_type string_type;
    //! String type to be used as a message text holder
    typedef typename base_type::target_string_type target_string_type;
    //! Attribute values view type
    typedef typename base_type::values_view_type values_view_type;
    //! Log record type
    typedef typename base_type::record_type record_type;

    //! WinAPI severity level mapper type
    typedef boost::function1<
        etw::level_t,
        values_view_type const&
    > severity_mapper_type;

public:
    //! \cond
    //! A filter that checks whether the event will be consumed by ETW
    class event_enabled_filter;
    friend class event_enabled_filter;
    class event_enabled_filter :
        public boost::log::filters::basic_filter< char_type, event_enabled_filter >
    {
        friend class basic_simple_nt6_event_log_backend< char_type >;

    private:
        weak_ptr< implementation > m_pImpl;

    private:
        explicit event_enabled_filter(shared_ptr< implementation > const& impl);

    public:
        BOOST_LOG_EXPORT bool operator() (values_view_type const& values) const;
    };
    //! \endcond

private:
    //! Pointer to the backend implementation that hides various types from windows.h
    shared_ptr< implementation > m_pImpl;

public:
    /*!
     * Constructor. Registers event provider.
     *
     * \param provider_id GUID that is specific for the event provider. Typically, it will be specific for the application.
     *        If not specified, the result of the \c get_default_provider_id function is used.
     *
     * \note Since the default provider identifier is always constant, even across different
     *       applications, it is strongly advised to always specify the \c provider_id parameter.
     */
    BOOST_LOG_EXPORT basic_simple_nt6_event_log_backend(GUID const& provider_id = get_default_provider_id());
    /*!
     * Destructor. Unregisters event provider.
     */
    BOOST_LOG_EXPORT ~basic_simple_nt6_event_log_backend();

    /*!
     * \return A filter that checks whether an event will be consumed by ETW or not
     */
    BOOST_LOG_EXPORT event_enabled_filter get_event_enabled_filter() const;

    /*!
     * The method installs the function object that maps application severity levels to WinAPI levels
     */
    BOOST_LOG_EXPORT void set_severity_mapper(severity_mapper_type const& mapper);

    /*!
     * \returns Default provider GUID.
     *
     * \note The returned GUID is always constant, therefore it is recommended to always generate
     *       your own application-specific event provider GUID in order not to conflict with other applications.
     */
    BOOST_LOG_EXPORT static GUID const& get_default_provider_id();

private:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! The method puts the formatted message to the event log
    BOOST_LOG_EXPORT void do_consume(record_type const& record, target_string_type const& formatted_message);
#endif
};

#ifdef BOOST_LOG_USE_CHAR
typedef basic_simple_nt6_event_log_backend< char > simple_nt6_event_log_backend;      //!< Convenience typedef for narrow-character logging
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
typedef basic_simple_nt6_event_log_backend< wchar_t > wsimple_nt6_event_log_backend;  //!< Convenience typedef for wide-character logging
#endif

} // namespace sinks

} // namespace experimental

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_SINKS_NT6_EVENT_LOG_BACKEND_HPP_INCLUDED_
