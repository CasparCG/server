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

#include "except.h"
#include "log.h"

#ifdef _MSC_VER
#define _CASPAR_DBG_BREAK _CrtDbgBreak()
#else
#define _CASPAR_DBG_BREAK
#endif

#define CASPAR_VERIFY_EXPR_STR(str) #str

#define CASPAR_VERIFY(expr)                                                                                            \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            CASPAR_LOG(warning) << "Assertion Failed: " << CASPAR_VERIFY_EXPR_STR(expr) << " "                         \
                                << "file:" << __FILE__ << " "                                                          \
                                << "line:" << __LINE__ << " ";                                                         \
            _CASPAR_DBG_BREAK;                                                                                         \
        }                                                                                                              \
    } while (0);

#define CASPAR_ENSURE(expr)                                                                                            \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            CASPAR_THROW_EXCEPTION(programming_error()                                                                 \
                                   << msg_info(std::string("Assertion Failed: ") + CASPAR_VERIFY_EXPR_STR(expr)));     \
        }                                                                                                              \
    } while (0);

#ifdef _DEBUG
#define CASPAR_ASSERT(expr) CASPAR_VERIFY(expr)
#else
#define CASPAR_ASSERT(expr)
#endif
