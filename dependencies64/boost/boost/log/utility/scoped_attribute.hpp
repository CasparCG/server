/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   scoped_attribute.hpp
 * \author Andrey Semashev
 * \date   13.05.2007
 *
 * The header contains definition of facilities to define scoped attributes.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_UTILITY_SCOPED_ATTRIBUTE_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_SCOPED_ATTRIBUTE_HPP_INCLUDED_

#include <boost/shared_ptr.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/core/core.hpp>
#include <boost/log/sources/basic_logger.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/utility/empty_deleter.hpp>
#include <boost/log/utility/no_unused_warnings.hpp>
#include <boost/log/utility/unique_identifier_name.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

    //! A base class for all scoped attribute classes
    class attribute_scope_guard
    {
    };

} // namespace aux

//! Scoped attribute guard type
typedef aux::attribute_scope_guard const& scoped_attribute;

namespace aux {

    //! A scoped logger attribute guard
    template< typename LoggerT >
    class scoped_logger_attribute :
        public attribute_scope_guard
    {
    private:
        //! Logger type
        typedef LoggerT logger_type;

    private:
        //! A reference to the logger
        mutable logger_type* m_pLogger;
        //! An iterator to the added attribute
        typename logger_type::attribute_set_type::iterator m_itAttribute;

    public:
        //! Constructor
        scoped_logger_attribute(
            logger_type& l,
            typename logger_type::string_type const& name,
            shared_ptr< attribute > const& attr
        ) :
            m_pLogger(boost::addressof(l))
        {
            std::pair<
                typename logger_type::attribute_set_type::iterator,
                bool
            > res = l.add_attribute(name, attr);
            if (res.second)
                m_itAttribute = res.first;
            else
                m_pLogger = 0; // if there already is a same-named attribute, don't register anything
        }
        //! Copy constructor (implemented as move)
        scoped_logger_attribute(scoped_logger_attribute const& that) :
            m_pLogger(that.m_pLogger),
            m_itAttribute(that.m_itAttribute)
        {
            that.m_pLogger = 0;
        }

        //! Destructor
        ~scoped_logger_attribute()
        {
            if (m_pLogger)
            {
                m_pLogger->remove_attribute(m_itAttribute);
            }
        }

    private:
        //! Assignment (closed)
        scoped_logger_attribute& operator= (scoped_logger_attribute const&);
    };

} // namespace aux

//  Generator helper functions
/*!
 * Registers an attribute in the logger
 *
 * \param l Logger to register the attribute in
 * \param name Attribute name
 * \param attr Pointer to the attribute. Must not be NULL.
 * \return An unspecified guard object that may be used to initialize \c scoped_attribute variable.
 */
template< typename LoggerT >
inline aux::scoped_logger_attribute< LoggerT > add_scoped_logger_attribute(
    LoggerT& l, typename LoggerT::string_type const& name, shared_ptr< attribute > const& attr)
{
    return aux::scoped_logger_attribute< LoggerT >(l, name, attr);
}

#ifndef BOOST_LOG_DOXYGEN_PASS

template< typename LoggerT >
inline aux::scoped_logger_attribute< LoggerT > add_scoped_logger_attribute(
    LoggerT& l, typename LoggerT::char_type const* name, shared_ptr< attribute > const& attr)
{
    return aux::scoped_logger_attribute< LoggerT >(l, name, attr);
}

template< typename LoggerT, typename AttributeT >
inline aux::scoped_logger_attribute< LoggerT > add_scoped_logger_attribute(
    LoggerT& l, typename LoggerT::string_type const& name, AttributeT& attr)
{
    return aux::scoped_logger_attribute< LoggerT >(
        l, name, shared_ptr< attribute >(boost::addressof(attr), empty_deleter()));
}

template< typename LoggerT, typename AttributeT >
inline aux::scoped_logger_attribute< LoggerT > add_scoped_logger_attribute(
    LoggerT& l, typename LoggerT::char_type const* name, AttributeT& attr)
{
    return aux::scoped_logger_attribute< LoggerT >(
        l, name, shared_ptr< attribute >(boost::addressof(attr), empty_deleter()));
}

#endif // BOOST_LOG_DOXYGEN_PASS

//! \cond

#define BOOST_LOG_SCOPED_LOGGER_ATTR_CTOR_INTERNAL(logger, attr_name, attr_type, attr_ctor_args, attr_var_name, sentry_var_name)\
    attr_type attr_var_name(BOOST_PP_SEQ_ENUM(attr_ctor_args));\
    ::boost::log::scoped_attribute sentry_var_name =\
        ::boost::log::add_scoped_logger_attribute(logger, attr_name, attr_var_name);\
    BOOST_LOG_NO_UNUSED_WARNINGS(sentry_var_name)

#define BOOST_LOG_SCOPED_LOGGER_ATTR_INTERNAL(logger, attr_name, attr_type, attr_var_name, sentry_var_name)\
    attr_type attr_var_name;\
    ::boost::log::scoped_attribute sentry_var_name =\
        ::boost::log::add_scoped_logger_attribute(logger, attr_name, attr_var_name);\
    BOOST_LOG_NO_UNUSED_WARNINGS(sentry_var_name)

//! \endcond

//! The macro sets a scoped logger-wide attribute with constructor arguments in a more compact way
#define BOOST_LOG_SCOPED_LOGGER_ATTR_CTOR(logger, attr_name, attr_type, attr_ctor_args)\
    BOOST_LOG_SCOPED_LOGGER_ATTR_CTOR_INTERNAL(\
        logger,\
        attr_name,\
        attr_type,\
        attr_ctor_args,\
        BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_scoped_logger_attr_),\
        BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_scoped_logger_attr_sentry_))

//! The macro sets a scoped logger-wide attribute in a more compact way
#define BOOST_LOG_SCOPED_LOGGER_ATTR(logger, attr_name, attr_type)\
    BOOST_LOG_SCOPED_LOGGER_ATTR_INTERNAL(\
        logger,\
        attr_name,\
        attr_type,\
        BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_scoped_logger_attr_),\
        BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_scoped_logger_attr_sentry_))

//! The macro sets a scoped logger-wide tag in a more compact way
#define BOOST_LOG_SCOPED_LOGGER_TAG(logger, attr_name, attr_type, attr_value)\
    BOOST_LOG_SCOPED_LOGGER_ATTR_CTOR(logger, attr_name, ::boost::log::attributes::constant< attr_type >, (attr_value))

namespace aux {

    //! A scoped thread-specific attribute guard
    template< typename CharT >
    class scoped_thread_attribute :
        public attribute_scope_guard
    {
    private:
        //! Logging core type
        typedef basic_core< CharT > core_type;

    private:
        //! A pointer to the logging core
        mutable shared_ptr< core_type > m_pCore;
        //! An iterator to the added attribute
        typename core_type::attribute_set_type::iterator m_itAttribute;

    public:
        //! Constructor
        scoped_thread_attribute(
            typename core_type::string_type const& name, shared_ptr< attribute > const& attr) :
            m_pCore(core_type::get())
        {
            std::pair<
                typename core_type::attribute_set_type::iterator,
                bool
            > res = m_pCore->add_thread_attribute(name, attr);
            if (res.second)
                m_itAttribute = res.first;
            else
                m_pCore.reset(); // if there already is a same-named attribute, don't register anything
        }
        //! Copy constructor (implemented as move)
        scoped_thread_attribute(scoped_thread_attribute const& that) : m_itAttribute(that.m_itAttribute)
        {
            m_pCore.swap(that.m_pCore);
        }

        //! Destructor
        ~scoped_thread_attribute()
        {
            if (m_pCore)
            {
                m_pCore->remove_thread_attribute(m_itAttribute);
            }
        }

    private:
        //! Assignment (closed)
        scoped_thread_attribute& operator= (scoped_thread_attribute const&);
    };

} // namespace aux

//  Generator helper functions
/*!
 * Registers a thread-specific attribute
 *
 * \param name Attribute name
 * \param attr Pointer to the attribute. Must not be NULL.
 * \return An unspecified guard object that may be used to initialize \c scoped_attribute variable.
 */
template< typename CharT >
inline aux::scoped_thread_attribute< CharT > add_scoped_thread_attribute(
    std::basic_string< CharT > const& name, shared_ptr< attribute > const& attr)
{
    return aux::scoped_thread_attribute< CharT >(name, attr);
}

#ifndef BOOST_LOG_DOXYGEN_PASS

template< typename CharT >
inline aux::scoped_thread_attribute< CharT > add_scoped_thread_attribute(
    const CharT* name, shared_ptr< attribute > const& attr)
{
    return aux::scoped_thread_attribute< CharT >(name, attr);
}

template< typename CharT, typename AttributeT >
inline aux::scoped_thread_attribute< CharT > add_scoped_thread_attribute(
    std::basic_string< CharT > const& name, AttributeT& attr)
{
    return aux::scoped_thread_attribute< CharT >(
        name, shared_ptr< attribute >(boost::addressof(attr), empty_deleter()));
}

template< typename CharT, typename AttributeT >
inline aux::scoped_thread_attribute< CharT > add_scoped_thread_attribute(
    const CharT* name, AttributeT& attr)
{
    return aux::scoped_thread_attribute< CharT >(
        name, shared_ptr< attribute >(boost::addressof(attr), empty_deleter()));
}

#endif // BOOST_LOG_DOXYGEN_PASS

//! \cond

#define BOOST_LOG_SCOPED_THREAD_ATTR_CTOR_INTERNAL(attr_name, attr_type, attr_ctor_args, attr_var_name, sentry_var_name)\
    attr_type attr_var_name(BOOST_PP_SEQ_ENUM(attr_ctor_args));\
    ::boost::log::scoped_attribute sentry_var_name =\
        ::boost::log::add_scoped_thread_attribute(attr_name, attr_var_name);\
    BOOST_LOG_NO_UNUSED_WARNINGS(sentry_var_name)

#define BOOST_LOG_SCOPED_THREAD_ATTR_INTERNAL(attr_name, attr_type, attr_var_name, sentry_var_name)\
    attr_type attr_var_name;\
    ::boost::log::scoped_attribute sentry_var_name =\
        ::boost::log::add_scoped_thread_attribute(attr_name, attr_var_name);\
    BOOST_LOG_NO_UNUSED_WARNINGS(sentry_var_name)

//! \endcond

//! The macro sets a scoped thread-wide attribute with constructor arguments in a more compact way
#define BOOST_LOG_SCOPED_THREAD_ATTR_CTOR(attr_name, attr_type, attr_ctor_args)\
    BOOST_LOG_SCOPED_THREAD_ATTR_CTOR_INTERNAL(\
        attr_name,\
        attr_type,\
        attr_ctor_args,\
        BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_scoped_thread_attr_),\
        BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_scoped_thread_attr_sentry_))

//! The macro sets a scoped thread-wide attribute in a more compact way
#define BOOST_LOG_SCOPED_THREAD_ATTR(attr_name, attr_type)\
    BOOST_LOG_SCOPED_THREAD_ATTR_INTERNAL(\
        attr_name,\
        attr_type,\
        BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_scoped_thread_attr_),\
        BOOST_LOG_UNIQUE_IDENTIFIER_NAME(_boost_log_scoped_thread_attr_sentry_))

//! The macro sets a scoped thread-wide tag in a more compact way
#define BOOST_LOG_SCOPED_THREAD_TAG(attr_name, attr_type, attr_value)\
    BOOST_LOG_SCOPED_THREAD_ATTR_CTOR(attr_name, ::boost::log::attributes::constant< attr_type >, (attr_value))

} // namespace log

} // namespace boost

#endif // BOOST_LOG_UTILITY_SCOPED_ATTRIBUTE_HPP_INCLUDED_
