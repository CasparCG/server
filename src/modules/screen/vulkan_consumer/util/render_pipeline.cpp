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

#include "render_pipeline.h"
#include "swapchain.h"

#include <accelerator/vulkan/util/texture.h>

#include <common/log.h>

#include <vulkan/vulkan.hpp>

#include "screen_vk_fragment.h"
#include "screen_vk_vertex.h"

#include <cstring>

namespace caspar { namespace screen { namespace vulkan {

namespace {

// SPIR-V is embedded via bin2c (see CMakeLists). bin2c appends a trailing 0x00,
// so the real byte count is sizeof(arr) - 1 (a multiple of 4 for SPIR-V). Copy
// into a uint32_t vector to guarantee alignment for vkCreateShaderModule.
std::vector<uint32_t> embedded_spirv(const uint8_t* data, size_t size)
{
    std::vector<uint32_t> buffer(size / sizeof(uint32_t));
    std::memcpy(buffer.data(), data, buffer.size() * sizeof(uint32_t));
    return buffer;
}

} // anonymous namespace

struct render_pipeline::impl
{
    vk::Device         device_;
    vk::PhysicalDevice physical_device_;
    vk::CommandPool    command_pool_;
    vk::Queue          queue_;
    swapchain*         swapchain_ = nullptr;

    vk::ShaderModule        vert_shader_module_;
    vk::ShaderModule        frag_shader_module_;
    vk::RenderPass          render_pass_;
    vk::DescriptorSetLayout descriptor_layout_;
    vk::PipelineLayout      pipeline_layout_;
    vk::Pipeline            pipeline_;
    vk::DescriptorPool      descriptor_pool_;
    vk::Sampler             sampler_;

    std::vector<vk::Framebuffer>   framebuffers_;
    std::vector<vk::CommandBuffer> command_buffers_;
    std::vector<vk::DescriptorSet> descriptor_sets_; // one per frame-in-flight slot

    impl(vk::Device         device,
         vk::PhysicalDevice physical_device,
         vk::CommandPool    command_pool,
         vk::Queue          queue,
         swapchain&         swap)
        : device_(device)
        , physical_device_(physical_device)
        , command_pool_(command_pool)
        , queue_(queue)
        , swapchain_(&swap)
    {
        create_shader_modules();
        create_render_pass();
        create_sampler();
        create_descriptor_layout();
        create_pipeline_layout();
        create_pipeline();
        create_descriptor_pool();
        create_framebuffers();
        create_command_buffers();
    }

    ~impl()
    {
        if (device_) {
            device_.waitIdle();

            cleanup_framebuffers();

            if (descriptor_pool_) {
                device_.destroyDescriptorPool(descriptor_pool_);
            }
            if (sampler_) {
                device_.destroySampler(sampler_);
            }
            if (pipeline_) {
                device_.destroyPipeline(pipeline_);
            }
            if (pipeline_layout_) {
                device_.destroyPipelineLayout(pipeline_layout_);
            }
            if (descriptor_layout_) {
                device_.destroyDescriptorSetLayout(descriptor_layout_);
            }
            if (render_pass_) {
                device_.destroyRenderPass(render_pass_);
            }
            if (vert_shader_module_) {
                device_.destroyShaderModule(vert_shader_module_);
            }
            if (frag_shader_module_) {
                device_.destroyShaderModule(frag_shader_module_);
            }
        }
    }

    void create_shader_modules()
    {
        auto vert_spirv =
            embedded_spirv(caspar::screen::vertex_shader_spv, sizeof(caspar::screen::vertex_shader_spv) - 1);
        vk::ShaderModuleCreateInfo vertCreateInfo{};
        vertCreateInfo.codeSize = vert_spirv.size() * sizeof(uint32_t);
        vertCreateInfo.pCode    = vert_spirv.data();
        vert_shader_module_     = device_.createShaderModule(vertCreateInfo);

        auto frag_spirv =
            embedded_spirv(caspar::screen::fragment_shader_spv, sizeof(caspar::screen::fragment_shader_spv) - 1);
        vk::ShaderModuleCreateInfo fragCreateInfo{};
        fragCreateInfo.codeSize = frag_spirv.size() * sizeof(uint32_t);
        fragCreateInfo.pCode    = frag_spirv.data();
        frag_shader_module_     = device_.createShaderModule(fragCreateInfo);
    }

    void create_render_pass()
    {
        vk::AttachmentDescription colorAttachment{};
        colorAttachment.format         = swapchain_->format();
        colorAttachment.samples        = vk::SampleCountFlagBits::e1;
        colorAttachment.loadOp         = vk::AttachmentLoadOp::eClear;
        colorAttachment.storeOp        = vk::AttachmentStoreOp::eStore;
        colorAttachment.stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
        colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        colorAttachment.initialLayout  = vk::ImageLayout::eUndefined;
        colorAttachment.finalLayout    = vk::ImageLayout::ePresentSrcKHR;

        vk::AttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout     = vk::ImageLayout::eColorAttachmentOptimal;

        vk::SubpassDescription subpass{};
        subpass.pipelineBindPoint    = vk::PipelineBindPoint::eGraphics;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments    = &colorAttachmentRef;

        vk::SubpassDependency dependency{};
        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.srcAccessMask = {};
        dependency.dstStageMask  = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        vk::RenderPassCreateInfo renderPassInfo{};
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments    = &colorAttachment;
        renderPassInfo.subpassCount    = 1;
        renderPassInfo.pSubpasses      = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies   = &dependency;

        render_pass_ = device_.createRenderPass(renderPassInfo);
    }

    void create_sampler()
    {
        vk::SamplerCreateInfo samplerInfo{};
        samplerInfo.magFilter               = vk::Filter::eLinear;
        samplerInfo.minFilter               = vk::Filter::eLinear;
        samplerInfo.addressModeU            = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.addressModeV            = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.addressModeW            = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.anisotropyEnable        = VK_FALSE;
        samplerInfo.maxAnisotropy           = 1.0f;
        samplerInfo.borderColor             = vk::BorderColor::eIntOpaqueBlack;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable           = VK_FALSE;
        samplerInfo.compareOp               = vk::CompareOp::eAlways;
        // Use NEAREST mipmap mode since textures have only 1 mip level
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
        samplerInfo.minLod     = 0.0f;
        samplerInfo.maxLod     = 0.0f;

        sampler_ = device_.createSampler(samplerInfo);
    }

    void create_descriptor_layout()
    {
        vk::DescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding            = 0;
        samplerBinding.descriptorType     = vk::DescriptorType::eCombinedImageSampler;
        samplerBinding.descriptorCount    = 1;
        samplerBinding.stageFlags         = vk::ShaderStageFlagBits::eFragment;
        samplerBinding.pImmutableSamplers = nullptr;

        vk::DescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings    = &samplerBinding;

        descriptor_layout_ = device_.createDescriptorSetLayout(layoutInfo);
    }

    void create_pipeline_layout()
    {
        vk::PushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
        pushConstantRange.offset     = 0;
        pushConstantRange.size       = sizeof(screen_push_constants);

        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.setLayoutCount         = 1;
        pipelineLayoutInfo.pSetLayouts            = &descriptor_layout_;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges    = &pushConstantRange;

        pipeline_layout_ = device_.createPipelineLayout(pipelineLayoutInfo);
    }

    void create_pipeline()
    {
        vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.stage  = vk::ShaderStageFlagBits::eVertex;
        vertShaderStageInfo.module = vert_shader_module_;
        vertShaderStageInfo.pName  = "main";

        vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.stage  = vk::ShaderStageFlagBits::eFragment;
        fragShaderStageInfo.module = frag_shader_module_;
        fragShaderStageInfo.pName  = "main";

        vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        // No vertex input - we generate vertices in the shader
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.topology               = vk::PrimitiveTopology::eTriangleList;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Dynamic viewport and scissor
        vk::PipelineViewportStateCreateInfo viewportState{};
        viewportState.viewportCount = 1;
        viewportState.scissorCount  = 1;

        vk::PipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.depthClampEnable        = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode             = vk::PolygonMode::eFill;
        rasterizer.lineWidth               = 1.0f;
        rasterizer.cullMode                = vk::CullModeFlagBits::eNone;
        rasterizer.frontFace               = vk::FrontFace::eCounterClockwise;
        rasterizer.depthBiasEnable         = VK_FALSE;

        vk::PipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sampleShadingEnable  = VK_FALSE;
        multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

        vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        colorBlendAttachment.blendEnable = VK_FALSE;

        vk::PipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.logicOpEnable   = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments    = &colorBlendAttachment;

        std::vector<vk::DynamicState> dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};

        vk::PipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates    = dynamicStates.data();

        vk::GraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.stageCount          = 2;
        pipelineInfo.pStages             = shaderStages;
        pipelineInfo.pVertexInputState   = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState      = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState   = &multisampling;
        pipelineInfo.pColorBlendState    = &colorBlending;
        pipelineInfo.pDynamicState       = &dynamicState;
        pipelineInfo.layout              = pipeline_layout_;
        pipelineInfo.renderPass          = render_pass_;
        pipelineInfo.subpass             = 0;

        pipeline_ = device_.createGraphicsPipeline(nullptr, pipelineInfo).value;
    }

    void create_descriptor_pool()
    {
        uint32_t count = swapchain_->max_frames_in_flight();

        vk::DescriptorPoolSize poolSize{};
        poolSize.type            = vk::DescriptorType::eCombinedImageSampler;
        poolSize.descriptorCount = count;

        vk::DescriptorPoolCreateInfo poolInfo{};
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes    = &poolSize;
        poolInfo.maxSets       = count;

        descriptor_pool_ = device_.createDescriptorPool(poolInfo);

        // One descriptor set per frame-in-flight slot (not per swapchain image).
        // The per-slot fence guarantees the set is no longer in use by the GPU
        // before we update it for the next frame in the same slot.
        std::vector<vk::DescriptorSetLayout> layouts(count, descriptor_layout_);
        vk::DescriptorSetAllocateInfo        allocInfo{};
        allocInfo.descriptorPool     = descriptor_pool_;
        allocInfo.descriptorSetCount = count;
        allocInfo.pSetLayouts        = layouts.data();

        descriptor_sets_ = device_.allocateDescriptorSets(allocInfo);
    }

    void create_framebuffers()
    {
        uint32_t width, height;
        swapchain_->get_extent(width, height);

        framebuffers_.resize(swapchain_->image_count());

        for (uint32_t i = 0; i < swapchain_->image_count(); ++i) {
            vk::ImageView attachments[] = {swapchain_->get_image_view(i)};

            vk::FramebufferCreateInfo framebufferInfo{};
            framebufferInfo.renderPass      = render_pass_;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments    = attachments;
            framebufferInfo.width           = width;
            framebufferInfo.height          = height;
            framebufferInfo.layers          = 1;

            framebuffers_[i] = device_.createFramebuffer(framebufferInfo);
        }
    }

    void create_command_buffers()
    {
        // One command buffer per frame-in-flight slot, not per swapchain image.
        // This ensures the per-slot fence always guards the matching command
        // buffer, preventing reuse while the GPU is still executing it.
        vk::CommandBufferAllocateInfo allocInfo{};
        allocInfo.commandPool        = command_pool_;
        allocInfo.level              = vk::CommandBufferLevel::ePrimary;
        allocInfo.commandBufferCount = swapchain_->max_frames_in_flight();

        command_buffers_ = device_.allocateCommandBuffers(allocInfo);
    }

    void cleanup_framebuffers()
    {
        if (!command_buffers_.empty()) {
            device_.freeCommandBuffers(command_pool_, command_buffers_);
            command_buffers_.clear();
        }

        for (auto framebuffer : framebuffers_) {
            device_.destroyFramebuffer(framebuffer);
        }
        framebuffers_.clear();
    }

    void recreate_framebuffers()
    {
        device_.waitIdle();
        cleanup_framebuffers();
        create_framebuffers();
        create_command_buffers();
    }

    void render(accelerator::vulkan::texture& src,
                uint32_t                      image_index,
                uint32_t                      frame_slot,
                const screen_push_constants&  params,
                vk::Semaphore                 wait_semaphore,
                vk::Semaphore                 signal_semaphore,
                vk::Fence                     fence)
    {
        // Use the frame-slot index for the command buffer so it is always
        // guarded by the matching in_flight fence (waited before this call).
        auto cmdBuffer = command_buffers_[frame_slot];

        // Reset and begin command buffer
        cmdBuffer.reset();

        vk::CommandBufferBeginInfo beginInfo{};
        cmdBuffer.begin(beginInfo);

        // `src` is already shader-read on this queue (see render_pipeline.h): the
        // upload barrier's dst scope synchronizes this same-queue draw, so no
        // layout transition or wait is recorded here.

        // Use the pre-allocated descriptor set for this frame slot.
        // The per-slot fence (waited before this call) guarantees the GPU is
        // no longer reading from it.
        vk::DescriptorSet descriptorSet = descriptor_sets_[frame_slot];

        // Update descriptor set with source texture
        vk::DescriptorImageInfo imageInfo{};
        imageInfo.sampler     = sampler_;
        imageInfo.imageView   = src.view();
        imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        vk::WriteDescriptorSet descriptorWrite{};
        descriptorWrite.dstSet          = descriptorSet;
        descriptorWrite.dstBinding      = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType  = vk::DescriptorType::eCombinedImageSampler;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo      = &imageInfo;

        device_.updateDescriptorSets(descriptorWrite, {});

        // Begin render pass
        uint32_t width, height;
        swapchain_->get_extent(width, height);

        vk::RenderPassBeginInfo renderPassInfo{};
        renderPassInfo.renderPass        = render_pass_;
        renderPassInfo.framebuffer       = framebuffers_[image_index];
        renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
        renderPassInfo.renderArea.extent = vk::Extent2D{width, height};

        vk::ClearValue clearColor;
        clearColor.color               = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues    = &clearColor;

        cmdBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

        // Bind pipeline
        cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);

        // Set viewport and scissor
        vk::Viewport viewport{};
        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = static_cast<float>(width);
        viewport.height   = static_cast<float>(height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        cmdBuffer.setViewport(0, viewport);

        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D{0, 0};
        scissor.extent = vk::Extent2D{width, height};
        cmdBuffer.setScissor(0, scissor);

        // Bind descriptor set
        cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout_, 0, descriptorSet, {});

        // Push constants
        cmdBuffer.pushConstants<screen_push_constants>(
            pipeline_layout_, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, params);

        // Draw fullscreen quad (6 vertices, no vertex buffer)
        cmdBuffer.draw(6, 1, 0, 0);

        cmdBuffer.endRenderPass();

        cmdBuffer.end();

        // Wait the swapchain image-available (binary) at color-output, signal the
        // render-finished (binary). The texture sync is handled by the upload
        // barrier (same queue, submission order) — no extra semaphore here.
        vk::SemaphoreSubmitInfo image_available{};
        image_available.semaphore = wait_semaphore;
        image_available.stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;

        vk::SemaphoreSubmitInfo signal{};
        signal.semaphore = signal_semaphore;
        signal.stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput;

        vk::CommandBufferSubmitInfo cmd_info{};
        cmd_info.commandBuffer = cmdBuffer;

        vk::SubmitInfo2 submitInfo{};
        submitInfo.setWaitSemaphoreInfos(image_available);
        submitInfo.setCommandBufferInfos(cmd_info);
        submitInfo.setSignalSemaphoreInfos(signal);

        queue_.submit2(submitInfo, fence);
    }
};

render_pipeline::render_pipeline(vk::Device         device,
                                 vk::PhysicalDevice physical_device,
                                 vk::CommandPool    command_pool,
                                 vk::Queue          queue,
                                 swapchain&         swap)
    : impl_(std::make_unique<impl>(device, physical_device, command_pool, queue, swap))
{
}

render_pipeline::~render_pipeline() = default;

void render_pipeline::render(accelerator::vulkan::texture& src,
                             uint32_t                      image_index,
                             uint32_t                      frame_slot,
                             const screen_push_constants&  params,
                             vk::Semaphore                 wait_semaphore,
                             vk::Semaphore                 signal_semaphore,
                             vk::Fence                     fence)
{
    impl_->render(src, image_index, frame_slot, params, wait_semaphore, signal_semaphore, fence);
}

void render_pipeline::recreate_framebuffers() { impl_->recreate_framebuffers(); }

}}} // namespace caspar::screen::vulkan
