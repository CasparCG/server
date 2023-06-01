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

#ifdef __cplusplus
#if defined _DEBUG && defined _MSC_VER
#include <crtdbg.h>
#endif

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#if defined(_MSC_VER)
#include <Windows.h>
#endif

#include <algorithm>
#include <array>
#include <assert.h>
#include <atomic>
#include <boost/algorithm/string.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/range.hpp>
#include <boost/range/algorithm.hpp>
#include <deque>
#include <functional>
#include <math.h>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <tbb/cache_aligned_allocator.h>
#include <tbb/concurrent_queue.h>
#include <vector>

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
extern "C" {
#define __STDC_CONSTANT_MACROS
#define __STDC_LIMIT_MACROS
#include <libavcodec/avcodec.h>
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#pragma warning(push)
#pragma warning(disable : 4996)

#if defined(_MSC_VER)
#include <atlbase.h>

#include <atlcom.h>
#include <atlhost.h>
#endif

#pragma warning(pop)

#include <functional>

#include "../common/except.h"
#include "../common/log.h"
#include "../common/memory.h"
#include "../common/timer.h"
#include "../common/utf.h"
#endif

#if defined(_MSC_VER)
#include <rpc.h>
#include <rpcndr.h>
#endif
