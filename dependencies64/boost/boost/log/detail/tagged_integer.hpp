/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   tagged_integer.hpp
 * \author Andrey Semashev
 * \date   11.01.2008
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_TAGGED_INTEGER_HPP_INCLUDED_
#define BOOST_LOG_TAGGED_INTEGER_HPP_INCLUDED_

#include <boost/log/detail/prologue.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

    //! A tagged integer wrapper for type safety
    template< typename IntT, typename TagT >
    struct tagged_integer
    {
        //! Contained value type
        typedef IntT integer_type;
        //! Tag
        typedef TagT tag;

        //! Contained value
        integer_type value;

        //! Conversion operator
        operator integer_type() const { return value; }

        //  Increment
        tagged_integer& operator++ () { ++value; return *this; }
        tagged_integer operator++ (int) { tagged_integer temp = *this; ++value; return temp; }
        //  Decrement
        tagged_integer& operator-- () { --value; return *this; }
        tagged_integer operator-- (int) { tagged_integer temp = *this; --value; return temp; }

#define BOOST_LOG_TAGGED_INTEGER_OP(op)\
        tagged_integer& operator op (tagged_integer const& that) { value op that.value; return *this; }

        BOOST_LOG_TAGGED_INTEGER_OP(|=)
        BOOST_LOG_TAGGED_INTEGER_OP(&=)
        BOOST_LOG_TAGGED_INTEGER_OP(^=)
        BOOST_LOG_TAGGED_INTEGER_OP(+=)
        BOOST_LOG_TAGGED_INTEGER_OP(-=)
        BOOST_LOG_TAGGED_INTEGER_OP(*=)
        BOOST_LOG_TAGGED_INTEGER_OP(/=)
        BOOST_LOG_TAGGED_INTEGER_OP(%=)

#undef BOOST_LOG_TAGGED_INTEGER_OP

        //! Inversion operator
        tagged_integer& operator~ () { ~value; return *this; }

        //  Shift operators
        template< typename T >
        tagged_integer& operator<<= (T const& that) { value <<= that; return *this; }
        template< typename T >
        tagged_integer& operator>>= (T const& that) { value >>= that; return *this; }

    private:
        //  Protection against improper usage
        template< typename T1, typename T2 >
        tagged_integer& operator<<= (tagged_integer< T1, T2 > const&);
        template< typename T1, typename T2 >
        tagged_integer& operator>>= (tagged_integer< T1, T2 > const&);
    };

    //  Relational operators
#define BOOST_LOG_TAGGED_INTEGER_OP(op)\
    template< typename IntT, typename TagT >\
    inline bool operator op (\
        tagged_integer< IntT, TagT > const& left, tagged_integer< IntT, TagT > const& right)\
    {\
        return (left.value op right.value);\
    }

    BOOST_LOG_TAGGED_INTEGER_OP(==)
    BOOST_LOG_TAGGED_INTEGER_OP(!=)
    BOOST_LOG_TAGGED_INTEGER_OP(<)
    BOOST_LOG_TAGGED_INTEGER_OP(>)
    BOOST_LOG_TAGGED_INTEGER_OP(<=)
    BOOST_LOG_TAGGED_INTEGER_OP(>=)

#undef BOOST_LOG_TAGGED_INTEGER_OP

#define BOOST_LOG_TAGGED_INTEGER_OP(op)\
    template< typename IntT, typename TagT >\
    inline tagged_integer< IntT, TagT > operator op (\
        tagged_integer< IntT, TagT > const& left, tagged_integer< IntT, TagT > const& right)\
    {\
        tagged_integer< IntT, TagT > temp = left;\
        temp op##= right;\
        return temp;\
    }

    BOOST_LOG_TAGGED_INTEGER_OP(|)
    BOOST_LOG_TAGGED_INTEGER_OP(&)
    BOOST_LOG_TAGGED_INTEGER_OP(^)
    BOOST_LOG_TAGGED_INTEGER_OP(+)
    BOOST_LOG_TAGGED_INTEGER_OP(-)
    BOOST_LOG_TAGGED_INTEGER_OP(*)
    BOOST_LOG_TAGGED_INTEGER_OP(/)
    BOOST_LOG_TAGGED_INTEGER_OP(%)

#undef BOOST_LOG_TAGGED_INTEGER_OP

#define BOOST_LOG_TAGGED_INTEGER_OP(op)\
    template< typename IntT, typename TagT, typename T >\
    inline tagged_integer< IntT, TagT > operator op (\
        tagged_integer< IntT, TagT > const& left, T const& right)\
    {\
        tagged_integer< IntT, TagT > temp = left;\
        temp op##= right;\
        return temp;\
    }

    BOOST_LOG_TAGGED_INTEGER_OP(<<)
    BOOST_LOG_TAGGED_INTEGER_OP(>>)

#undef BOOST_LOG_TAGGED_INTEGER_OP

} // namespace aux

} // namespace log

} // namespace boost

#endif // BOOST_LOG_TAGGED_INTEGER_HPP_INCLUDED_
