/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   attribute_value_extractor.hpp
 * \author Andrey Semashev
 * \date   01.03.2008
 *
 * The header contains implementation of convenience tools to extract an attribute value
 * into a user-defined functor.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_UTILITY_ATTRIBUTE_VALUE_EXTRACTOR_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_ATTRIBUTE_VALUE_EXTRACTOR_HPP_INCLUDED_

#include <typeinfo>
#include <string>
#include <boost/mpl/is_sequence.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/attributes/attribute.hpp>
#include <boost/log/attributes/attribute_values_view.hpp>
#include <boost/log/utility/type_dispatch/static_type_dispatcher.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

//! Fixed type attribute value extractor
template< typename CharT, typename T >
class fixed_type_value_extractor
{
public:
    //! Function object result type
    typedef bool result_type;

    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Attribute values view type
    typedef basic_attribute_values_view< char_type > values_view_type;
    //! Attribute value type
    typedef T value_type;

private:
    //! A simple type dispatcher to forward the extracted value to the receiver
    template< typename ReceiverT >
    struct dispatcher :
        public type_dispatcher,
        public type_visitor< value_type >
    {
        //! Constructor
        explicit dispatcher(ReceiverT& receiver) : m_Receiver(receiver) {}
        //! Returns itself if the value type matches the requested type
        void* get_visitor(std::type_info const& type)
        {
            BOOST_LOG_ASSUME(this != 0);
            if (type == typeid(value_type))
                return static_cast< type_visitor< value_type >* >(this);
            else
                return 0;
        }
        //! Extracts the value
        void visit(value_type const& value)
        {
            m_Receiver(value);
        }

    private:
        //! The reference to the receiver functional object
        ReceiverT& m_Receiver;
    };

private:
    //! Attribute name to extract
    string_type m_Name;

public:
    //! Constructor
    explicit fixed_type_value_extractor(string_type const& name) : m_Name(name) {}

    //! Extraction operator
    template< typename ReceiverT >
    result_type operator() (values_view_type const& attrs, ReceiverT& receiver) const
    {
        return extract(attrs, receiver);
    }
    //! Extraction operator
    template< typename ReceiverT >
    result_type operator() (values_view_type const& attrs, ReceiverT const& receiver) const
    {
        return extract(attrs, receiver);
    }

private:
    //! Implementation of the attribute value extraction
    template< typename ReceiverT >
    result_type extract(values_view_type const& attrs, ReceiverT& receiver) const
    {
        typename values_view_type::const_iterator it = attrs.find(m_Name);
        if (it != attrs.end())
        {
            dispatcher< ReceiverT > disp(receiver);
            return it->second->dispatch(disp);
        }
        return false;
    }
};

//! Attribute value extractor with type list support
template< typename CharT, typename TypeSequenceT >
class type_list_value_extractor
{
public:
    //! Function object result type
    typedef bool result_type;

    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Attribute values view type
    typedef basic_attribute_values_view< char_type > values_view_type;
    //! Attribute value types
    typedef TypeSequenceT value_types;

private:
#ifndef BOOST_LOG_NO_MEMBER_TEMPLATE_FRIENDS
    template< typename ReceiverT >
    struct visitor;
    template< typename ReceiverT >
    friend struct visitor;

    template< typename ReceiverT >
    class dispatcher;
    template< typename ReceiverT >
    friend class dispatcher;
#endif

    //! Visitor class definition
    template< typename ReceiverT >
    struct visitor
    {
        template< typename T >
        struct BOOST_LOG_NO_VTABLE apply :
            public type_visitor< T >
        {
            typedef apply< T > type;
            typedef typename type_visitor< T >::supported_type supported_type;

            //! Visitation (virtual)
            void visit(supported_type const& value)
            {
                BOOST_LOG_ASSUME(this != NULL);
                static_cast< dispatcher< ReceiverT >* >(this)->m_Receiver(value);
            }
        };
    };

    //! Type dispatcher class definition
    template< typename ReceiverT >
    class dispatcher :
        public static_type_dispatcher< value_types, visitor< ReceiverT > >
    {
    public:
        ReceiverT& m_Receiver;

        //! Dispatcher constructor
        explicit dispatcher(ReceiverT& receiver) : m_Receiver(receiver)
        {
        }
    };

private:
    //! Attribute name to extract
    string_type m_Name;

public:
    //! Constructor
    explicit type_list_value_extractor(string_type const& name) : m_Name(name) {}

    //! Extraction operator
    template< typename ReceiverT >
    result_type operator() (values_view_type const& attrs, ReceiverT& receiver) const
    {
        return extract(attrs, receiver);
    }
    //! Extraction operator
    template< typename ReceiverT >
    result_type operator() (values_view_type const& attrs, ReceiverT const& receiver) const
    {
        return extract(attrs, receiver);
    }

private:
    //! Implementation of the attribute value extraction
    template< typename ReceiverT >
    bool extract(values_view_type const& attrs, ReceiverT& receiver) const
    {
        typename values_view_type::const_iterator it = attrs.find(m_Name);
        if (it != attrs.end())
        {
            dispatcher< ReceiverT > disp(receiver);
            return it->second->dispatch(disp);
        }
        return false;
    }
};

} // namespace aux

/*!
 * \brief Generic attribute value extractor
 *
 * Attribute value extractor is a functional object that attempts to extract the stored
 * attribute value from the attribute value wrapper object. The extracted value is passed to
 * an unary functional object (the receiver) provided by user.
 *
 * The extractor can be specialized on one or several attribute value types that should be
 * specified in the second template argument.
 */
template< typename CharT, typename T >
class attribute_value_extractor :
    public mpl::if_<
        mpl::is_sequence< T >,
        boost::log::aux::type_list_value_extractor< CharT, T >,
        boost::log::aux::fixed_type_value_extractor< CharT, T >
    >::type
{
    //! Base type
    typedef typename mpl::if_<
        mpl::is_sequence< T >,
        boost::log::aux::type_list_value_extractor< CharT, T >,
        boost::log::aux::fixed_type_value_extractor< CharT, T >
    >::type base_type;

public:
    //! String type
    typedef typename base_type::string_type string_type;

public:
    /*!
     * Constructor
     *
     * \param name Attribute name to be extracted on invokation
     */
    explicit attribute_value_extractor(string_type const& name) : base_type(name) {}

#ifdef BOOST_LOG_DOXYGEN_PASS
    //! Function object result type
    typedef bool result_type;

    //! Character type
    typedef CharT char_type;
    //! Attribute values view type
    typedef basic_attribute_values_view< char_type > values_view_type;

    /*!
     * Extraction operator. Looks for an attribute value with the name specified on construction
     * and tries to acquire the stored value of one of the supported types. If extraction succeeds,
     * the extracted value is passed to \a receiver.
     *
     * \param attrs A set of attribute values in which to look for the specified attribute value.
     * \param receiver A receiving functional object to pass the extracted value to.
     * \return \c true if extraction succeeded, \c false otherwise
     */
    template< typename ReceiverT >
    result_type operator() (values_view_type const& attrs, ReceiverT& receiver) const;
#endif // BOOST_LOG_DOXYGEN_PASS
};

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be extracted.
 *
 * \pre <tt>name != NULL</tt>, \c name points to a zero-terminated string
 * \param name An attribute value name to extract.
 * \param attrs A set of attribute values in which to look for the specified attribute value.
 * \param receiver A receiving functional object to pass the extracted value to.
 * \return \c true if extraction succeeded, \c false otherwise
 */
template< typename T, typename CharT, typename ReceiverT >
inline bool extract(const CharT* name, basic_attribute_values_view< CharT > const& attrs, ReceiverT receiver)
{
    attribute_value_extractor< CharT, T > extractor(name);
    return extractor(attrs, receiver);
}

/*!
 * The function extracts an attribute value from the view. The user has to explicitly specify the
 * type or set of possible types of the attribute value to be extracted.
 *
 * \param name An attribute value name to extract.
 * \param attrs A set of attribute values in which to look for the specified attribute value.
 * \param receiver A receiving functional object to pass the extracted value to.
 * \return \c true if extraction succeeded, \c false otherwise
 */
template< typename T, typename CharT, typename ReceiverT >
inline bool extract(std::basic_string< CharT > const& name, basic_attribute_values_view< CharT > const& attrs, ReceiverT receiver)
{
    attribute_value_extractor< CharT, T > extractor(name);
    return extractor(attrs, receiver);
}

} // namespace log

} // namespace boost

#endif // BOOST_LOG_UTILITY_ATTRIBUTE_VALUE_EXTRACTOR_HPP_INCLUDED_
