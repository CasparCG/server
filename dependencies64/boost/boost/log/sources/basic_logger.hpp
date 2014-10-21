/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   basic_logger.hpp
 * \author Andrey Semashev
 * \date   08.03.2007
 *
 * The header contains implementation of a base class for loggers. Convenience macros
 * for defining custom loggers are also provided.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SOURCES_BASIC_LOGGER_HPP_INCLUDED_
#define BOOST_LOG_SOURCES_BASIC_LOGGER_HPP_INCLUDED_

#include <exception>
#include <string>
#include <utility>
#include <ostream>
#include <boost/assert.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/mpl/lambda.hpp>
#include <boost/mpl/reverse_fold.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/preprocessor/facilities/empty.hpp>
#include <boost/preprocessor/facilities/identity.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/parameter_tools.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/core/core.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/sources/features.hpp>
#include <boost/log/sources/threading_models.hpp>
#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/log/detail/multiple_lock.hpp>
#include <boost/log/detail/light_rw_mutex.hpp>
#endif // !defined(BOOST_LOG_NO_THREADS)

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace sources {

/*!
 * \brief Basic logger class
 *
 * The \c basic_logger class template serves as a base class for all loggers
 * provided by the library. It can also be used as a base for user-defined
 * loggers. The template parameters are:
 *
 * \li \c CharT - logging character type
 * \li \c FinalT - final type of the logger that eventually derives from
 *     the \c basic_logger. There may be other classes in the hierarchy
 *     between the final class and \c basic_logger.
 * \li \c ThreadingModelT - threading model policy. Must provide methods
 *     of the Boost.Thread locking concept used in \c basic_logger class
 *     and all its derivatives in the hierarchy up to the \c FinalT class.
 *     The \c basic_logger class itself requires methods of the
 *     SharedLockable concept. The threading model policy must also be
 *     default and copy-constructible and support member function \c swap.
 *     There are currently two policies provided: \c single_thread_model
 *     and \c multi_thread_model.
 *
 * The logger implements fundamental facilities of loggers, such as storing
 * source-specific attribute set and formatting log record messages. The basic
 * logger interacts with the logging core in order to apply filtering and
 * pass records to sinks.
 */
template< typename CharT, typename FinalT, typename ThreadingModelT >
class basic_logger :
    public ThreadingModelT
{
public:
    //! Character type
    typedef CharT char_type;
    //! Final logger type
    typedef FinalT final_type;

    //! String type to be used as a message text holder
    typedef std::basic_string< char_type > string_type;
    //! Attribute set type
    typedef basic_attribute_set< char_type > attribute_set_type;
    //! Logging system core type
    typedef basic_core< char_type > core_type;
    //! Logging record type
    typedef basic_record< char_type > record_type;
    //! Threading model type
    typedef ThreadingModelT threading_model;

#if !defined(BOOST_LOG_NO_THREADS)
    //! Lock requirement for the swap_unlocked method
    typedef lock_guard< threading_model > swap_lock;
    //! Lock requirement for the add_attribute_unlocked method
    typedef lock_guard< threading_model > add_attribute_lock;
    //! Lock requirement for the remove_attribute_unlocked method
    typedef lock_guard< threading_model > remove_attribute_lock;
    //! Lock requirement for the remove_all_attributes_unlocked method
    typedef lock_guard< threading_model > remove_all_attributes_lock;
    //! Lock requirement for the get_attributes method
    typedef boost::log::aux::shared_lock_guard< threading_model > get_attributes_lock;
    //! Lock requirement for the open_record_unlocked method
    typedef boost::log::aux::shared_lock_guard< threading_model > open_record_lock;
    //! Lock requirement for the set_attributes method
    typedef lock_guard< threading_model > set_attributes_lock;
#else
    typedef no_lock swap_lock;
    typedef no_lock add_attribute_lock;
    typedef no_lock remove_attribute_lock;
    typedef no_lock remove_all_attributes_lock;
    typedef no_lock get_attributes_lock;
    typedef no_lock open_record_lock;
    typedef no_lock set_attributes_lock;
#endif

private:
    //! A pointer to the logging system
    shared_ptr< core_type > m_pCore;

    //! Logger-specific attribute set
    attribute_set_type m_Attributes;

public:
    /*!
     * Constructor. Initializes internal data structures of the basic logger class,
     * acquires reference to the logging core.
     */
    basic_logger() :
        threading_model(),
        m_pCore(core_type::get())
    {
    }
    /*!
     * Copy constructor. Copies all attributes from the source logger.
     *
     * \note Not thread-safe. The source logger must be locked in the final class before copying.
     *
     * \param that Source logger
     */
    basic_logger(basic_logger const& that) :
        threading_model(),
        m_pCore(core_type::get()),
        m_Attributes(that.m_Attributes)
    {
    }
    /*!
     * Constructor with named arguments. The constructor ignores all arguments. The result of
     * construction is equivalent to default construction.
     */
    template< typename ArgsT >
    explicit basic_logger(ArgsT const&) :
        threading_model(),
        m_pCore(core_type::get())
    {
    }

protected:
    /*!
     * An accessor to the logging system pointer
     */
    shared_ptr< core_type > const& core() const { return m_pCore; }
    /*!
     * An accessor to the logger attributes
     */
    attribute_set_type& attributes() { return m_Attributes; }
    /*!
     * An accessor to the logger attributes
     */
    attribute_set_type const& attributes() const { return m_Attributes; }
    /*!
     * An accessor to the threading model base
     */
    threading_model& get_threading_model() { return *this; }
    /*!
     * An accessor to the threading model base
     */
    threading_model const& get_threading_model() const { return *this; }
    /*!
     * An accessor to the final logger
     */
    final_type* final_this()
    {
        BOOST_LOG_ASSUME(this != NULL);
        return static_cast< final_type* >(this);
    }
    /*!
     * An accessor to the final logger
     */
    final_type const* final_this() const
    {
        BOOST_LOG_ASSUME(this != NULL);
        return static_cast< final_type const* >(this);
    }

    /*!
     * Unlocked \c swap
     */
    void swap_unlocked(basic_logger& that)
    {
        get_threading_model().swap(that.get_threading_model());
        m_Attributes.swap(that.m_Attributes);
    }

    /*!
     * Unlocked \c add_attribute
     */
    std::pair< typename attribute_set_type::iterator, bool > add_attribute_unlocked(
        string_type const& name, shared_ptr< attribute > const& attr)
    {
        return m_Attributes.insert(std::make_pair(name, attr));
    }

    /*!
     * Unlocked \c remove_attribute
     */
    void remove_attribute_unlocked(typename attribute_set_type::iterator it)
    {
        m_Attributes.erase(it);
    }

    /*!
     * Unlocked \c remove_all_attributes
     */
    void remove_all_attributes_unlocked()
    {
        m_Attributes.clear();
    }

    /*!
     * Unlocked \c open_record
     */
    record_type open_record_unlocked()
    {
        return m_pCore->open_record(m_Attributes);
    }
    /*!
     * Unlocked \c open_record
     */
    template< typename ArgsT >
    record_type open_record_unlocked(ArgsT const& args)
    {
        return m_pCore->open_record(m_Attributes);
    }

    //! Lock requirement for the push_record_unlocked method
    typedef no_lock push_record_lock;

    /*!
     * Unlocked \c push_record
     */
    void push_record_unlocked(record_type const& record)
    {
        m_pCore->push_record(record);
    }

    /*!
     * Unlocked \c get_attributes
     */
    attribute_set_type get_attributes_unlocked() const
    {
        return m_Attributes;
    }

    /*!
     * Unlocked \c set_attributes
     */
    void set_attributes_unlocked(attribute_set_type const& attrs)
    {
        m_Attributes = attrs;
    }

private:
    //! Assignment (should be implemented through copy and swap in the final class)
    basic_logger& operator= (basic_logger const&);
};

/*!
 * Free-standing swap for all loggers
 */
template< typename CharT, typename FinalT, typename ThreadingModelT >
inline void swap(
    basic_logger< CharT, FinalT, ThreadingModelT >& left,
    basic_logger< CharT, FinalT, ThreadingModelT >& right)
{
    static_cast< FinalT& >(left).swap(static_cast< FinalT& >(right));
}

namespace aux {

/*!
 * \brief A helper metafunction that is used to inherit all logger features into the final logger
 */
struct inherit_logger_features
{
    template< typename PrevT, typename T >
    struct apply
    {
        typedef typename mpl::lambda< T >::type::BOOST_NESTED_TEMPLATE apply< PrevT >::type type;
    };
};

} // namespace aux

/*!
 * \brief A composite logger that inherits a number of features
 *
 * The composite logger is a helper class that simplifies feature composition into the final logger.
 * The user's logger class is expected to derive from the composite logger class, instantiated with
 * the character type, the user's logger class, the threading model and the list of the required features.
 * The former three parameters are passed to the \c basic_logger class template. The feature list
 * must be a MPL type sequence, where each element is an unary MPL metafunction class, that upon
 * applying on its argument results in a logging feature class that derives from the argument.
 * Every logger feature provided by the library can participate in the feature list.
 */
template< typename CharT, typename FinalT, typename ThreadingModelT, typename FeaturesT >
class basic_composite_logger :
    public mpl::reverse_fold<
        FeaturesT,
        basic_logger< CharT, FinalT, ThreadingModelT >,
        aux::inherit_logger_features
    >::type
{
    //! Base type (the hierarchy of features)
    typedef typename mpl::reverse_fold<
        FeaturesT,
        basic_logger< CharT, FinalT, ThreadingModelT >,
        aux::inherit_logger_features
    >::type base_type;

protected:
    //! The composite logger type (for use in the user's logger class)
    typedef basic_composite_logger logger_base;

public:
    //! Threading model being used
    typedef typename base_type::threading_model threading_model;
    //! Attribute set type
    typedef typename base_type::attribute_set_type attribute_set_type;
    //! String type
    typedef typename base_type::string_type string_type;
    //! Log record type
    typedef typename base_type::record_type record_type;

#if !defined(BOOST_LOG_NO_THREADS)

public:
    /*!
     * Default constructor (default-constructs all features)
     */
    basic_composite_logger() {}
    /*!
     * Copy constructor
     */
    basic_composite_logger(basic_composite_logger const& that) :
        base_type((
            boost::log::aux::shared_lock_guard< const threading_model >(that.get_threading_model()),
            static_cast< base_type const& >(that)
        ))
    {
    }
    /*!
     * Constructor with named parameters
     */
    template< typename ArgsT >
    explicit basic_composite_logger(ArgsT const& args) : base_type(args)
    {
    }

    /*!
     * Assignment for the final class. Threadsafe, provides strong exception guarantee.
     */
    FinalT& operator= (FinalT const& that)
    {
        BOOST_LOG_ASSUME(this != NULL);
        if (static_cast< FinalT* >(this) != boost::addressof(that))
        {
            // We'll have to explicitly create the copy in order to make sure it's unlocked when we attempt to lock *this
            FinalT tmp(that);
            lock_guard< threading_model > _(base_type::get_threading_model());
            base_type::swap_unlocked(tmp);
        }
        return static_cast< FinalT& >(*this);
    }

    /*!
     * The method adds an attribute to the source-specific attribute set. The attribute will be implicitly added to
     * every log record made with the current logger.
     *
     * \param name The attribute name.
     * \param attr Pointer to the attribute. Must not be NULL.
     * \return A pair of values. If the second member is \c true, then the attribute is added and the first member points to the
     *         attribute. Otherwise the attribute was not added and the first member points to the attribute that prevents
     *         addition.
     */
    std::pair< typename attribute_set_type::iterator, bool > add_attribute(
        string_type const& name, shared_ptr< attribute > const& attr)
    {
        typename base_type::add_attribute_lock _(base_type::get_threading_model());
        return base_type::add_attribute_unlocked(name, attr);
    }
    /*!
     * The method removes an attribute from the source-specific attribute set.
     *
     * \pre The attribute was added with the add_attribute call for this instance of the logger.
     * \post The attribute is no longer registered as a source-specific attribute for this logger. The iterator is invalidated after removal.
     *
     * \param it Iterator to the previously added attribute.
     */
    void remove_attribute(typename attribute_set_type::iterator it)
    {
        typename base_type::remove_attribute_lock _(base_type::get_threading_model());
        base_type::remove_attribute_unlocked(it);
    }

    /*!
     * The method removes all attributes from the logger. All iterators and references to the removed attributes are invalidated.
     */
    void remove_all_attributes()
    {
        typename base_type::remove_all_attributes_lock _(base_type::get_threading_model());
        base_type::remove_all_attributes_unlocked();
    }

    /*!
     * The method retrieves a copy of a set with all attributes from the logger.
     *
     * \return The copy of the attribute set. Attributes are shallow-copied.
     */
    attribute_set_type get_attributes() const
    {
        typename base_type::get_attributes_lock _(base_type::get_threading_model());
        return base_type::get_attributes_unlocked();
    }

    /*!
     * The method installs the whole attribute set into the logger. All iterators and references to elements of
     * the previous set are invalidated. Iterators to the \a attrs set are not valid to be used with the logger (that is,
     * the logger owns a copy of \a attrs after completion).
     *
     * \param attrs The set of attributes to install into the logger. Attributes are shallow-copied.
     */
    void set_attributes(attribute_set_type const& attrs)
    {
        typename base_type::set_attributes_lock _(base_type::get_threading_model());
        base_type::set_attributes_unlocked(attrs);
    }

    /*!
     * The method opens a new log record in the logging core.
     *
     * \return A valid record handle if the logging record is opened successfully, an invalid handle otherwise.
     */
    record_type open_record()
    {
        typename base_type::open_record_lock _(base_type::get_threading_model());
        return base_type::open_record_unlocked(boost::log::aux::empty_arg_list());
    }
    /*!
     * The method opens a new log record in the logging core.
     *
     * \param args A set of additional named arguments. The parameter is ignored.
     * \return A valid record handle if the logging record is opened successfully, an invalid handle otherwise.
     */
    template< typename ArgsT >
    record_type open_record(ArgsT const& args)
    {
        typename base_type::open_record_lock _(base_type::get_threading_model());
        return base_type::open_record_unlocked(args);
    }
    /*!
     * The method pushes the constructed message to the logging core
     *
     * \param record The log record with the formatted message
     */
    void push_record(record_type const& record)
    {
        typename base_type::push_record_lock _(base_type::get_threading_model());
        base_type::push_record_unlocked(record);
    }
    /*!
     * Thread-safe implementation of swap
     */
    void swap(basic_composite_logger& that)
    {
        boost::log::aux::multiple_unique_lock2<
            threading_model,
            threading_model
        > _(base_type::get_threading_model(), that.get_threading_model());
        base_type::swap_unlocked(that);
    }
};

//! An optimized composite logger version with no multithreading support
template< typename CharT, typename FinalT, typename FeaturesT >
class basic_composite_logger< CharT, FinalT, single_thread_model, FeaturesT > :
    public mpl::reverse_fold<
        FeaturesT,
        basic_logger< CharT, FinalT, single_thread_model >,
        aux::inherit_logger_features
    >::type
{
    typedef typename mpl::reverse_fold<
        FeaturesT,
        basic_logger< CharT, FinalT, single_thread_model >,
        aux::inherit_logger_features
    >::type base_type;

protected:
    typedef basic_composite_logger logger_base;

public:
    typedef typename base_type::threading_model threading_model;
    typedef typename base_type::attribute_set_type attribute_set_type;
    typedef typename base_type::string_type string_type;
    typedef typename base_type::record_type record_type;

#endif // !defined(BOOST_LOG_NO_THREADS)

public:
    basic_composite_logger() {}
    basic_composite_logger(basic_composite_logger const& that) :
        base_type(static_cast< base_type const& >(that))
    {
    }
    template< typename ArgsT >
    explicit basic_composite_logger(ArgsT const& args) : base_type(args)
    {
    }

    FinalT& operator= (FinalT that)
    {
        base_type::swap_unlocked(that);
        return static_cast< FinalT& >(*this);
    }

    std::pair< typename attribute_set_type::iterator, bool > add_attribute(
        string_type const& name, shared_ptr< attribute > const& attr)
    {
        return base_type::add_attribute_unlocked(name, attr);
    }
    void remove_attribute(typename attribute_set_type::iterator it)
    {
        base_type::remove_attribute_unlocked(it);
    }
    void remove_all_attributes()
    {
        base_type::remove_all_attributes_unlocked();
    }
    attribute_set_type get_attributes() const
    {
        return base_type::get_attributes_unlocked();
    }
    void set_attributes(attribute_set_type const& attrs)
    {
        base_type::set_attributes_unlocked(attrs);
    }
    record_type open_record()
    {
        return base_type::open_record_unlocked(boost::log::aux::empty_arg_list());
    }
    template< typename ArgsT >
    record_type open_record(ArgsT const& args)
    {
        return base_type::open_record_unlocked(args);
    }
    void push_record(record_type const& record)
    {
        base_type::push_record_unlocked(record);
    }
    void swap(basic_composite_logger& that)
    {
        base_type::swap_unlocked(that);
    }
};


//! \cond

#define BOOST_LOG_FORWARD_LOGGER_CONSTRUCTORS_IMPL(class_type, typename_keyword)\
    public:\
        class_type() {}\
        class_type(class_type const& that) : class_type::logger_base(\
            static_cast< typename_keyword() class_type::logger_base const& >(that)) {}\
        BOOST_LOG_PARAMETRIZED_CONSTRUCTORS_FORWARD(class_type, class_type::logger_base)

//! \endcond

#define BOOST_LOG_FORWARD_LOGGER_PARAMETRIZED_CONSTRUCTORS(class_type)\
    BOOST_LOG_FORWARD_LOGGER_PARAMETRIZED_CONSTRUCTORS_IMPL(class_type, BOOST_PP_EMPTY)

#define BOOST_LOG_FORWARD_LOGGER_PARAMETRIZED_CONSTRUCTORS_TEMPLATE(class_type)\
    BOOST_LOG_FORWARD_LOGGER_PARAMETRIZED_CONSTRUCTORS_IMPL(class_type, BOOST_PP_IDENTITY(typename))

#define BOOST_LOG_FORWARD_LOGGER_CONSTRUCTORS(class_type)\
    BOOST_LOG_FORWARD_LOGGER_CONSTRUCTORS_IMPL(class_type, BOOST_PP_EMPTY)

#define BOOST_LOG_FORWARD_LOGGER_CONSTRUCTORS_TEMPLATE(class_type)\
    BOOST_LOG_FORWARD_LOGGER_CONSTRUCTORS_IMPL(class_type, BOOST_PP_IDENTITY(typename))


#ifdef BOOST_LOG_USE_CHAR

/*!
 * \brief Narrow-char logger. Functionally equivalent to \c basic_logger.
 *
 * See \c basic_logger class template for a more detailed description.
 */
class logger :
    public basic_composite_logger< char, logger, single_thread_model, features< >::type >
{
    BOOST_LOG_FORWARD_LOGGER_CONSTRUCTORS(logger)
};

#if !defined(BOOST_LOG_NO_THREADS)

/*!
 * \brief Narrow-char thread-safe logger. Functionally equivalent to \c basic_logger.
 *
 * See \c basic_logger class template for a more detailed description.
 */
class logger_mt :
    public basic_composite_logger< char, logger_mt, multi_thread_model< boost::log::aux::light_rw_mutex >, features< >::type >
{
    BOOST_LOG_FORWARD_LOGGER_CONSTRUCTORS(logger_mt)
};

#endif // !defined(BOOST_LOG_NO_THREADS)
#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T

/*!
 * \brief Wide-char logger. Functionally equivalent to \c basic_logger.
 *
 * See \c basic_logger class template for a more detailed description.
 */
class wlogger :
    public basic_composite_logger< wchar_t, wlogger, single_thread_model, features< >::type >
{
    BOOST_LOG_FORWARD_LOGGER_CONSTRUCTORS(wlogger)
};

#if !defined(BOOST_LOG_NO_THREADS)

/*!
 * \brief Wide-char thread-safe logger. Functionally equivalent to \c basic_logger.
 *
 * See \c basic_logger class template for a more detailed description.
 */
class wlogger_mt :
    public basic_composite_logger< wchar_t, wlogger_mt, multi_thread_model< boost::log::aux::light_rw_mutex >, features< >::type >
{
    BOOST_LOG_FORWARD_LOGGER_CONSTRUCTORS(wlogger_mt)
};

#endif // !defined(BOOST_LOG_NO_THREADS)
#endif // BOOST_LOG_USE_WCHAR_T

} // namespace sources

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

/*!
 *  \brief The macro declares a logger class that inherits a number of base classes
 *
 *  \param type_name The name of the logger class to declare
 *  \param char_type The character type of the logger. Either char or wchar_t expected.
 *  \param base_seq A Boost.Preprocessor sequence of type identifiers of the base classes templates
 *  \param threading A threading model class
 */
#define BOOST_LOG_DECLARE_LOGGER_TYPE(type_name, char_type, base_seq, threading)\
    class type_name :\
        public ::boost::log::sources::basic_composite_logger<\
            char_type,\
            type_name,\
            threading,\
            ::boost::log::sources::features< BOOST_PP_SEQ_ENUM(base_seq) >::type\
        >\
    {\
        BOOST_LOG_FORWARD_LOGGER_CONSTRUCTORS(type_name)\
    }



#ifdef BOOST_LOG_USE_CHAR

/*!
 *  \brief The macro declares a narrow-char logger class that inherits a number of base classes
 *
 *  Equivalent to BOOST_LOG_DECLARE_LOGGER_TYPE(type_name, char, base_seq, single_thread_model)
 *
 *  \param type_name The name of the logger class to declare
 *  \param base_seq A Boost.Preprocessor sequence of type identifiers of the base classes templates
 */
#define BOOST_LOG_DECLARE_LOGGER(type_name, base_seq)\
    BOOST_LOG_DECLARE_LOGGER_TYPE(type_name, char, base_seq, ::boost::log::sources::single_thread_model)

#if !defined(BOOST_LOG_NO_THREADS)

/*!
 *  \brief The macro declares a narrow-char thread-safe logger class that inherits a number of base classes
 *
 *  Equivalent to <tt>BOOST_LOG_DECLARE_LOGGER_TYPE(type_name, char, base_seq, multi_thread_model< shared_mutex >)</tt>
 *
 *  \param type_name The name of the logger class to declare
 *  \param base_seq A Boost.Preprocessor sequence of type identifiers of the base classes templates
 */
#define BOOST_LOG_DECLARE_LOGGER_MT(type_name, base_seq)\
    BOOST_LOG_DECLARE_LOGGER_TYPE(type_name, char, base_seq, ::boost::log::sources::multi_thread_model< ::boost::shared_mutex >)

#endif // !defined(BOOST_LOG_NO_THREADS)
#endif // BOOST_LOG_USE_CHAR

#ifdef BOOST_LOG_USE_WCHAR_T

/*!
 *  \brief The macro declares a wide-char logger class that inherits a number of base classes
 *
 *  Equivalent to BOOST_LOG_DECLARE_LOGGER_TYPE(type_name, wchar_t, base_seq, single_thread_model)
 *
 *  \param type_name The name of the logger class to declare
 *  \param base_seq A Boost.Preprocessor sequence of type identifiers of the base classes templates
 */
#define BOOST_LOG_DECLARE_WLOGGER(type_name, base_seq)\
    BOOST_LOG_DECLARE_LOGGER_TYPE(type_name, wchar_t, base_seq, ::boost::log::sources::single_thread_model)

#if !defined(BOOST_LOG_NO_THREADS)

/*!
 *  \brief The macro declares a wide-char thread-safe logger class that inherits a number of base classes
 *
 *  Equivalent to <tt>BOOST_LOG_DECLARE_LOGGER_TYPE(type_name, wchar_t, base_seq, multi_thread_model< shared_mutex >)</tt>
 *
 *  \param type_name The name of the logger class to declare
 *  \param base_seq A Boost.Preprocessor sequence of type identifiers of the base classes templates
 */
#define BOOST_LOG_DECLARE_WLOGGER_MT(type_name, base_seq)\
    BOOST_LOG_DECLARE_LOGGER_TYPE(type_name, wchar_t, base_seq, ::boost::log::sources::multi_thread_model< ::boost::shared_mutex >)

#endif // !defined(BOOST_LOG_NO_THREADS)
#endif // BOOST_LOG_USE_WCHAR_T

#endif // BOOST_LOG_SOURCES_BASIC_LOGGER_HPP_INCLUDED_
