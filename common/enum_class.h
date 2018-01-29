#pragma once

#include <type_traits>

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
	}; \
	static enum_class operator | (enum_class lhs, enum_class rhs) \
	{ \
		return static_cast<enum_class>( \
				static_cast<std::underlying_type<enum_class>::type>(lhs) \
					| static_cast<std::underlying_type<enum_class>::type>(rhs)); \
	}; \
	static enum_class& operator|=(enum_class& lhs, enum_class rhs) \
	{ \
		lhs = lhs | rhs; \
		return lhs; \
	}; \
	static enum_class operator ^ (enum_class lhs, enum_class rhs) \
	{ \
		return static_cast<enum_class>( \
				static_cast<std::underlying_type<enum_class>::type>(lhs) \
					^ static_cast<std::underlying_type<enum_class>::type>(rhs)); \
	}; \
	static enum_class& operator^=(enum_class& lhs, enum_class rhs) \
	{ \
		lhs = lhs ^ rhs; \
		return lhs; \
	}; \
	static enum_class operator~ (enum_class e) \
	{ \
		return static_cast<enum_class>( \
				~static_cast<std::underlying_type<enum_class>::type>(e)); \
	};

namespace caspar {

// For enum classes starting at 0 and without any gaps with a terminating count constant.
template <typename E>
const std::vector<E>& enum_constants()
{
	typedef typename std::underlying_type<E>::type integer;

    static auto res = []
    {
        std::vector<E> result;
        for (auto i : boost::irange(static_cast<integer>(0), static_cast<integer>(E::count))) {
            result.push_back(static_cast<E>(i));
        }
        return result;
    }();
	return res;
}

}
