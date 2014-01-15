/*
 *          Copyright Andrey Semashev 2007 - 2010.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   sources/features.hpp
 * \author Andrey Semashev
 * \date   17.07.2009
 *
 * The header contains definition of a features list class template.
 */

#if (defined(_MSC_VER) && _MSC_VER > 1000)
#pragma once
#endif // _MSC_VER > 1000

#ifndef BOOST_LOG_SOURCES_FEATURES_HPP_INCLUDED_
#define BOOST_LOG_SOURCES_FEATURES_HPP_INCLUDED_

#include <boost/mpl/vector.hpp>
#include <boost/mpl/limits/vector.hpp>
#include <boost/mpl/aux_/na.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
#include <boost/log/detail/prologue.hpp>

//! The macro defines the maximum number of features that can be specified for a logger
#ifndef BOOST_LOG_FEATURES_LIMIT
#define BOOST_LOG_FEATURES_LIMIT BOOST_MPL_LIMIT_VECTOR_SIZE
#endif // BOOST_LOG_FEATURES_LIMIT

namespace boost {

namespace BOOST_LOG_NAMESPACE {

namespace sources {

#if !defined(BOOST_LOG_DOXYGEN_PASS)

//! An MPL sequence of logger features
template< BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(BOOST_LOG_FEATURES_LIMIT, typename FeatureT, mpl::na) >
struct features :
    public mpl::vector< BOOST_PP_ENUM_PARAMS(BOOST_LOG_FEATURES_LIMIT, FeatureT) >::type
{
};

#else // !defined(BOOST_LOG_DOXYGEN_PASS)

/*!
 * \brief An MPL sequence of logger features
 *
 * This class template can be used to specify logger features in a \c basic_composite_logger instantiation.
 * The resulting type is an MPL type sequence.
 */
template< typename... FeaturesT >
struct features;

#endif // !defined(BOOST_LOG_DOXYGEN_PASS)

} // namespace sources

} // namespace log

} // namespace boost

#endif // BOOST_LOG_SOURCES_FEATURES_HPP_INCLUDED_
