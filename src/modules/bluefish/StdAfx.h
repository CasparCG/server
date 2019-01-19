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

#if defined _DEBUG && defined _MSC_VER
#include <crtdbg.h>
#endif

#define NOMINMAX

#include <Windows.h>

#include <algorithm>
#include <array>
#include <deque>
#include <functional>
#include <math.h>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include <common/memory.h>
#include <common/utf.h>
//#include "../common/executor.h" // Can't include this due to MSVC lambda bug

#include <common/except.h>
#include <common/log.h>

#include <assert.h>
#include <boost/property_tree/ptree.hpp>
