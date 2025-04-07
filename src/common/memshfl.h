/*
 * Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
 *
 * This file is part of CasparCG (www.casparcg.com).
 *
 * CasparCG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CasparCG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Robert Nagy, ronag89@gmail.com
 */

#pragma once

#ifdef USE_SIMDE
#define SIMDE_ENABLE_NATIVE_ALIASES
#include <simde/x86/ssse3.h>
#else
#ifdef _MSC_VER
#include <intrin.h>
#include <tbb/scalable_allocator.h>
#else
#include <tmmintrin.h>
#endif
#endif

namespace caspar {

#ifdef _MSC_VER
static std::shared_ptr<void> create_aligned_buffer(size_t size, size_t alignment = 64)
{
    return std::shared_ptr<void>(scalable_aligned_malloc(size, alignment), scalable_aligned_free);
}
#else
static std::shared_ptr<void> create_aligned_buffer(size_t size, size_t alignment = 64)
{
    return std::shared_ptr<void>(aligned_alloc(alignment, size), free);
}
#endif

} // namespace caspar
