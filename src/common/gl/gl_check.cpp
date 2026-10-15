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
#include "gl_check.h"

#include "../except.h"
#include "../log.h"

#include <GL/glew.h>

#include <chrono>
#include <cstddef>
#include <mutex>

namespace caspar { namespace gl {

// GL() wraps nearly every GL call, so one invalid context makes this log grow at the frame rate.
static constexpr auto        error_log_window = std::chrono::seconds(1);
static constexpr std::size_t error_log_burst  = 4;

static std::mutex                            g_error_log_mutex;
static std::chrono::steady_clock::time_point g_error_log_window_start;
static bool                                  g_error_log_window_open = false;
static std::size_t                           g_error_log_logged      = 0;
static std::size_t                           g_error_log_suppressed  = 0;

static bool claim_error_log_slot(std::size_t& suppressed_before)
{
    const auto now = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(g_error_log_mutex);

    if (!g_error_log_window_open || now - g_error_log_window_start >= error_log_window) {
        g_error_log_window_open  = true;
        g_error_log_window_start = now;
        g_error_log_logged       = 0;
        suppressed_before        = g_error_log_suppressed;
        g_error_log_suppressed   = 0;
    }

    if (g_error_log_logged < error_log_burst) {
        ++g_error_log_logged;
        return true;
    }

    ++g_error_log_suppressed;
    return false;
}

void SMFL_GLCheckError(const std::string& /*unused*/, const char* func, const char* file, unsigned int line)
{
    // Get the last error
    GLenum LastErrorCode = GL_NO_ERROR;

    for (GLenum ErrorCode = glGetError(); ErrorCode != GL_NO_ERROR; ErrorCode = glGetError()) {
        std::size_t suppressed = 0;
        if (claim_error_log_slot(suppressed)) {
            if (suppressed > 0) {
                CASPAR_LOG(error) << "OpenGL Error: " << suppressed << " further errors were not logged.";
            }
            std::string str(reinterpret_cast<const char*>(glewGetErrorString(ErrorCode)));
            CASPAR_LOG(error) << "OpenGL Error: " << ErrorCode << L" " << str;
        }
        LastErrorCode = ErrorCode;
    }

    if (LastErrorCode != GL_NO_ERROR) {
        // Decode the error code
        switch (LastErrorCode) {
            case GL_INVALID_ENUM:
                CASPAR_THROW_EXCEPTION(ogl_invalid_enum()
                                       << msg_info(
                                              "an unacceptable value has been specified for an enumerated argument")
                                       << error_info("GL_INVALID_ENUM"));

            case GL_INVALID_VALUE:
                CASPAR_THROW_EXCEPTION(ogl_invalid_value() << msg_info("a numeric argument is out of range")
                                                           << error_info("GL_INVALID_VALUE"));

            case GL_INVALID_OPERATION:
                CASPAR_THROW_EXCEPTION(ogl_invalid_operation()
                                       << msg_info("the specified operation is not allowed in the current state")
                                       << error_info("GL_INVALID_OPERATION"));

            case GL_STACK_OVERFLOW:
                CASPAR_THROW_EXCEPTION(ogl_stack_overflow() << msg_info("this command would cause a stack overflow")
                                                            << error_info("GL_STACK_OVERFLOW"));

            case GL_STACK_UNDERFLOW:
                CASPAR_THROW_EXCEPTION(ogl_stack_underflow() << msg_info("this command would cause a stack underflow")
                                                             << error_info("GL_STACK_UNDERFLOW"));

            case GL_OUT_OF_MEMORY:
                CASPAR_THROW_EXCEPTION(ogl_out_of_memory()
                                       << msg_info("there is not enough memory left to execute the command")
                                       << error_info("GL_OUT_OF_MEMORY"));

            case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
                CASPAR_THROW_EXCEPTION(
                    ogl_invalid_framebuffer_operation_ext()
                    << msg_info("the object bound to FRAMEBUFFER_BINDING_EXT is not \"framebuffer complete\"")
                    << error_info("GL_INVALID_FRAMEBUFFER_OPERATION_EXT"));
        }
    }
}

}} // namespace caspar::gl
