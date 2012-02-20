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

#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#if defined(_MSC_VER)
#	ifndef _SCL_SECURE_NO_WARNINGS
#		define _SCL_SECURE_NO_WARNINGS
#	endif
#	ifndef _CRT_SECURE_NO_WARNINGS
#		define _CRT_SECURE_NO_WARNINGS
#	endif
#endif

#ifdef _DEBUG
#include <crtdbg.h>
#endif

#include <cstdint>
#include <winsock2.h>
#include <tchar.h>
#include <sstream>
#include <memory>
#include <functional>
#include <algorithm>
#include <vector>
#include <deque>
#include <queue>
#include <string>
#include <math.h>

#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/range/algorithm.hpp>

#include "../common/utf.h"
#include "../common/memory.h"
//#include "../common/executor.h" // Can't include this due to MSVC lambda bug

#include "../common/log.h"
#include "../common/except.h"

#include <assert.h>