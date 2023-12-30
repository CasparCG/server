#pragma once

#include <memory>

namespace caspar { namespace common {

enum class bit_depth : uint8_t
{
    bit8  = 0,
    bit16 = 1,
};

inline int bytes_per_pixel(bit_depth depth) { return static_cast<int>(depth) + 1; }

}} // namespace caspar::common