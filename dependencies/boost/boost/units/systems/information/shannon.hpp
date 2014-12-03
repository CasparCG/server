// Boost.Units - A C++ library for zero-overhead dimensional analysis and 
// unit/quantity manipulation and conversion
//
// Copyright (C) 2014 Erik Erlandson
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNITS_INFORMATION_SHANNON_HPP
#define BOOST_UNITS_INFORMATION_SHANNON_HPP

#include <boost/units/systems/information/byte.hpp>
#include <boost/units/base_units/information/shannon.hpp>

namespace boost {
namespace units { 
namespace information {

namespace hu {
namespace shannon {
typedef unit<information_dimension, make_system<shannon_base_unit>::type> info;
}}

BOOST_UNITS_STATIC_CONSTANT(shannon, hu::shannon::info);
BOOST_UNITS_STATIC_CONSTANT(shannons, hu::shannon::info);

}}}

#endif
