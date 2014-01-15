/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   channel_feature.hpp
 * \author Andrey Semashev
 * \date   28.02.2008
 *
 * The header contains implementation of a channel support feature.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SOURCES_CHANNEL_FEATURE_HPP_INCLUDED_
#define BOOST_LOG_SOURCES_CHANNEL_FEATURE_HPP_INCLUDED_

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/if.hpp>
#include <boost/parameter/binding.hpp>
#include <boost/type_traits/is_void.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/keywords/channel.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/sources/threading_models.hpp> // strictest_lock
#if !defined(BOOST_LOG_NO_THREADS)
#include <boost/thread/locks.hpp>
#endif // !defined(BOOST_LOG_NO_THREADS)

#ifdef _MSC_VER
#pragma warning(push)
// 'm_A' : class 'A' needs to have dll-interface to be used by clients of class 'B'
#pragma warning(disable: 4251)
#endif // _MSC_VER

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace sources {

namespace aux {

    //! A helper traits to get channel attribute name constant in the proper type
    template< typename >
    struct channel_attribute_name;

#ifdef BOOST_LOG_USE_CHAR
    template< >
    struct channel_attribute_name< char >
    {
        static const char* get() { return "Channel"; }
    };
#endif
#ifdef BOOST_LOG_USE_WCHAR_T
    template< >
    struct channel_attribute_name< wchar_t >
    {
        static const wchar_t* get() { return L"Channel"; }
    };
#endif

} // namespace aux

/*!
 * \brief Channel feature implementation
 */
template< typename BaseT, typename ChannelT >
class basic_channel_logger :
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
    //! String type
    typedef typename base_type::string_type string_type;
    //! Threading model being used
    typedef typename base_type::threading_model threading_model;

    //! Channel type
    typedef ChannelT channel_type;
    //! Channel attribute type
    typedef attributes::constant< channel_type > channel_attribute;

private:
    //! Channel attribute
    shared_ptr< channel_attribute > m_pChannel;

public:
    /*!
     * Default constructor. The constructed logger does not have the channel attribute.
     */
    basic_channel_logger() : base_type()
    {
    }
    /*!
     * Copy constructor
     */
    basic_channel_logger(basic_channel_logger const& that) :
        base_type(static_cast< base_type const& >(that)),
        m_pChannel(that.m_pChannel)
    {
    }
    /*!
     * Constructor with arguments. Allows to register a channel name attribute on construction.
     *
     * \param args A set of named arguments. The following arguments are supported:
     *             \li \c channel - a string that represents the channel name
     */
    template< typename ArgsT >
    explicit basic_channel_logger(ArgsT const& args) :
        base_type(args)
    {
        init_channel_attribute(args, typename is_void<
            typename parameter::binding< ArgsT, keywords::tag::channel, void >::type
        >::type());
    }

    /*!
     * The observer of the channel attribute presence
     *
     * \return \c true if the channel attribute is set by the logger, \c false otherwise
     */
    bool has_channel() const { return !!m_pChannel; }

    /*!
     * The observer of the channel name
     *
     * \pre <tt>this->has_channel() == true</tt>
     * \return The channel name that was set by the logger
     */
    channel_type const& channel() const { return m_pChannel->get(); }

protected:
    /*!
     * Channel attribute accessor
     */
    shared_ptr< channel_attribute > const& get_channel_attribute() const { return m_pChannel; }

    //! Lock requirement for the swap_unlocked method
    typedef typename strictest_lock<
        typename base_type::swap_lock,
#ifndef BOOST_LOG_NO_THREADS
        lock_guard< threading_model >
#else
        no_lock
#endif // !defined(BOOST_LOG_NO_THREADS)
    >::type swap_lock;

    /*!
     * Unlocked swap
     */
    void swap_unlocked(basic_channel_logger& that)
    {
        base_type::swap_unlocked(static_cast< base_type& >(that));
        m_pChannel.swap(that.m_pChannel);
    }

private:
#ifndef BOOST_LOG_DOXYGEN_PASS
    //! Initializes the channel attribute
    template< typename ArgsT >
    void init_channel_attribute(ArgsT const& args, mpl::false_ const&)
    {
        channel_type channel_name(args[keywords::channel]);
        m_pChannel = boost::make_shared< channel_attribute >(channel_name);
        base_type::add_attribute_unlocked(
            aux::channel_attribute_name< char_type >::get(),
            m_pChannel);
    }
    //! Initializes the channel attribute (dummy, if no channel is specified)
    template< typename ArgsT >
    void init_channel_attribute(ArgsT const& args, mpl::true_ const&)
    {
    }
#endif // BOOST_LOG_DOXYGEN_PASS
};

/*!
 * \brief Channel support feature
 *
 * The logger with this feature automatically registers constant attribute with the specified
 * on construction value, which is a channel name. The channel name cannot be modified
 * through the logger life time.
 *
 * The type of the channel name can be customized by providing it as a template parameter
 * to the feature template. By default, a string with the corresponding character type will be used.
 */
template< typename ChannelT = void >
struct channel
{
    template< typename BaseT >
    struct apply
    {
        typedef basic_channel_logger<
            BaseT,
            typename mpl::if_<
                is_void< ChannelT >,
                typename BaseT::string_type,
                ChannelT
            >::type
        > type;
    };
};

} // namespace sources

} // namespace log

} // namespace boost

#ifdef _MSC_VER
#pragma warning(pop)
#endif // _MSC_VER

#endif // BOOST_LOG_SOURCES_CHANNEL_FEATURE_HPP_INCLUDED_
