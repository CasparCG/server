/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   filters/attr.hpp
 * \author Andrey Semashev
 * \date   22.04.2007
 *
 * The header contains implementation of a generic attribute placeholder in filters.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_FILTERS_ATTR_HPP_INCLUDED_
#define BOOST_LOG_FILTERS_ATTR_HPP_INCLUDED_

#include <new> // std::nothrow
#include <string>
#include <utility>
#include <boost/mpl/at.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/int.hpp>
#include <boost/mpl/if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/is_sequence.hpp>
#include <boost/utility/addressof.hpp>
#include <boost/log/detail/prologue.hpp>
#include <boost/log/detail/functional.hpp>
#include <boost/log/detail/embedded_string_type.hpp>
#include <boost/log/filters/basic_filters.hpp>
#include <boost/log/filters/exception_policies.hpp>
#include <boost/log/attributes/attribute_values_view.hpp>
#include <boost/log/utility/attribute_value_extractor.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace filters {

namespace aux {

    //! Function adapter that saves the checker function result into a referenced variable
    template< typename PredicateT >
    struct predicate_wrapper
    {
        typedef void result_type;

        predicate_wrapper(PredicateT const& fun, bool& res) : m_Fun(fun), m_Result(res) {}

        template< typename T >
        void operator() (T const& arg) const
        {
            m_Result = m_Fun(arg);
        }

    private:
        PredicateT const& m_Fun;
        bool& m_Result;
    };

} // namespace aux

/*!
 * \brief The filter checks that the attribute value satisfies the predicate \c FunT
 *
 * The \c flt_attr filter extracts stored attribute value and applies the predicate
 * to it. The result of the predicate is returned as a result of filtering. If the
 * attribute value is not found a \c missing_attribute_value exception is thrown.
 *
 * One should not resort to direct usage of this class. The filter is constructed
 * with the \c attr helper function and lambda expression. See "Advanced features ->
 * Filters -> Generic attribute placeholder" section of the library documentation.
 */
template<
    typename CharT,
    typename FunT,
    typename AttributeValueTypesT,
    typename ExceptionPolicyT
>
class flt_attr :
    public basic_filter< CharT, flt_attr< CharT, FunT, AttributeValueTypesT, ExceptionPolicyT > >
{
    //! Base type
    typedef basic_filter< CharT, flt_attr< CharT, FunT, AttributeValueTypesT, ExceptionPolicyT > > base_type;
    //! Attribute value extractor type
    typedef attribute_value_extractor< CharT, AttributeValueTypesT > extractor;

public:
    //! String type
    typedef typename base_type::string_type string_type;
    //! Attribute values container type
    typedef typename base_type::values_view_type values_view_type;
    //! Predicate functor type
    typedef FunT checker_type;

private:
    //! Attribute value extractor
    extractor m_Extractor;
    //! Attribute value checker
    checker_type m_Checker;

public:
    /*!
     * Constructs the filter
     *
     * \param name The attribute name
     * \param checker A predicate that is applied to the attribute value
     */
    flt_attr(string_type const& name, checker_type const& checker)
        : m_Extractor(name), m_Checker(checker)
    {
    }

    /*!
     * Applies the filter
     *
     * \param values A set of attribute values of a single log record
     * \return true if the log record passed the filter, false otherwise
     */
    bool operator() (values_view_type const& values) const
    {
        bool result = false;
        aux::predicate_wrapper< checker_type > receiver(m_Checker, result);
        if (!m_Extractor(values, receiver))
            ExceptionPolicyT::on_attribute_value_not_found(__FILE__, __LINE__);

        return result;
    }
};

#ifndef BOOST_LOG_DOXYGEN_PASS

namespace aux {

    //! The base class for attr filter generator
    template< typename CharT, typename AttributeValueTypesT, typename ExceptionPolicyT, bool >
    class flt_attr_gen_base;

    //! Specialization for non-string attribute value types
    template< typename CharT, typename AttributeValueTypesT, typename ExceptionPolicyT >
    class flt_attr_gen_base< CharT, AttributeValueTypesT, ExceptionPolicyT, false >
    {
    public:
        //! Char type
        typedef CharT char_type;
        //! String type
        typedef std::basic_string< char_type > string_type;
        //! Attribute values container type
        typedef basic_attribute_values_view< char_type > values_view_type;
        //! Size type
        typedef typename values_view_type::size_type size_type;
        //! Supported attribute value types
        typedef AttributeValueTypesT attribute_value_types;

    protected:
        //! Attribute name
        string_type m_AttributeName;

    public:
        explicit flt_attr_gen_base(string_type const& name) : m_AttributeName(name) {}

#define BOOST_LOG_FILTER_ATTR_MEMBER(member, fun)\
        template< typename T >\
        flt_attr<\
            char_type,\
            boost::log::aux::binder2nd< fun, typename boost::log::aux::make_embedded_string_type< T >::type >,\
            attribute_value_types,\
            ExceptionPolicyT\
        > member (T const& arg) const\
        {\
            typedef typename boost::log::aux::make_embedded_string_type< T >::type arg_type;\
            typedef boost::log::aux::binder2nd< fun, arg_type > binder_t;\
            typedef flt_attr< char_type, binder_t, attribute_value_types, ExceptionPolicyT > flt_attr_t;\
            return flt_attr_t(this->m_AttributeName, binder_t(fun(), arg_type(arg)));\
        }

        BOOST_LOG_FILTER_ATTR_MEMBER(operator ==, boost::log::aux::equal_to)
        BOOST_LOG_FILTER_ATTR_MEMBER(operator !=, boost::log::aux::not_equal_to)
        BOOST_LOG_FILTER_ATTR_MEMBER(operator >, boost::log::aux::greater)
        BOOST_LOG_FILTER_ATTR_MEMBER(operator <, boost::log::aux::less)
        BOOST_LOG_FILTER_ATTR_MEMBER(operator >=, boost::log::aux::greater_equal)
        BOOST_LOG_FILTER_ATTR_MEMBER(operator <=, boost::log::aux::less_equal)

        //! Filter generator for checking whether the attribute value lies within a specific range
        template< typename T >
        flt_attr<
            char_type,
            boost::log::aux::binder2nd<
                boost::log::aux::in_range_fun,
                std::pair<
                    typename boost::log::aux::make_embedded_string_type< T >::type,
                    typename boost::log::aux::make_embedded_string_type< T >::type
                >
            >,
            attribute_value_types,
            ExceptionPolicyT
        > is_in_range(T const& lower, T const& upper) const
        {
            typedef typename boost::log::aux::make_embedded_string_type< T >::type arg_type;
            typedef boost::log::aux::binder2nd< boost::log::aux::in_range_fun, std::pair< arg_type, arg_type > > binder_t;
            typedef flt_attr< char_type, binder_t, attribute_value_types, ExceptionPolicyT > flt_attr_t;
            return flt_attr_t(
                this->m_AttributeName,
                binder_t(boost::log::aux::in_range_fun(), std::make_pair(arg_type(lower), arg_type(upper))));
        }

        //! Filter generator for user-provided predicate function
        template< typename UnaryFunT >
        flt_attr<
            char_type,
            UnaryFunT,
            attribute_value_types,
            ExceptionPolicyT
        > satisfies(UnaryFunT const& fun) const
        {
            typedef flt_attr<
                char_type,
                UnaryFunT,
                attribute_value_types,
                ExceptionPolicyT
            > flt_attr_t;
            return flt_attr_t(this->m_AttributeName, fun);
        }
    };

    //! Specialization for string attribute value types
    template< typename CharT, typename AttributeValueTypesT, typename ExceptionPolicyT >
    class flt_attr_gen_base< CharT, AttributeValueTypesT, ExceptionPolicyT, true > :
        public flt_attr_gen_base< CharT, AttributeValueTypesT, ExceptionPolicyT, false >
    {
        typedef flt_attr_gen_base< CharT, AttributeValueTypesT, ExceptionPolicyT, false > base_type;

    public:
        //! Char type
        typedef typename base_type::char_type char_type;
        //! String type
        typedef typename base_type::string_type string_type;
        //! Supported attribute value types (actually, a single string type in this specialization)
        typedef typename base_type::attribute_value_types attribute_value_types;

    private:
        //! The attribute value character type
        typedef typename attribute_value_types::value_type attribute_value_char_type;

    public:
        explicit flt_attr_gen_base(string_type const& name) : base_type(name) {}

        BOOST_LOG_FILTER_ATTR_MEMBER(begins_with, boost::log::aux::begins_with_fun)
        BOOST_LOG_FILTER_ATTR_MEMBER(ends_with, boost::log::aux::ends_with_fun)
        BOOST_LOG_FILTER_ATTR_MEMBER(contains, boost::log::aux::contains_fun)

#undef BOOST_LOG_FILTER_ATTR_MEMBER

        //! Filter generator for checking whether the attribute value matches a regex
        template< typename ExpressionT >
        flt_attr<
            char_type,
            boost::log::aux::binder2nd< boost::log::aux::matches_fun, ExpressionT >,
            attribute_value_types,
            ExceptionPolicyT
        > matches(ExpressionT const& expr) const
        {
            typedef boost::log::aux::binder2nd<
                boost::log::aux::matches_fun,
                ExpressionT
            > binder_t;
            typedef flt_attr< char_type, binder_t, attribute_value_types, ExceptionPolicyT > flt_attr_t;
            return flt_attr_t(this->m_AttributeName, binder_t(boost::log::aux::matches_fun(), expr));
        }

        //! Filter generator for checking whether the attribute value matches a regex
        template< typename ExpressionT, typename ArgT >
        flt_attr<
            char_type,
            boost::log::aux::binder2nd< boost::log::aux::binder3rd< boost::log::aux::matches_fun, ArgT >, ExpressionT >,
            attribute_value_types,
            ExceptionPolicyT
        > matches(ExpressionT const& expr, ArgT const& arg) const
        {
            typedef boost::log::aux::binder3rd< boost::log::aux::matches_fun, ArgT > binder3rd_t;
            typedef boost::log::aux::binder2nd< binder3rd_t, ExpressionT > binder2nd_t;
            typedef flt_attr< char_type, binder2nd_t, attribute_value_types, ExceptionPolicyT > flt_attr_t;
            return flt_attr_t(this->m_AttributeName, binder2nd_t(binder3rd_t(boost::log::aux::matches_fun(), arg), expr));
        }
    };

    //! The MPL predicate detects if the type is STL string
    template< typename T >
    struct is_string : mpl::false_ {};
    template< typename CharT, typename TraitsT, typename AllocatorT >
    struct is_string< std::basic_string< CharT, TraitsT, AllocatorT > > : mpl::true_ {};

    //! The metafunction replaces 1-sized MPL sequences with a single equivalent type
    template< typename TypesT, bool = mpl::is_sequence< TypesT >::value >
    struct simplify_type_sequence;
    template< typename TypesT >
    struct simplify_type_sequence< TypesT, false >
    {
        typedef TypesT type;
        enum { is_only_string = is_string< TypesT >::value };
    };
    template< typename TypesT >
    struct simplify_type_sequence< TypesT, true >
    {
    private:
        typedef typename mpl::eval_if<
            mpl::equal_to< mpl::size< TypesT >, mpl::int_< 1 > >,
            mpl::at_c< TypesT, 0 >,
            mpl::identity< TypesT >
        >::type actual_types;

    public:
        typedef actual_types type;
        enum { is_only_string = is_string< actual_types >::value };
    };

    //! Attribute filter generator
    template< typename CharT, typename AttributeValueTypesT, typename ExceptionPolicyT >
    class flt_attr_gen :
        public flt_attr_gen_base<
            CharT,
            typename simplify_type_sequence< AttributeValueTypesT >::type,
            ExceptionPolicyT,
            simplify_type_sequence< AttributeValueTypesT >::is_only_string
        >
    {
        typedef flt_attr_gen_base<
            CharT,
            typename simplify_type_sequence< AttributeValueTypesT >::type,
            ExceptionPolicyT,
            simplify_type_sequence< AttributeValueTypesT >::is_only_string
        > base_type;

    public:
        //! String type
        typedef typename base_type::string_type string_type;

    public:
        explicit flt_attr_gen(string_type const& name) : base_type(name) {}
    };

} // namespace aux

#endif // BOOST_LOG_DOXYGEN_PASS

/*!
 * The function generates an attribute placeholder in filter expressions
 *
 * \param name Attribute name. Must point to a zero-terminated string, must not be NULL.
 * \return An object that will, upon applying a corresponding operation to it, construct the filter.
 */
template< typename AttributeValueTypesT, typename CharT >
inline
#ifndef BOOST_LOG_DOXYGEN_PASS
aux::flt_attr_gen< CharT, AttributeValueTypesT, throw_policy >
#else
implementation_defined
#endif
attr(const CharT* name)
{
    return aux::flt_attr_gen< CharT, AttributeValueTypesT, throw_policy >(name);
}

/*!
 * The function generates an attribute placeholder in filter expressions
 *
 * \param name Attribute name.
 * \return An object that will, upon applying a corresponding operation to it, construct the filter.
 */
template< typename AttributeValueTypesT, typename CharT >
inline
#ifndef BOOST_LOG_DOXYGEN_PASS
aux::flt_attr_gen< CharT, AttributeValueTypesT, throw_policy >
#else
implementation_defined
#endif
attr(std::basic_string< CharT > const& name)
{
    return aux::flt_attr_gen< CharT, AttributeValueTypesT, throw_policy >(name);
}

/*!
 * The function generates an attribute placeholder in filter expressions.
 * The filter will not throw if the attribute value is not found in the record being filtered.
 * Instead, a negative result will be returned.
 *
 * \param name Attribute name. Must point to a zero-terminated string, must not be NULL.
 * \return An object that will, upon applying a corresponding operation to it, construct the filter.
 */
template< typename AttributeValueTypesT, typename CharT >
inline
#ifndef BOOST_LOG_DOXYGEN_PASS
aux::flt_attr_gen< CharT, AttributeValueTypesT, no_throw_policy >
#else
implementation_defined
#endif
attr(const CharT* name, std::nothrow_t const&)
{
    return aux::flt_attr_gen< CharT, AttributeValueTypesT, no_throw_policy >(name);
}

/*!
 * The function generates an attribute placeholder in filter expressions.
 * The filter will not throw if the attribute value is not found in the record being filtered.
 * Instead, a negative result will be returned.
 *
 * \param name Attribute name.
 * \return An object that will, upon applying a corresponding operation to it, construct the filter.
 */
template< typename AttributeValueTypesT, typename CharT >
inline
#ifndef BOOST_LOG_DOXYGEN_PASS
aux::flt_attr_gen< CharT, AttributeValueTypesT, no_throw_policy >
#else
implementation_defined
#endif
attr(std::basic_string< CharT > const& name, std::nothrow_t const&)
{
    return aux::flt_attr_gen< CharT, AttributeValueTypesT, no_throw_policy >(name);
}

} // namespace filters

} // namespace log

} // namespace boost

#endif // BOOST_LOG_FILTERS_ATTR_HPP_INCLUDED_
