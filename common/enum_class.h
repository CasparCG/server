#pragma once

#include <type_traits>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/irange.hpp>

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

template<typename E>
struct enum_from_int
{
	typedef E result_type;

	E operator()(typename std::underlying_type<E>::type i) const
	{
		return static_cast<E>(i);
	}
};

// For enum classes starting at 0 and without any gaps with a terminating count constant.
template <typename E>
boost::transformed_range<enum_from_int<E>, const boost::integer_range<typename std::underlying_type<E>::type>> iterate_enum()
{
	typedef typename std::underlying_type<E>::type integer;
	return boost::irange(static_cast<integer>(0), static_cast<integer>(E::count)) | boost::adaptors::transformed(enum_from_int<E>());
}

}