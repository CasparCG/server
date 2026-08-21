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
 * Author: CasparCG Team
 */

#pragma once

#include <common/except.h>
#include <common/log.h>

#include <vulkan/vulkan.h>

#include <string>

namespace caspar { namespace screen { namespace vulkan {

struct vk_exception : virtual caspar_exception
{
};

// Special exception for device lost - allows targeted handling
struct device_lost_exception : virtual vk_exception
{
};

inline const char* vk_result_to_string(VkResult result)
{
    switch (result) {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN:
            return "VK_ERROR_UNKNOWN";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_FRAGMENTATION:
            return "VK_ERROR_FRAGMENTATION";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        default:
            return "UNKNOWN_VK_RESULT";
    }
}

inline void
vk_check_error(VkResult result, const std::string& expr, const char* func, const char* file, unsigned int line)
{
    if (result != VK_SUCCESS) {
        std::stringstream ss;
        ss << "Vulkan error: " << vk_result_to_string(result) << " (" << static_cast<int>(result) << ")"
           << "\n\tExpression: " << expr << "\n\tFunction: " << func << "\n\tFile: " << file << "\n\tLine: " << line;

        CASPAR_LOG(error) << ss.str();

        // Throw specific exception for device lost to enable targeted recovery
        if (result == VK_ERROR_DEVICE_LOST) {
            CASPAR_THROW_EXCEPTION(device_lost_exception() << msg_info(ss.str()));
        }
        CASPAR_THROW_EXCEPTION(vk_exception() << msg_info(ss.str()));
    }
}

#define CASPAR_VK_EXPR_STR(expr) #expr

#define VK(expr)                                                                                                       \
    if (false) {                                                                                                       \
    } else {                                                                                                           \
        VkResult _vk_result_ = (expr);                                                                                 \
        caspar::screen::vk::vk_check_error(_vk_result_, CASPAR_VK_EXPR_STR(expr), __FUNCTION__, __FILE__, __LINE__);   \
    }

#define VK2(expr)                                                                                                      \
    [&]() {                                                                                                            \
        VkResult _vk_result_ = (expr);                                                                                 \
        caspar::screen::vk::vk_check_error(_vk_result_, CASPAR_VK_EXPR_STR(expr), __FUNCTION__, __FILE__, __LINE__);   \
        return _vk_result_;                                                                                            \
    }()

}}} // namespace caspar::screen::vulkan
