/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   severity_feature.hpp
 * \author Andrey Semashev
 * \date   08.03.2007
 *
 * The header contains implementation of a severity level support feature.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SOURCES_SEVERITY_FEATURE_HPP_INCLUDED_
#define BOOST_LOG_SOURCES_SEVERITY_FEATURE_HPP_INCLUDED_

#include <algorithm> // swap
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/log/detail/prologue.hpp>
#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/thread/locks.hpp>
#endif
#include <boost/log/sources/threading_models.hpp> // strictest_lock
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/basic_attribute_value.hpp>
#include <boost/log/keywords/severity.hpp>

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
// non dll-interface class 'A' used as base for dll-interface class 'B'
#pragma warning(disable: 4275)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace sources {

namespace aux {

    //! A helper traits to get severity attribute name constant in the proper type
    template< typename >
    struct severity_attribute_name;

#ifdef BOOST_LOG_USE_CHAR
    template< >
    struct severity_attribute_name< char >
    {
        static const char* get() { return "Severity"; }
    };
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
    template< >
    struct severity_attribute_name< wchar_t >
    {
        static const wchar_t* get() { return L"Severity"; }
    };
#endif

    //! The method returns the severity level for the current thread
    BOOST_LOG_EXPORT int get_severity_level();
    //! The method sets the severity level for the current thread
    BOOST_LOG_EXPORT void set_severity_level(int level);

    //! Severity level attribute implementation
    template< typename LevelT >
    class severity_level :
        public attribute,
        public attribute_value,
        public enable_shared_from_this< severity_level< LevelT > >
    {
    public:
        //! Stored level type
        typedef LevelT held_type;

    public:
        //! The method returns the actual attribute value. It must not return NULL.
        virtual shared_ptr< attribute_value > get_value()
        {
            return this->shared_from_this();
        }
        //! The method sets the actual level
        void set_value(held_type level)
        {
            set_severity_level(static_cast< int >(level));
        }

        //! The method dispatches the value to the given object
        virtual bool dispatch(type_dispatcher& dispatcher)
        {
            register type_visitor< held_type >* visitor =
                dispatcher.get_visitor< held_type >();
            if (visitor)
            {
                visitor->visit(static_cast< held_type >(get_severity_level()));
                return true;
            }
            else
                return false;
        }

        //! The method is called when the attribute value is passed to another thread
        virtual shared_ptr< attribute_value > detach_from_thread()
        {
#if !defined(BOOST_LOG_NO_THREADS)
            return boost::make_shared<
                attributes::basic_attribute_value< held_type >
            >(static_cast< held_type >(get_severity_level()));
#else
            // With multithreading disabled we may safely return this here. This method will not be called anyway.
            return this->shared_from_this();
#endif
        }
    };

} // namespace aux

/*!
 * \brief Severity level feature implementation
 */
template< typename BaseT, typename LevelT = int >
class basic_severity_logger :
    public BaseT
{
    //! Base type
    typedef BaseT base_type;

public:
    //! Character type
    typedef typename base_type::char_type char_type;
    //! Final type
    typedef typename base_type::final_type final_type;
    //! Attribute set type
    typedef typename base_type::attribute_set_type attribute_set_type;
    //! Threading model being used
    typedef typename base_type::threading_model threading_model;
    //! Log record type
    typedef typename base_type::record_type record_type;

    //! Severity level type
    typedef LevelT severity_level;
    //! Severity attribute type
    typedef aux::severity_level< severity_level > severity_attribute;

    //! Lock requirement for the open_record_unlocked method
    typedef typename strictest_lock<
        typename base_type::open_record_lock,
        no_lock
    >::type open_record_lock;
    //! Lock requirement for the swap_unlocked method
    typedef typename strictest_lock<
        typename base_type::swap_lock,
#ifndef BOOST_LOG_NO_THREADS
        lock_guard< threading_model >
#else
        no_lock
#endif // !defined(BOOST_LOG_NO_THREADS)
    >::type swap_lock;

private:
    //! Default severity
    severity_level m_DefaultSeverity;
    //! Severity attribute
    shared_ptr< severity_attribute > m_pSeverity;

public:
    /*!
     * Default constructor. The constructed logger will have a severity attribute registered.
     * The default level for log records will be 0.
     */
    basic_severity_logger() :
        base_type(),
        m_DefaultSeverity(static_cast< severity_level >(0)),
        m_pSeverity(boost::make_shared< severity_attribute >())
    {
        base_type::add_attribute_unlocked(
            aux::severity_attribute_name< char_type >::get(),
            m_pSeverity);
    }
    /*!
     * Copy constructor
     */
    basic_severity_logger(basic_severity_logger const& that) :
        base_type(static_cast< base_type const& >(that)),
        m_DefaultSeverity(that.m_DefaultSeverity),
        m_pSeverity(that.m_pSeverity)
    {
        base_type::attributes()[aux::severity_attribute_name< char_type >::get()] = m_pSeverity;
    }
    /*!
     * Constructor with named arguments. Allows to setup the default level for log records.
     *
     * \param args A set of named arguments. The following arguments are supported:
     *             \li \c severity - default severity value
     */
    template< typename ArgsT >
    explicit basic_severity_logger(ArgsT const& args) :
        base_type(args),
        m_DefaultSeverity(args[keywords::severity | severity_level()]),
        m_pSeverity(boost::make_shared< severity_attribute >())
    {
        base_type::add_attribute_unlocked(
            aux::severity_attribute_name< char_type >::get(),
            m_pSeverity);
    }

    /*!
     * Default severity value getter
     */
    severity_level default_severity() const { return m_DefaultSeverity; }

protected:
    /*!
     * Severity attribute accessor
     */
    shared_ptr< severity_attribute > const& get_severity_attribute() const { return m_pSeverity; }

    /*!
     * Unlocked \c open_record
     */
    template< typename ArgsT >
    record_type open_record_unlocked(ArgsT const& args)
    {
        m_pSeverity->set_value(args[keywords::severity | m_DefaultSeverity]);
        return base_type::open_record_unlocked(args);
    }

    //! Unlocked \c swap
    void swap_unlocked(basic_severity_logger& that)
    {
        base_type::swap_unlocked(static_cast< base_type& >(that));
        using std::swap;
        swap(m_DefaultSeverity, that.m_DefaultSeverity);
        m_pSeverity.swap(that.m_pSeverity);
    }
};

/*!
 * \brief Severity level support feature
 *
 * The logger with this feature registers a special attribute with an integral value type on construction.
 * This attribute will provide severity level for each log record being made through the logger.
 * The severity level can be omitted on logging record construction, in which case the default
 * level will be used. The default level can also be customized by passing it to the logger constructor.
 *
 * The type of the severity level attribute can be specified as a template parameter for the feature
 * template. By default, \c int will be used.
 */
template< typename LevelT = int >
struct severity
{
    template< typename BaseT >
    struct apply
    {
        typedef basic_severity_logger<
            BaseT,
            LevelT
        > type;
    };
};

} // namespace sources

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

//! The macro allows to put a record with a specific severity level into log
#define BOOST_LOG_STREAM_SEV(logger, lvl)\
    BOOST_LOG_STREAM_WITH_PARAMS((logger), (::boost::log::keywords::severity = (lvl)))

#ifndef BOOST_LOG_NO_SHORTHAND_NAMES

//! An equivalent to BOOST_LOG_STREAM_SEV(logger, lvl)
#define BOOST_LOG_SEV(logger, lvl) BOOST_LOG_STREAM_SEV(logger, lvl)

#endif // BOOST_LOG_NO_SHORTHAND_NAMES

#endif // BOOST_LOG_SOURCES_SEVERITY_FEATURE_HPP_INCLUDED_
