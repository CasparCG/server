/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   record_ordering.hpp
 * \author Andrey Semashev
 * \date   23.08.2009
 *
 * This header contains ordering predicates for logging records.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_UTILITY_RECORD_ORDERING_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_RECORD_ORDERING_HPP_INCLUDED_

#include <boost/optional.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/functional.hpp>
#include <boost/log/detail/function_traits.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/attributes/attribute.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

/*!
 * \brief Ordering predicate, based on log record handle comparison
 *
 * This predicate offers a quick log record ordering based on the log record handles.
 * It is not specified whether one record is less than another, in terms of the
 * predicate, until the actual comparison is performed. Moreover, the ordering may
 * change every time the application runs.
 *
 * This kind of ordering may be useful if log records are to be stored in an associative
 * container with as least performance overhead as possible.
 */
template< typename FunT = aux::less >
class handle_ordering :
    private FunT
{
public:
    //! Result type
    typedef bool result_type;

public:
    /*!
     * Default constructor. Requires \c FunT to be default constructible.
     */
    handle_ordering() : FunT()
    {
    }
    /*!
     * Initializing constructor. Constructs \c FunT instance as a copy of the \a fun argument.
     */
    explicit handle_ordering(FunT const& fun) : FunT(fun)
    {
    }

    /*!
     * Ordering operator
     */
    template< typename CharT >
    result_type operator() (basic_record< CharT > const& left, basic_record< CharT > const& right) const
    {
        return FunT::operator() (left.handle().get(), right.handle().get());
    }
};

/*!
 * \brief Ordering predicate, based on attribute values associated with records
 *
 * This predicate allows to order log records based on values of a specificly named attribute
 * associated with them. Two given log records being compared should both have the specified
 * attribute value of the specified type to be able to be ordered properly. As a special case,
 * if neither of the records have the value, these records are considered equivalent. Otherwise,
 * the ordering results are unspecified.
 */
template< typename CharT, typename ValueT, typename FunT = aux::less >
class attribute_value_ordering :
    private FunT
{
public:
    //! Result type
    typedef bool result_type;
    //! Character type
    typedef CharT char_type;
    //! Log record type
    typedef basic_record< char_type > record_type;
    //! String type
    typedef typename record_type::string_type string_type;
    //! Compared attribute value type
    typedef ValueT attribute_value_type;

private:
    //! Attribute value name
    const string_type m_Name;

public:
    /*!
     * Initializing constructor.
     *
     * \param name The attribute value name to be compared
     * \param fun The ordering functor
     */
    attribute_value_ordering(string_type const& name, FunT const& fun = FunT()) : FunT(fun), m_Name(name)
    {
    }

    /*!
     * Ordering operator
     */
    result_type operator() (record_type const& left, record_type const& right) const
    {
        optional< attribute_value_type > left_value, right_value;

        typedef typename record_type::values_view_type values_view_type;
        typename values_view_type::const_iterator
            it_left = left.attribute_values().find(m_Name),
            it_right = right.attribute_values().find(m_Name);
        if (it_left != left.attribute_values().end())
            left_value = it_left->second->get< attribute_value_type >();
        if (it_right != right.attribute_values().end())
            right_value = it_right->second->get< attribute_value_type >();

        if (left_value)
        {
            if (right_value)
                return FunT::operator() (left_value.get(), right_value.get());
            else
                return false;
        }
        else
            return !right_value;
    }
};

/*!
 * The function constructs a log record ordering predicate
 */
template< typename ValueT, typename CharT, typename FunT >
inline attribute_value_ordering< CharT, ValueT, FunT >
make_attr_ordering(const CharT* name, FunT const& fun)
{
    typedef attribute_value_ordering< CharT, ValueT, FunT > ordering_t;
    return ordering_t(name, fun);
}

/*!
 * The function constructs a log record ordering predicate
 */
template< typename ValueT, typename CharT, typename FunT >
inline attribute_value_ordering< CharT, ValueT, FunT >
make_attr_ordering(std::basic_string< CharT > const& name, FunT const& fun)
{
    typedef attribute_value_ordering< CharT, ValueT, FunT > ordering_t;
    return ordering_t(name, fun);
}

#if !defined(BOOST_LOG_NO_FUNCTION_TRAITS)

namespace aux {

    //! An ordering predicate constructor that uses SFINAE to disable invalid instantiations
    template<
        typename CharT,
        typename FunT,
        typename ArityCheckT = typename enable_if_c< aux::arity_of< FunT >::value == 2 >::type,
        typename Arg1T = typename aux::first_argument_type_of< FunT >::type,
        typename Arg2T = typename aux::second_argument_type_of< FunT >::type,
        typename ArgsCheckT = typename enable_if< is_same< Arg1T, Arg2T > >::type
    >
    struct make_attr_ordering_type
    {
        typedef attribute_value_ordering< CharT, Arg1T, FunT > type;
    };

} // namespace aux

/*!
 * The function constructs a log record ordering predicate
 */
template< typename CharT, typename FunT >
inline typename aux::make_attr_ordering_type< CharT, FunT >::type
make_attr_ordering(const CharT* name, FunT const& fun)
{
    typedef typename aux::make_attr_ordering_type< CharT, FunT >::type ordering_t;
    return ordering_t(name, fun);
}

/*!
 * The function constructs a log record ordering predicate
 */
template< typename CharT, typename FunT >
inline typename aux::make_attr_ordering_type< CharT, FunT >::type
make_attr_ordering(std::basic_string< CharT > const& name, FunT const& fun)
{
    typedef typename aux::make_attr_ordering_type< CharT, FunT >::type ordering_t;
    return ordering_t(name, fun);
}

#endif // BOOST_LOG_NO_FUNCTION_TRAITS

} // namespace log

} // namespace boost

#endif // BOOST_LOG_UTILITY_RECORD_ORDERING_HPP_INCLUDED_
