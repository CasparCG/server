#pragma once

#include <memory>

namespace caspar { namespace common {

enum class bit_depth : uint8_t
{
    bit8 = 0,
    bit10,
    bit12,
    // bit14,
    bit16,
};

inline int bytes_per_pixel(bit_depth depth)
{
    if (depth == bit_depth::bit8)
        return 1;
    return 2;
}

}} // namespace caspar::common