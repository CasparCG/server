/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   parameter_tools.hpp
 * \author Andrey Semashev
 * \date   28.06.2009
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/libs/log/doc/log.html.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_DETAIL_PARAMETER_TOOLS_HPP_INCLUDED_
#define BOOST_LOG_DETAIL_PARAMETER_TOOLS_HPP_INCLUDED_

#include <boost/parameter/keyword.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/log/detail/prologue.hpp>

#ifndef BOOST_LOG_MAX_PARAMETER_ARGS
//! The maximum number of named arguments that are accepted by constructors and functions
#define BOOST_LOG_MAX_PARAMETER_ARGS 16
#endif

// The macro applies the passed macro with the specified arguments BOOST_LOG_MAX_PARAMETER_ARGS times
#define BOOST_LOG_PARAMETRIZED_CONSTRUCTORS_GEN(macro, args)\
    public:\
        BOOST_PP_REPEAT_FROM_TO(1, BOOST_LOG_MAX_PARAMETER_ARGS, macro, args)


#define BOOST_LOG_CTOR_FORWARD(z, n, types)\
    template< BOOST_PP_ENUM_PARAMS(n, typename T) >\
    explicit BOOST_PP_TUPLE_ELEM(2, 0, types)(BOOST_PP_ENUM_BINARY_PARAMS(n, T, const& arg)) :\
        BOOST_PP_TUPLE_ELEM(2, 1, types)((BOOST_PP_ENUM_PARAMS(n, arg))) {}

// The macro expands to a number of templated constructors that aggregate their named arguments
// into an ArgumentsPack and pass it to the base class constructor.
#define BOOST_LOG_PARAMETRIZED_CONSTRUCTORS_FORWARD(class_type, base_type)\
    BOOST_LOG_PARAMETRIZED_CONSTRUCTORS_GEN(BOOST_LOG_CTOR_FORWARD, (class_type, base_type))


#define BOOST_LOG_CTOR_CALL(z, n, types)\
    template< BOOST_PP_ENUM_PARAMS(n, typename T) >\
    explicit BOOST_PP_TUPLE_ELEM(2, 0, types)(BOOST_PP_ENUM_BINARY_PARAMS(n, T, const& arg))\
        { BOOST_PP_TUPLE_ELEM(2, 1, types)((BOOST_PP_ENUM_PARAMS(n, arg))); }

// The macro expands to a number of templated constructors that aggregate their named arguments
// into an ArgumentsPack and pass it to a function call.
#define BOOST_LOG_PARAMETRIZED_CONSTRUCTORS_CALL(class_type, fun)\
    BOOST_LOG_PARAMETRIZED_CONSTRUCTORS_GEN(BOOST_LOG_CTOR_CALL, (class_type, fun))

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace aux {

    // Yeah, not too cute. The empty_arg_list class should really be public.
    typedef boost::parameter::aux::empty_arg_list empty_arg_list;

} // namespace aux

} // namespace log

} // namespace boost

#endif // BOOST_LOG_DETAIL_PARAMETER_TOOLS_HPP_INCLUDED_
