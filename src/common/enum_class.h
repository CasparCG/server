#pragma once

#include <type_traits>
#include <vector>

#define ENUM_ENABLE_BITWISE(enum_class)                                                                                \
    static enum_class operator&(enum_class lhs, enum_class rhs)                                                        \
    {                                                                                                                  \
        return static_cast<enum_class>(static_cast<std::underlying_type<enum_class>::type>(lhs) &                      \
                                       static_cast<std::underlying_type<enum_class>::type>(rhs));                      \
    };                                                                                                                 \
    static enum_class& operator&=(enum_class& lhs, enum_class rhs)                                                     \
    {                                                                                                                  \
        lhs = lhs & rhs;                                                                                               \
        return lhs;                                                                                                    \
    };                                                                                                                 \
    static enum_class operator|(enum_class lhs, enum_class rhs)                                                        \
    {                                                                                                                  \
        return static_cast<enum_class>(static_cast<std::underlying_type<enum_class>::type>(lhs) |                      \
                                       static_cast<std::underlying_type<enum_class>::type>(rhs));                      \
    };                                                                                                                 \
    static enum_class& operator|=(enum_class& lhs, enum_class rhs)                                                     \
    {                                                                                                                  \
        lhs = lhs | rhs;                                                                                               \
        return lhs;                                                                                                    \
    };                                                                                                                 \
    static enum_class operator^(enum_class lhs, enum_class rhs)                                                        \
    {                                                                                                                  \
        return static_cast<enum_class>(static_cast<std::underlying_type<enum_class>::type>(lhs) ^                      \
                                       static_cast<std::underlying_type<enum_class>::type>(rhs));                      \
    };                                                                                                                 \
    static enum_class& operator^=(enum_class& lhs, enum_class rhs)                                                     \
    {                                                                                                                  \
        lhs = lhs ^ rhs;                                                                                               \
        return lhs;                                                                                                    \
    };                                                                                                                 \
    static enum_class operator~(enum_class e)                                                                          \
    {                                                                                                                  \
        return static_cast<enum_class>(~static_cast<std::underlying_type<enum_class>::type>(e));                       \
    };

namespace caspar {

template <typename E>
const std::vector<E>& enum_constants()
{
    using integer = typename std::underlying_type<E>::type;

    static auto res = [] {
        std::vector<E> result;
        for (auto n = 0; n < static_cast<int>(E::count); ++n) {
            result.push_back(static_cast<E>(n));
        }
        return result;
    }();
    return res;
}

} // namespace caspar
