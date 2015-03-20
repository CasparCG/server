#pragma once

#include <type_traits>

#include <boost/range/irange.hpp>

#include "linq.h"

// Macro that defines & and &= for an enum class. Add more when needed.

#define ENUM_ENABLE_BITWISE(enum_class) \
	static enum_class operator&(enum_class lhs, enum_class rhs) \
	{ \
		return static_cast<enum_class>( \
				static_cast<std::underlying_type<enum_class>::type>(lhs) \
					& static_cast<std::underlying_type<enum_class>::type>(rhs)); \
	}; \
	static enum_class& operator&=(enum_class& lhs, enum_class rhs) \
	{ \
		lhs = lhs & rhs; \
		return lhs; \
	};

namespace caspar {

// For enum classes starting at 0 and without any gaps with a terminating count constant.
template <typename E>
const std::vector<E>& enum_constants()
{
	typedef typename std::underlying_type<E>::type integer;

	static const auto ints = boost::irange(static_cast<integer>(0), static_cast<integer>(E::count));
	static const auto result = cpplinq::from(ints.begin(), ints.end())
		//.cast<E>()
		.select([](int i) { return static_cast<E>(i); })
		.to_vector();

	return result;
}

}
