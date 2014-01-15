/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   basic_filters.hpp
 * \author Andrey Semashev
 * \date   22.04.2007
 *
 * The header contains implementation of basic facilities used in auto-generated filters, including
 * base class for filters.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_FILTERS_BASIC_FILTERS_HPP_INCLUDED_
#define BOOST_LOG_FILTERS_BASIC_FILTERS_HPP_INCLUDED_

#include <string>
#include <functional>
#include <boost/shared_ptr.hpp>
#include <boost/static_assert.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/type_traits/is_base_and_derived.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/function_traits.hpp>
#include <boost/log/attributes/attribute_values_view.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace filters {

//! A base class for all filters that participate in lambda expressions
struct filter_base {};

/*!
 * \brief A type trait to detect filters
 *
 * The \c is_filter class is a metafunction that returns \c true if it is instantiated with
 * a filter type and \c false otherwise.
 */
template< typename T >
struct is_filter : public is_base_and_derived< filter_base, T > {};

/*!
 * \brief A base class for filters
 *
 * The \c basic_filter class defines standard types that most filters use and
 * have to provide in order to be valid functors. This class also enables
 * support for the \c is_filter type trait, which allows the filter
 * to take part in lambda expressions.
 */
template< typename CharT, typename DerivedT >
struct basic_filter : public filter_base
{
    //! Character type
    typedef CharT char_type;
    //! String type
    typedef std::basic_string< char_type > string_type;
    //! Attribute values view type
    typedef basic_attribute_values_view< char_type > values_view_type;

    //  Various standard functor typedefs
    typedef bool result_type;
    typedef values_view_type argument_type;
    typedef argument_type arg1_type;
    enum _ { arity = 1 };
};

/*!
 * \brief Filter wrapper
 *
 * This simple wrapper is intended to provide necessary facade of a filter to a third-party
 * functional object that implements the filter. The wrapper enables the filter to take part
 * in lambda expressions of filters.
 */
template< typename CharT, typename T >
class flt_wrap :
    public basic_filter< CharT, flt_wrap< CharT, T > >
{
private:
    //! Base type
    typedef basic_filter< CharT, flt_wrap< CharT, T > > base_type;
    //! Attribute values container type
    typedef typename base_type::values_view_type values_view_type;

private:
    //! Underlying filter
    T m_Filter;

public:
    /*!
     * Constructor. Creates a wrapper with the aggregated functional object.
     */
    explicit flt_wrap(T const& that) : m_Filter(that) {}

    /*!
     * Passes the call to the aggregated functional object
     *
     * \param values A set of attribute values of a single log record
     * \return The result of the aggregated functional object
     */
    bool operator() (values_view_type const& values) const
    {
        return static_cast< bool >(m_Filter(values));
    }
};

/*!
 * The function wraps a functor as a filter
 */
template< typename CharT, typename FunT >
inline flt_wrap< CharT, FunT > wrap(FunT const& fun)
{
    return flt_wrap< CharT, FunT >(fun);
}

#ifndef BOOST_LOG_NO_FUNCTION_TRAITS

namespace aux {

    template< typename T >
    struct char_type_from_attr_view
    {
    };
    template< typename CharT >
    struct char_type_from_attr_view< basic_attribute_values_view< CharT > >
    {
        typedef CharT type;
    };

    //! The metafunction guesses character type from the function object arguments
    template<
        typename FunT,
        typename ArgT = typename boost::log::aux::first_argument_type_of< FunT >::type,
        typename CharT = typename char_type_from_attr_view< ArgT >::type
    >
    struct char_type_of
    {
        typedef CharT type;
    };

} // namespace aux

/*!
 * The function wraps a functor as a filter (for those functors which provide argument typedefs)
 */
template< typename FunT >
inline flt_wrap<
    typename aux::char_type_of< FunT >::type,
    FunT
> wrap(FunT const& fun)
{
    return flt_wrap<
        typename aux::char_type_of< FunT >::type,
        FunT
    >(fun);
}

#endif // BOOST_LOG_NO_FUNCTION_TRAITS


/*!
 * \brief Negation filter
 *
 * The filter is used when result of the inner sub-filter has to be negated.
 */
template< typename FltT >
class flt_negation :
    public basic_filter< typename FltT::char_type, flt_negation< FltT > >
{
private:
    //! Base type
    typedef basic_filter< typename FltT::char_type, flt_negation< FltT > > base_type;
    //! Attribute values container type
    typedef typename base_type::values_view_type values_view_type;

private:
    //! Underlying filter
    FltT m_Filter;

public:
    /*!
     * Constructs the filter and initializes the stored subfilter with \a that
     */
    explicit flt_negation(FltT const& that) : m_Filter(that) {}

    /*!
     * Passes the call to the aggregated functional object
     *
     * \param values A set of attribute values of a single log record
     * \return The opposite value to the result of the aggregated sub-filter.
     */
    bool operator() (values_view_type const& values) const
    {
        return (!m_Filter(values));
    }
};

template< typename CharT, typename FltT >
inline flt_negation< FltT > operator! (basic_filter< CharT, FltT > const& that)
{
    return flt_negation< FltT >(static_cast< FltT const& >(that));
}

/*!
 * \brief Conjunction filter
 *
 * The filter implements logical AND of results of two sub-filters.
 */
template< typename LeftT, typename RightT >
class flt_and :
    public basic_filter< typename LeftT::char_type, flt_and< LeftT, RightT > >
{
    BOOST_STATIC_ASSERT((is_same< typename LeftT::char_type, typename RightT::char_type >::value));

private:
    //! Base type
    typedef basic_filter< typename LeftT::char_type, flt_and< LeftT, RightT > > base_type;
    //! Attribute values container type
    typedef typename base_type::values_view_type values_view_type;

private:
    //! Left-side filter
    LeftT m_Left;
    //! Right-side filter
    RightT m_Right;

public:
    /*!
     * Constructs the filter and initializes the stored subfilters
     */
    flt_and(LeftT const& left, RightT const& right) : m_Left(left), m_Right(right) {}

    /*!
     * \param values A set of attribute values of a single log record
     * \return Conjunction of the results of the aggregated sub-filters.
     */
    bool operator() (values_view_type const& values) const
    {
        return (m_Left(values) && m_Right(values));
    }
};

template< typename CharT, typename LeftT, typename RightT >
inline flt_and< LeftT, RightT > operator&& (
    basic_filter< CharT, LeftT > const& left, basic_filter< CharT, RightT > const& right)
{
    return flt_and< LeftT, RightT >(
        static_cast< LeftT const& >(left), static_cast< RightT const& >(right));
}


/*!
 * \brief Disjunction filter
 *
 * The filter implements logical OR of results of two sub-filters.
 */
template< typename LeftT, typename RightT >
class flt_or :
    public basic_filter< typename LeftT::char_type, flt_or< LeftT, RightT > >
{
    BOOST_STATIC_ASSERT((is_same< typename LeftT::char_type, typename RightT::char_type >::value));

private:
    //! Base type
    typedef basic_filter< typename LeftT::char_type, flt_or< LeftT, RightT > > base_type;
    //! Attribute values container type
    typedef typename base_type::values_view_type values_view_type;

private:
    //! Left-side filter
    LeftT m_Left;
    //! Right-side filter
    RightT m_Right;

public:
    /*!
     * Constructs the filter and initializes the stored subfilters
     */
    flt_or(LeftT const& left, RightT const& right) : m_Left(left), m_Right(right) {}

    /*!
     * \param values A set of attribute values of a single log record
     * \return Disjunction of the results of the aggregated sub-filters.
     */
    bool operator() (values_view_type const& values) const
    {
        return (m_Left(values) || m_Right(values));
    }
};

template< typename CharT, typename LeftT, typename RightT >
inline flt_or< LeftT, RightT > operator|| (
    basic_filter< CharT, LeftT > const& left, basic_filter< CharT, RightT > const& right)
{
    return flt_or< LeftT, RightT >(
        static_cast< LeftT const& >(left), static_cast< RightT const& >(right));
}

} // namespace filters

} // namespace log

} // namespace boost

#endif // BOOST_LOG_FILTERS_BASIC_FILTERS_HPP_INCLUDED_
