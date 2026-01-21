#pragma once

#include <memory>
#include <cstdint>

namespace caspar { namespace common {

enum class bit_depth : uint8_t
{
    bit8 = 0,
    bit10,
    bit12,
    // bit14,
    bit16,
};

}} // namespace caspar::common
