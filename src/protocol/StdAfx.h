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
 * Author: Nicklas P Andersson
 */

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#if defined(_MSC_VER)
#ifndef _SCL_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#if defined _DEBUG && defined _MSC_VER
#include <crtdbg.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/range/algorithm.hpp>

#include <common/memory.h>
#include <common/utf.h>

#include <common/except.h>
#include <common/log.h>

#include <cassert>
