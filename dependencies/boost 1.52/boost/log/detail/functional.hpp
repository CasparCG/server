/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   functional.hpp
 * \author Andrey Semashev
 * \date   30.03.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_DETAIL_FUNCTIONAL_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_FUNCTIONAL_HPP_INCLUDED_

#include <algorithm>
#include <functional>
#include <boost/mpl/if.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/and.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_unsigned.hpp>
#include <boost/log/detail/prologue.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

//! Comparison implementation for integral types
template< template< typename > class PredT, typename T, typename U >
inline bool compare_integral(T const& left, U const& right)
{
    // We do these tricks mostly to silence compiler warnings like 'signed/unsigned mismatch'
    typedef typename mpl::eval_if_c<
        (sizeof(T) > sizeof(U)),
        mpl::identity< T >,
        mpl::eval_if_c<
            (sizeof(U) > sizeof(T)),
            mpl::identity< U >,
            mpl::if_<
                is_unsigned< T >,
                T,
                U
            >
        >
    >::type actual_type;
    return PredT< actual_type >()(left, right);
}

//! Equality predicate
struct equal_to
{
    typedef bool result_type;

    template< typename T, typename U >
    bool operator() (T const& left, U const& right) const
    {
        return op(left, right, typename mpl::and_< is_integral< T >, is_integral< U > >::type());
    }

private:
    template< typename T, typename U >
    static bool op(T const& left, U const& right, mpl::false_ const&)
    {
        return (left == right);
    }
    template< typename T, typename U >
    static bool op(T const& left, U const& right, mpl::true_ const&)
    {
        return compare_integral< std::equal_to >(left, right);
    }
};

//! Inequality predicate
struct not_equal_to
{
    typedef bool result_type;

    template< typename T, typename U >
    bool operator() (T const& left, U const& right) const
    {
        return op(left, right, typename mpl::and_< is_integral< T >, is_integral< U > >::type());
    }

private:
    template< typename T, typename U >
    static bool op(T const& left, U const& right, mpl::false_ const&)
    {
        return (left != right);
    }
    template< typename T, typename U >
    static bool op(T const& left, U const& right, mpl::true_ const&)
    {
        return compare_integral< std::not_equal_to >(left, right);
    }
};

//! Less predicate
struct less
{
    typedef bool result_type;

    template< typename T, typename U >
    bool operator() (T const& left, U const& right) const
    {
        return op(left, right, typename mpl::and_< is_integral< T >, is_integral< U > >::type());
    }

private:
    template< typename T, typename U >
    static bool op(T const& left, U const& right, mpl::false_ const&)
    {
        return (left < right);
    }
    template< typename T, typename U >
    static bool op(T const& left, U const& right, mpl::true_ const&)
    {
        return compare_integral< std::less >(left, right);
    }
};

//! Greater predicate
struct greater
{
    typedef bool result_type;

    template< typename T, typename U >
    bool operator() (T const& left, U const& right) const
    {
        return op(left, right, typename mpl::and_< is_integral< T >, is_integral< U > >::type());
    }

private:
    template< typename T, typename U >
    static bool op(T const& left, U const& right, mpl::false_ const&)
    {
        return (left > right);
    }
    template< typename T, typename U >
    static bool op(T const& left, U const& right, mpl::true_ const&)
    {
        return compare_integral< std::greater >(left, right);
    }
};

//! Less or equal predicate
struct less_equal
{
    typedef bool result_type;

    template< typename T, typename U >
    bool operator() (T const& left, U const& right) const
    {
        return op(left, right, typename mpl::and_< is_integral< T >, is_integral< U > >::type());
    }

private:
    template< typename T, typename U >
    static bool op(T const& left, U const& right, mpl::false_ const&)
    {
        return (left <= right);
    }
    template< typename T, typename U >
    static bool op(T const& left, U const& right, mpl::true_ const&)
    {
        return compare_integral< std::less_equal >(left, right);
    }
};

//! Greater or equal predicate
struct greater_equal
{
    typedef bool result_type;

    template< typename T, typename U >
    bool operator() (T const& left, U const& right) const
    {
        return op(left, right, typename mpl::and_< is_integral< T >, is_integral< U > >::type());
    }

private:
    template< typename T, typename U >
    static bool op(T const& left, U const& right, mpl::false_ const&)
    {
        return (left >= right);
    }
    template< typename T, typename U >
    static bool op(T const& left, U const& right, mpl::true_ const&)
    {
        return compare_integral< std::greater_equal >(left, right);
    }
};

//! The in_range functor
struct in_range_fun
{
    typedef bool result_type;

    template< typename T, typename U >
    bool operator() (T const& value, std::pair< U, U > const& rng) const
    {
        return op(value, rng, typename mpl::and_< is_integral< T >, is_integral< U > >::type());
    }

private:
    template< typename T, typename U >
    static bool op(T const& value, std::pair< U, U > const& rng, mpl::false_ const&)
    {
        return (value >= rng.first && value < rng.second);
    }
    template< typename T, typename U >
    static bool op(T const& value, std::pair< U, U > const& rng, mpl::true_ const&)
    {
        return (compare_integral< std::greater_equal >(value, rng.first)
            && compare_integral< std::less >(value, rng.second));
    }
};

//! The begins_with functor
struct begins_with_fun
{
    typedef bool result_type;

    template< typename T, typename U >
    bool operator() (T const& left, U const& right) const
    {
        if (left.size() >= right.size())
            return std::equal(right.begin(), right.end(), left.begin());
        else
            return false;
    }
};

//! The ends_with functor
struct ends_with_fun
{
    typedef bool result_type;

    template< typename T, typename U >
    bool operator() (T const& left, U const& right) const
    {
        if (left.size() >= right.size())
            return std::equal(right.begin(), right.end(), left.end() - right.size());
        else
            return false;
    }
};

//! The contains functor
struct contains_fun
{
    typedef bool result_type;

    template< typename T, typename U >
    bool operator() (T const& left, U const& right) const
    {
        if (left.size() >= right.size())
        {
            bool result = false;
            typename T::const_iterator search_end = left.end() - right.size() + 1;
            for (typename T::const_iterator it = left.begin(); it != search_end && !result; ++it)
                result = std::equal(right.begin(), right.end(), it);

            return result;
        }
        else
            return false;
    }
};

//! This tag type is used if an expression is not supported for matching against strings
struct unsupported_match_expression_tag;
//! This tag type is used if an expression is recognized as a Boost.Regex expression
struct boost_regex_expression_tag;
//! This tag type is used if an expression is recognized as a Boost.Xpressive expression
struct boost_xpressive_expression_tag;
//! This tag type is used if an expression is recognized as a Boost.Spirit (classic) expression
struct boost_spirit_classic_expression_tag;
//! This tag type is used if an expression is recognized as a Boost.Spirit.Qi expression
struct boost_spirit_qi_expression_tag;

//! Preliminary declaration of a trait that detects if an expression is a Boost.Regex expression
template< typename, bool = true >
struct is_regex :
    public mpl::false_
{
};
//! Preliminary declaration of a trait that detects if an expression is a Boost.Xpressive expression
template< typename, bool = true >
struct is_xpressive_regex :
    public mpl::false_
{
};
//! Preliminary declaration of a trait that detects if an expression is a Boost.Spirit (classic) expression
template< typename, bool = true >
struct is_spirit_classic_parser :
    public mpl::false_
{
};
//! Preliminary declaration of a trait that detects if an expression is a Boost.Spirit.Qi expression
template< typename, bool = true >
struct is_spirit_qi_parser :
    public mpl::false_
{
};

//! The regex matching functor implementation
template< typename TagT >
struct matches_fun_impl;

//! The regex matching functor
struct matches_fun
{
    typedef bool result_type;

private:
    //! A traits to obtain the tag of the expression
    template< typename ExpressionT >
    struct match_traits
    {
        typedef typename mpl::eval_if<
            is_regex< ExpressionT >,
            mpl::identity< boost_regex_expression_tag >,
            mpl::eval_if<
                is_xpressive_regex< ExpressionT >,
                mpl::identity< boost_xpressive_expression_tag >,
                mpl::eval_if<
                    is_spirit_classic_parser< ExpressionT >,
                    mpl::identity< boost_spirit_classic_expression_tag >,
                    mpl::if_<
                        is_spirit_qi_parser< ExpressionT >,
                        boost_spirit_qi_expression_tag,
                        unsupported_match_expression_tag
                    >
                >
            >
        >::type tag_type;
    };

public:
    template< typename StringT, typename ExpressionT >
    bool operator() (StringT const& str, ExpressionT const& expr) const
    {
        typedef typename match_traits< ExpressionT >::tag_type tag_type;
        typedef matches_fun_impl< tag_type > impl;
        return impl::matches(str, expr);
    }
    template< typename StringT, typename ExpressionT, typename ArgT >
    bool operator() (StringT const& str, ExpressionT const& expr, ArgT const& arg) const
    {
        typedef typename match_traits< ExpressionT >::tag_type tag_type;
        typedef matches_fun_impl< tag_type > impl;
        return impl::matches(str, expr, arg);
    }
};

//! The function object that does nothing
struct nop
{
    typedef void result_type;

    void operator() () const {}
    template< typename T >
    void operator() (T const&) const {}
};

//! Second argument binder
template< typename FunT, typename SecondArgT >
struct binder2nd :
    private FunT
{
    typedef typename FunT::result_type result_type;

    binder2nd(FunT const& fun, SecondArgT const& arg) : FunT(fun), m_Arg(arg) {}

    template< typename T >
    result_type operator() (T const& arg) const
    {
        return FunT::operator() (arg, m_Arg);
    }

private:
    SecondArgT m_Arg;
};

template< typename FunT, typename SecondArgT >
inline binder2nd< FunT, SecondArgT > bind2nd(FunT const& fun, SecondArgT const& arg)
{
    return binder2nd< FunT, SecondArgT >(fun, arg);
}

//! Third argument binder
template< typename FunT, typename ThirdArgT >
struct binder3rd :
    private FunT
{
    typedef typename FunT::result_type result_type;

    binder3rd(FunT const& fun, ThirdArgT const& arg) : FunT(fun), m_Arg(arg) {}

    template< typename T1, typename T2 >
    result_type operator() (T1 const& arg1, T2 const& arg2) const
    {
        return FunT::operator() (arg1, arg2, m_Arg);
    }

private:
    ThirdArgT m_Arg;
};

template< typename FunT, typename ThirdArgT >
inline binder3rd< FunT, ThirdArgT > bind3rd(FunT const& fun, ThirdArgT const& arg)
{
    return binder3rd< FunT, ThirdArgT >(fun, arg);
}

} // namespace aux

} // namespace log

} // namespace boost

#endif // BOOST_LOG_DETAIL_FUNCTIONAL_HPP_INCLUDED_
