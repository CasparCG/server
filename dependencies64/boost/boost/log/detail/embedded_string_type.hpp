/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   embedded_string_type.hpp
 * \author Andrey Semashev
 * \date   16.08.2009
 *
 * This header is the Boost.Log library implementation, see the library documentation
 * at http://www.boost.org/libs/log/doc/log.html.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_DETAIL_EMBEDDED_STRING_TYPE_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_EMBEDDED_STRING_TYPE_HPP_INCLUDED_

#include <string>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/type_traits/is_array.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_extent.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/log/detail/prologue.hpp>

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

    template< typename > struct is_char : mpl::false_ {};
    template< > struct is_char< char > : mpl::true_ {};
    template< > struct is_char< wchar_t > : mpl::true_ {};

    //! An auxiliary type translator to store strings by value in function objects
    template< typename ArgT >
    struct make_embedded_string_type
    {
        // Make sure that string literals and C strings are converted to STL strings
        typedef typename remove_cv<
            typename mpl::eval_if<
                is_array< ArgT >,
                remove_extent< ArgT >,
                mpl::eval_if<
                    is_pointer< ArgT >,
                    remove_pointer< ArgT >,
                    mpl::identity< void >
                >
            >::type
        >::type root_type;

        typedef typename mpl::eval_if<
            is_char< root_type >,
            mpl::identity< std::basic_string< root_type > >,
            remove_cv< ArgT >
        >::type type;
    };

} // namespace aux

} // namespace log

} // namespace boost

#endif // BOOST_LOG_DETAIL_EMBEDDED_STRING_TYPE_HPP_INCLUDED_
