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
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include "../common/compiler/vs/disable_silly_warnings.h"

#if defined _DEBUG && defined _MSC_VER
#include <crtdbg.h>
#endif

#include <algorithm>
#include <array>
#include <assert.h>
#include <deque>
#include <functional>
#include <math.h>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include <tbb/concurrent_queue.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/regex.hpp>

#include "../common/memory.h"
#include "../common/utf.h"

#include "../common/except.h"
#include "../common/log.h"
#endif
#include <rpc.h>
#include <rpcndr.h>
