///////////////////////////
//
// SFML - Simple and Fast Multimedia Library
// Copyright (C) 2007-2009 Laurent Gomila (laurent.gom@gmail.com)
//
// This software is provided 'as-is', without any express or implied warranty.
// In no event will the authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it freely,
// subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented;
//    you must not claim that you wrote the original software.
//    If you use this software in a product, an acknowledgment
//    in the product documentation would be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such,
//    and must not be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
///////////////////////////

#pragma once

#include "../except.h"

namespace caspar { namespace gl {

struct ogl_exception : virtual caspar_exception
{
};
struct ogl_invalid_enum : virtual ogl_exception
{
};
struct ogl_invalid_value : virtual ogl_exception
{
};
struct ogl_invalid_operation : virtual ogl_exception
{
};
struct ogl_stack_overflow : virtual ogl_exception
{
};
struct ogl_stack_underflow : virtual ogl_exception
{
};
struct ogl_out_of_memory : virtual ogl_exception
{
};
struct ogl_invalid_framebuffer_operation_ext : virtual ogl_exception
{
};

void SMFL_GLCheckError(const std::string& expr, const char* func, const char* file, unsigned int line);

//#ifdef _DEBUG

#define CASPAR_GL_EXPR_STR(expr) #expr

#define GL(expr)                                                                                                       \
    if (false) {                                                                                                       \
    } else {                                                                                                           \
        (expr);                                                                                                        \
        caspar::gl::SMFL_GLCheckError(CASPAR_GL_EXPR_STR(expr), __FUNCTION__, __FILE__, __LINE__);                     \
    }

// TODO: decltype version does not play well with gcc
#define GL2(expr)                                                                                                      \
    [&]() {                                                                                                            \
        auto ret = (expr);                                                                                             \
        caspar::gl::SMFL_GLCheckError(CASPAR_GL_EXPR_STR(expr), __FUNCTION__, __FILE__, __LINE__);                     \
        return ret;                                                                                                    \
    }()
/*#define GL2(expr) \
    [&]() -> decltype(expr) \
    { \
        auto ret = (expr); \
        caspar::gl::SMFL_GLCheckError(CASPAR_GL_EXPR_STR(expr), __FILE__, __LINE__); \
        return ret; \
    }()*/
//#define GL2(expr) [&]() -> decltype(expr) { auto ret = (expr); caspar::gl::SMFL_GLCheckError(CASPAR_GL_EXPR_STR(expr),
//__FILE__, __LINE__); return ret; }() #else #define GL(expr) expr #endif

}} // namespace caspar::gl
