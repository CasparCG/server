#pragma once

#include <type_traits>

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
