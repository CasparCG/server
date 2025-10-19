/*
 * Copyright 2025
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
 * Author: Niklas Andersson, niklas@niklaspandersson.se
 */

#include "device.h"

#include "../image/image_kernel.h"
#include "buffer.h"
#include "pipeline.h"
#include "texture.h"

#include <common/array.h>
#include <common/assert.h>
#include <common/env.h>
#include <common/except.h>
#include <common/os/thread.h>

#include <VkBootstrap.h>
#include <vulkan/vulkan.hpp>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 4189)
#include <vk_mem_alloc.h>
#pragma warning(pop)

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/property_tree/ptree.hpp>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>

#include <array>
#include <deque>
#include <future>
#include <thread>

namespace caspar { namespace accelerator { namespace vulkan {

using namespace boost::asio;

inline VKAPI_ATTR VkBool32 VKAPI_CALL default_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                             VkDebugUtilsMessageTypeFlagsEXT        messageType,
                                                             const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                             void*)
{
    auto ms = vkb::to_string_message_severity(messageSeverity);
    auto mt = vkb::to_string_message_type(messageType);
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        CASPAR_LOG(info) << "[" << ms << ": " << mt << "] - " << pCallbackData->pMessageIdName << ", "
                         << pCallbackData->pMessage;
        // printf("[%s: %s] - %s\n%s\n", ms, mt, pCallbackData->pMessageIdName, pCallbackData->pMessage);
    } else {
        if (pCallbackData->pMessage != nullptr) {
            CASPAR_LOG(info) << "[" << ms << ": " << mt << "] " << pCallbackData->pMessage;
            // printf("[%s: %s]\n%s\n", ms, mt, pCallbackData->pMessage);
        }
    }

    return VK_FALSE; // Applications must return false here (Except Validation, if return true, will skip calling to
                     // driver)
}

void transitionImageLayout(const vk::Image&        image,
                           vk::ImageLayout         oldLayout,
                           vk::AccessFlags2        srcAccessMask,
                           vk::PipelineStageFlags2 srcStage,
                           vk::ImageLayout         newLayout,
                           vk::AccessFlags2        dstAccessMask,
                           vk::PipelineStageFlags2 dstStage,
                           vk::CommandBuffer       cmdBuffer)
{
    vk::PipelineStageFlags2 sourceStage;
    vk::PipelineStageFlags2 destinationStage;

    auto range = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

    vk::ImageMemoryBarrier2 barrier{};
    barrier.oldLayout = oldLayout, barrier.newLayout = newLayout, barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored,
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored, barrier.image = image, barrier.subresourceRange = range;

    barrier.srcAccessMask = srcAccessMask;
    barrier.srcStageMask  = srcStage;

    barrier.dstAccessMask = dstAccessMask;
    barrier.dstStageMask  = dstStage;

    vk::DependencyInfo dep_info;
    dep_info.setImageMemoryBarriers(barrier);

    cmdBuffer.pipelineBarrier2(dep_info);
}

struct device::impl : public std::enable_shared_from_this<impl>
{
    using texture_queue_t = tbb::concurrent_bounded_queue<std::shared_ptr<texture>>;
    using buffer_queue_t  = tbb::concurrent_bounded_queue<std::shared_ptr<buffer>>;

    std::array<tbb::concurrent_unordered_map<size_t, texture_queue_t>, 2>                attachment_pools_;
    std::array<std::array<tbb::concurrent_unordered_map<size_t, texture_queue_t>, 4>, 2> device_pools_;
    std::array<tbb::concurrent_unordered_map<size_t, buffer_queue_t>, 2>                 host_pools_;

    std::wstring version_;

    vkb::Instance                      _vkb_instance;
    vkb::PhysicalDevice                _vkb_physical_device;
    vk::PhysicalDeviceMemoryProperties _memoryProperties;
    vk::PhysicalDevice                 _physical_device;
    vk::Device                         _device;
    vk::Queue                          _queue;
    vk::CommandPool                    _command_pool;
    VmaAllocator                       _allocator;

    std::array<std::shared_ptr<pipeline>, 2> _pipelines;

    struct inflight_command_buffer
    {
        vk::CommandBuffer cmd;
        vk::Fence         fence;
    };
    std::deque<inflight_command_buffer> _transfer_cmd_buffers;

    io_context                             io_context_;
    decltype(make_work_guard(io_context_)) work_;
    std::thread                            thread_;

    impl()
        : work_(make_work_guard(io_context_))
    {
        CASPAR_LOG(info) << L"Initializing Vulkan Device.";

        auto instance_builder = vkb::InstanceBuilder()
#ifdef _DEBUG
                                    .enable_validation_layers(true)
                                    .set_debug_messenger_severity(VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
                                    .set_debug_messenger_type(VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
                                    .set_debug_callback(default_debug_callback)
#endif
                                    .set_app_name("CasparCG")
                                    .set_headless(true)
                                    .set_engine_name("CasparCG")
                                    .require_api_version(VK_API_VERSION_1_3);
        auto instance_ret = instance_builder.build();
        if (!instance_ret) {
            CASPAR_THROW_EXCEPTION(caspar_exception()
                                   << msg_info("Failed to create Vulkan instance: " + instance_ret.error().message()));
        }
        _vkb_instance = instance_ret.value();

        // Find suitable physical device
        auto gpu_selector = vkb::PhysicalDeviceSelector(_vkb_instance);

        vk::PhysicalDeviceVulkan13Features features13;
        features13.dynamicRendering = true;
        features13.synchronization2 = true;

        vk::PhysicalDeviceVulkan12Features features12;
        features12.descriptorIndexing              = true;
        features12.descriptorBindingPartiallyBound = true;

        vk::PhysicalDeviceVulkan14Features features14;
        features14.dynamicRenderingLocalRead = true;

        auto gpu_res = gpu_selector.set_minimum_version(1, 4)
                           .set_required_features_12(features12)
                           .set_required_features_13(features13)
                           .set_required_features_14(features14)
                           .select();
        if (!gpu_res) {
            CASPAR_THROW_EXCEPTION(caspar_exception()
                                   << msg_info("Failed to select physical device: " + gpu_res.error().message()));
        }
        _vkb_physical_device = gpu_res.value();

        // Create the logical device
        auto device_builder = vkb::DeviceBuilder(_vkb_physical_device);
        _physical_device    = vk::PhysicalDevice(_vkb_physical_device.physical_device);

        auto device_res = device_builder.build();
        if (!device_res) {
            CASPAR_THROW_EXCEPTION(caspar_exception()
                                   << msg_info("Failed to create device: " + device_res.error().message()));
        }
        auto vkb_device   = device_res.value();
        _device           = vk::Device(vkb_device.device);
        _queue            = vk::Queue(vkb_device.get_queue(vkb::QueueType::graphics).value());
        auto queue_family = vkb_device.get_queue_index(vkb::QueueType::graphics).value();

        vk::CommandPoolCreateInfo pool_info;
        pool_info.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        pool_info.queueFamilyIndex = queue_family;

        _command_pool = _device.createCommandPool(pool_info);

        VmaVulkanFunctions vulkanFunctions    = {};
        vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr   = &vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.flags                  = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        allocatorCreateInfo.vulkanApiVersion       = VK_API_VERSION_1_4;
        allocatorCreateInfo.physicalDevice         = _physical_device;
        allocatorCreateInfo.device                 = _device;
        allocatorCreateInfo.instance               = _vkb_instance.instance;
        allocatorCreateInfo.pVulkanFunctions       = &vulkanFunctions;

        vmaCreateAllocator(&allocatorCreateInfo, &_allocator);

        _memoryProperties = _physical_device.getMemoryProperties();

        _pipelines[0] = std::make_shared<pipeline>(_device, vk::Format::eR8G8B8A8Unorm);
        _pipelines[1] = std::make_shared<pipeline>(_device, vk::Format::eR16G16B16A16Unorm);

        thread_ = std::thread([&] {
            set_thread_name(L"Vulkan Device");
            io_context_.run();
        });
    }

    ~impl()
    {
        work_.reset();
        thread_.join();

        _device.waitIdle();

        for (auto& pool : host_pools_)
            pool.clear();

        for (auto& pool : attachment_pools_)
            pool.clear();

        for (auto& pools : device_pools_)
            for (auto& pool : pools)
                pool.clear();

        for (auto& inflight : _transfer_cmd_buffers) {
            _device.destroyFence(inflight.fence);
            inflight.fence = nullptr;
        }

        _device.destroyCommandPool(_command_pool);
        vmaDestroyAllocator(_allocator);
        for (auto& pipeline : _pipelines) {
            pipeline.reset();
        }

        _device.destroy();
        vkb::destroy_instance(_vkb_instance);
    }

    template <typename Func>
    auto spawn_async(Func&& func)
    {
        using result_type = decltype(func(std::declval<yield_context>()));
        using task_type   = std::packaged_task<result_type(yield_context)>;

        auto task   = task_type(std::forward<Func>(func));
        auto future = task.get_future();
        boost::asio::spawn(io_context_,
                           std::move(task)
#if BOOST_VERSION >= 108000
                               ,
                           [](std::exception_ptr e) {
                               if (e)
                                   std::rethrow_exception(e);
                           }
#endif
        );
        return future;
    }

    template <typename Func>
    auto dispatch_async(Func&& func)
    {
        using result_type = decltype(func());
        using task_type   = std::packaged_task<result_type()>;

        auto task   = task_type(std::forward<Func>(func));
        auto future = task.get_future();
        boost::asio::dispatch(io_context_, std::move(task));
        return future;
    }

    template <typename Func>
    auto dispatch_sync(Func&& func) -> decltype(func())
    {
        return dispatch_async(std::forward<Func>(func)).get();
    }

    std::wstring version() { return version_; }

    uint32_t findDedicatedMemoryType(uint32_t typeMask, vk::MemoryPropertyFlags properties)
    {
        for (uint32_t i = 0; i < _memoryProperties.memoryTypeCount; ++i) {
            if ((typeMask & (1 << i)) &&
                ((_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)) {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type");
    }

    void submitSingleTimeCommands(std::function<void(const vk::CommandBuffer&)> func, bool wait = false)
    {
        vk::CommandBuffer cmd_buffer = nullptr;
        vk::Fence         fence      = nullptr;
        if (_transfer_cmd_buffers.size() > 1) {
            // try to reuse the oldest existing command buffer
            auto& inflight = _transfer_cmd_buffers.front();
            auto  result   = _device.waitForFences(inflight.fence, VK_TRUE, 0);
            if (result == vk::Result::eSuccess) {
                cmd_buffer = inflight.cmd;
                cmd_buffer.reset();
                fence = inflight.fence;
                _device.resetFences(inflight.fence);
                _transfer_cmd_buffers.pop_front();
            }
        }

        if (!cmd_buffer) {
            // create a new command buffer
            vk::CommandBufferAllocateInfo allocInfo{};
            allocInfo.commandPool        = _command_pool;
            allocInfo.level              = vk::CommandBufferLevel::ePrimary;
            allocInfo.commandBufferCount = 1;

            cmd_buffer = _device.allocateCommandBuffers(allocInfo)[0];
            fence      = _device.createFence(vk::FenceCreateInfo{});
        }

        cmd_buffer.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        func(cmd_buffer);
        cmd_buffer.end();

        vk::SubmitInfo submitInfo{};
        submitInfo.setCommandBuffers(cmd_buffer);
        _queue.submit(submitInfo, fence);

        _transfer_cmd_buffers.push_back({cmd_buffer, fence});

        if (wait) {
            auto res = _device.waitForFences(fence, VK_TRUE, 1000000000);
        }
    }

    std::vector<vk::CommandBuffer> allocateCommandBuffers(uint32_t count)
    {
        return _device.allocateCommandBuffers(
            vk::CommandBufferAllocateInfo(_command_pool, vk::CommandBufferLevel::ePrimary, count));
    }
    void submit(const vk::SubmitInfo& submitInfo, vk::Fence fence) { _queue.submit(submitInfo, fence); }

    std::shared_ptr<texture>
    create_attachment(int width, int height, common::bit_depth depth, uint32_t components_count)
    {
        CASPAR_VERIFY(width > 0 && height > 0);

        auto depth_pool_index = depth == common::bit_depth::bit8 ? 0 : 1;
        auto format = depth == common::bit_depth::bit8 ? vk::Format::eR8G8B8A8Unorm : vk::Format::eR16G16B16A16Unorm;

        // TODO (perf) Shared pool.
        auto pool   = &attachment_pools_[depth_pool_index][(width << 16 & 0xFFFF0000) | (height & 0x0000FFFF)];
        auto extent = vk::Extent3D{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};

        std::shared_ptr<texture> tex;
        if (!pool->try_pop(tex)) {
            vk::ImageCreateInfo imageInfo{};
            imageInfo.imageType     = vk::ImageType::e2D;
            imageInfo.format        = format;
            imageInfo.extent        = extent;
            imageInfo.mipLevels     = 1;
            imageInfo.arrayLayers   = 1;
            imageInfo.initialLayout = vk::ImageLayout::eUndefined;
            imageInfo.samples       = vk::SampleCountFlagBits::e1;
            imageInfo.tiling        = vk::ImageTiling::eOptimal;
            imageInfo.usage         = vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eInputAttachment |
                              vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst |
                              vk::ImageUsageFlagBits::eSampled;
            imageInfo.sharingMode = vk::SharingMode::eExclusive;
            auto image            = _device.createImage(imageInfo);

            auto memReq = _device.getImageMemoryRequirements(image);

            vk::MemoryAllocateInfo allocInfo{};
            allocInfo.allocationSize = memReq.size;
            allocInfo.memoryTypeIndex =
                findDedicatedMemoryType(memReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

            auto imageMemory = _device.allocateMemory(allocInfo);
            _device.bindImageMemory(image, imageMemory, 0);
            auto range = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

            vk::ImageViewCreateInfo createInfo(
                {}, image, vk::ImageViewType::e2D, format, vk::ComponentMapping(), range);

            auto imageView = _device.createImageView(createInfo);

            tex = std::make_shared<texture>(
                width, height, components_count, depth, image, imageMemory, imageView, _device);
        }

        submitSingleTimeCommands([&](vk::CommandBuffer cmd) {
            transitionImageLayout(
                tex->id(),
                vk::ImageLayout::eUndefined,
                vk::AccessFlagBits2::eNone,
                vk::PipelineStageFlagBits2::eTopOfPipe,
                vk::ImageLayout::eRenderingLocalRead,
                vk::AccessFlagBits2::eColorAttachmentWrite | vk::AccessFlagBits2::eInputAttachmentRead,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput | vk::PipelineStageFlagBits2::eFragmentShader,
                cmd);
        });

        tex->set_depth(depth);

        auto ptr = tex.get();
        return std::shared_ptr<texture>(
            ptr, [tex = std::move(tex), pool, self = shared_from_this()](texture*) mutable { pool->push(tex); });
    }

    std::shared_ptr<texture> create_texture(int width, int height, int stride, common::bit_depth depth, bool clear)
    {
        CASPAR_VERIFY(stride > 0 && stride < 5);
        CASPAR_VERIFY(width > 0 && height > 0);

        static vk::Format INTERNAL_FORMAT[][5] = {{vk::Format::eUndefined,
                                                   vk::Format::eR8Unorm,
                                                   vk::Format::eR8G8Unorm,
                                                   vk::Format::eR8G8B8Unorm,
                                                   vk::Format::eR8G8B8A8Unorm},
                                                  {vk::Format::eUndefined,
                                                   vk::Format::eR16Unorm,
                                                   vk::Format::eR16G16Unorm,
                                                   vk::Format::eR16G16B16Unorm,
                                                   vk::Format::eR16G16B16A16Unorm}};

        auto depth_pool_index = depth == common::bit_depth::bit8 ? 0 : 1;
        auto format           = INTERNAL_FORMAT[depth_pool_index][stride];

        auto pool   = &device_pools_[depth_pool_index][stride - 1][(width << 16 & 0xFFFF0000) | (height & 0x0000FFFF)];
        auto extent = vk::Extent3D{static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
        std::shared_ptr<texture> tex;
        if (!pool->try_pop(tex)) {
            vk::ImageCreateInfo imageInfo{};
            imageInfo.imageType     = vk::ImageType::e2D;
            imageInfo.format        = format;
            imageInfo.extent        = extent;
            imageInfo.mipLevels     = 1;
            imageInfo.arrayLayers   = 1;
            imageInfo.initialLayout = vk::ImageLayout::eUndefined;
            imageInfo.samples       = vk::SampleCountFlagBits::e1;
            imageInfo.tiling        = vk::ImageTiling::eOptimal;
            imageInfo.usage         = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
            imageInfo.sharingMode   = vk::SharingMode::eExclusive;
            auto image              = _device.createImage(imageInfo);

            auto memReq = _device.getImageMemoryRequirements(image);

            vk::MemoryAllocateInfo allocInfo{};
            allocInfo.allocationSize = memReq.size;
            allocInfo.memoryTypeIndex =
                findDedicatedMemoryType(memReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);

            auto imageMemory = _device.allocateMemory(allocInfo);
            _device.bindImageMemory(image, imageMemory, 0);
            auto clearValue = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
            auto range      = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);

            vk::ImageViewCreateInfo createInfo(
                {}, image, vk::ImageViewType::e2D, format, vk::ComponentMapping(), range);

            auto imageView = _device.createImageView(createInfo);

            tex = std::make_shared<texture>(width, height, stride, depth, image, imageMemory, imageView, _device);
        }
        tex->set_depth(depth);

        if (clear) {
            tex->clear();
        }

        auto ptr = tex.get();
        return std::shared_ptr<texture>(
            ptr, [tex = std::move(tex), pool, self = shared_from_this()](texture*) mutable { pool->push(tex); });
    }

    std::shared_ptr<buffer> create_buffer(int size, bool write)
    {
        CASPAR_VERIFY(size > 0);

        // TODO (perf) Shared pool.
        auto pool = &host_pools_[static_cast<int>(write ? 1 : 0)][size];

        std::shared_ptr<buffer> buf;
        if (!pool->try_pop(buf)) {
            buf = std::make_shared<buffer>(size, write, _allocator);
        }

        auto ptr = buf.get();
        return std::shared_ptr<buffer>(ptr, [buf = std::move(buf), self = shared_from_this()](buffer*) mutable {
            auto pool = &self->host_pools_[static_cast<int>(buf->write() ? 1 : 0)][buf->size()];
            pool->push(std::move(buf));
        });
    }

    array<uint8_t> create_array(int size)
    {
        auto buf = create_buffer(size, true);
        auto ptr = reinterpret_cast<uint8_t*>(buf->data());
        return array<uint8_t>(ptr, buf->size(), std::move(buf));
    }

    std::future<std::shared_ptr<texture>>
    copy_async(const array<const uint8_t>& source, int width, int height, int stride, common::bit_depth depth)
    {
        return dispatch_async([=] {
            std::shared_ptr<buffer> buf;

            auto tmp = source.storage<std::shared_ptr<buffer>>();
            if (tmp) {
                buf = *tmp;
            } else {
                buf = create_buffer(static_cast<int>(source.size()), true);
                std::memcpy(buf->data(), source.data(), source.size());
            }

            static vk::Format INTERNAL_FORMAT[][5] = {{vk::Format::eUndefined,
                                                       vk::Format::eR8Unorm,
                                                       vk::Format::eR8G8Unorm,
                                                       vk::Format::eR8G8B8Unorm,
                                                       vk::Format::eR8G8B8A8Unorm},
                                                      {vk::Format::eUndefined,
                                                       vk::Format::eR16Unorm,
                                                       vk::Format::eR16G16Unorm,
                                                       vk::Format::eR16G16B16Unorm,
                                                       vk::Format::eR16G16B16A16Unorm}};
            auto              depth_pool_index     = depth == common::bit_depth::bit8 ? 0 : 1;

            auto tex    = create_texture(width, height, stride, depth, false);
            auto format = INTERNAL_FORMAT[depth_pool_index][stride];

            vk::BufferImageCopy region(0,
                                       0,
                                       0,
                                       vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                                       vk::Offset3D(0, 0, 0),
                                       vk::Extent3D(width, height, 1));

            submitSingleTimeCommands(
                [&](vk::CommandBuffer cmd) {
                    transitionImageLayout(tex->id(),
                                          vk::ImageLayout::eUndefined,
                                          vk::AccessFlagBits2::eNone,
                                          vk::PipelineStageFlagBits2::eTopOfPipe,

                                          vk::ImageLayout::eTransferDstOptimal,
                                          vk::AccessFlagBits2::eTransferWrite,
                                          vk::PipelineStageFlagBits2::eTransfer,
                                          cmd);

                    cmd.copyBufferToImage(buf->id(), tex->id(), vk::ImageLayout::eTransferDstOptimal, region);

                    transitionImageLayout(tex->id(),
                                          vk::ImageLayout::eTransferDstOptimal,
                                          vk::AccessFlagBits2::eTransferWrite,
                                          vk::PipelineStageFlagBits2::eTransfer,

                                          vk::ImageLayout::eShaderReadOnlyOptimal,
                                          vk::AccessFlagBits2::eShaderRead,
                                          vk::PipelineStageFlagBits2::eFragmentShader,
                                          cmd);
                },
                true);
            // TODO: return the fence and wait on it in the callers thread instead

            return tex;
        });
    }

    std::future<array<const uint8_t>> copy_async(const std::shared_ptr<texture>& source)
    {
        return spawn_async([=](yield_context yield) {
            auto buf = create_buffer(source->size(), false);

            vk::CopyImageToBufferInfo2 copyInfo{};
            copyInfo.dstBuffer      = buf->id();
            copyInfo.srcImage       = source->id();
            copyInfo.srcImageLayout = vk::ImageLayout::eTransferSrcOptimal;

            vk::BufferImageCopy2 region{};
            region.bufferOffset     = 0;
            region.imageSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
            region.imageOffset      = vk::Offset3D{0, 0, 0};
            region.imageExtent =
                vk::Extent3D{static_cast<uint32_t>(source->width()), static_cast<uint32_t>(source->height()), 1};
            copyInfo.setRegions(region);

            submitSingleTimeCommands(
                [&](vk::CommandBuffer cmd) {
                    transitionImageLayout(source->id(),
                                          vk::ImageLayout::eRenderingLocalRead,
                                          vk::AccessFlagBits2::eColorAttachmentWrite,
                                          vk::PipelineStageFlagBits2::eColorAttachmentOutput,

                                          vk::ImageLayout::eTransferSrcOptimal,
                                          vk::AccessFlagBits2::eHostRead,
                                          vk::PipelineStageFlagBits2::eHost,
                                          cmd);
                    cmd.copyImageToBuffer2(copyInfo);
                },
                true);
            // TODO: return the fence and wait on it in the callers thread instead

            auto ptr  = reinterpret_cast<uint8_t*>(buf->data());
            auto size = buf->size();
            return array<const uint8_t>(ptr, size, std::move(buf));
        });
    }

    boost::property_tree::wptree info() const
    {
        boost::property_tree::wptree info;

        boost::property_tree::wptree pooled_device_buffers;
        size_t                       total_pooled_device_buffer_size  = 0;
        size_t                       total_pooled_device_buffer_count = 0;

        for (size_t i = 0; i < device_pools_.size(); ++i) {
            auto& depth_pools = device_pools_.at(i);
            for (size_t j = 0; j < depth_pools.size(); ++j) {
                auto& pools      = depth_pools.at(j);
                bool  mipmapping = j > 3;
                auto  stride     = mipmapping ? j - 3 : j + 1;

                for (auto& pool : pools) {
                    auto width  = pool.first >> 16;
                    auto height = pool.first & 0x0000FFFF;
                    auto size   = width * height * stride;
                    auto count  = pool.second.size();

                    if (count == 0)
                        continue;

                    boost::property_tree::wptree pool_info;

                    pool_info.add(L"stride", stride);
                    pool_info.add(L"mipmapping", mipmapping);
                    pool_info.add(L"width", width);
                    pool_info.add(L"height", height);
                    pool_info.add(L"size", size);
                    pool_info.add(L"count", count);

                    total_pooled_device_buffer_size += size * count;
                    total_pooled_device_buffer_count += count;

                    pooled_device_buffers.add_child(L"device_buffer_pool", pool_info);
                }
            }
        }

        info.add_child(L"gl.details.pooled_device_buffers", pooled_device_buffers);

        boost::property_tree::wptree pooled_host_buffers;
        size_t                       total_read_size   = 0;
        size_t                       total_write_size  = 0;
        size_t                       total_read_count  = 0;
        size_t                       total_write_count = 0;

        for (size_t i = 0; i < host_pools_.size(); ++i) {
            auto& pools    = host_pools_.at(i);
            auto  is_write = i == 1;

            for (auto& pool : pools) {
                auto size  = pool.first;
                auto count = pool.second.size();

                if (count == 0)
                    continue;

                boost::property_tree::wptree pool_info;

                pool_info.add(L"usage", is_write ? L"write_only" : L"read_only");
                pool_info.add(L"size", size);
                pool_info.add(L"count", count);

                pooled_host_buffers.add_child(L"host_buffer_pool", pool_info);

                (is_write ? total_write_count : total_read_count) += count;
                (is_write ? total_write_size : total_read_size) += size * count;
            }
        }

        info.add_child(L"gl.details.pooled_host_buffers", pooled_host_buffers);
        info.add(L"gl.summary.pooled_device_buffers.total_count", total_pooled_device_buffer_count);
        info.add(L"gl.summary.pooled_device_buffers.total_size", total_pooled_device_buffer_size);
        // info.add_child(L"gl.summary.all_device_buffers", texture::info());
        info.add(L"gl.summary.pooled_host_buffers.total_read_count", total_read_count);
        info.add(L"gl.summary.pooled_host_buffers.total_write_count", total_write_count);
        info.add(L"gl.summary.pooled_host_buffers.total_read_size", total_read_size);
        info.add(L"gl.summary.pooled_host_buffers.total_write_size", total_write_size);
        info.add_child(L"gl.summary.all_host_buffers", buffer::info());

        return info;
    }

    std::future<void> gc()
    {
        return spawn_async([=](yield_context yield) {
            CASPAR_LOG(info) << " vulkan: Running GC.";

            try {
                for (auto& depth_pools : device_pools_) {
                    for (auto& pools : depth_pools) {
                        for (auto& pool : pools)
                            pool.second.clear();
                    }
                }
                for (auto& pools : host_pools_) {
                    for (auto& pool : pools)
                        pool.second.clear();
                }
                for (auto& pools : attachment_pools_) {
                    for (auto& pool : pools)
                        pool.second.clear();
                }
            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
            }
        });
    }
};

device::device()
    : impl_(new impl())
{
}
device::~device() {}

vk::PhysicalDeviceMemoryProperties device::getMemoryProperties() { return impl_->_memoryProperties; }
std::vector<vk::CommandBuffer>     device::allocateCommandBuffers(uint32_t count)
{
    return impl_->allocateCommandBuffers(count);
}
void       device::submit(const vk::SubmitInfo& submitInfo, vk::Fence fence) { impl_->submit(submitInfo, fence); }
vk::Device device::getVkDevice() const { return impl_->_device; }
std::shared_ptr<pipeline> device::get_pipeline(common::bit_depth depth)
{
    return impl_->_pipelines[depth == common::bit_depth::bit8 ? 0 : 1];
}

std::shared_ptr<texture>
device::create_attachment(int width, int height, common::bit_depth depth, uint32_t components_count)
{
    return impl_->create_attachment(width, height, depth, components_count);
}

std::shared_ptr<texture> device::create_texture(int width, int height, int stride, common::bit_depth depth)
{
    return impl_->create_texture(width, height, stride, depth, true);
}
array<uint8_t> device::create_array(int size) { return impl_->create_array(size); }
std::future<std::shared_ptr<texture>>
device::copy_async(const array<const uint8_t>& source, int width, int height, int stride, common::bit_depth depth)
{
    return impl_->copy_async(source, width, height, stride, depth);
}
std::future<array<const uint8_t>> device::copy_async(const std::shared_ptr<texture>& source)
{
    return impl_->copy_async(source);
}
void device::dispatch(std::function<void()> func) { boost::asio::dispatch(impl_->io_context_, std::move(func)); }
std::wstring                 device::version() const { return impl_->version(); }
boost::property_tree::wptree device::info() const { return impl_->info(); }
std::future<void>            device::gc() { return impl_->gc(); }
}}} // namespace caspar::accelerator::vulkan
