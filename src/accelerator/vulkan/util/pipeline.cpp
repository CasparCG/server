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

#include "pipeline.h"
#include "../image/image_kernel.h"
#include "texture.h"

#include "vulkan_image_fragment.h"
#include "vulkan_image_vertex.h"
#include <core/frame/geometry.h>

#include <vulkan/vulkan.hpp>

#include <unordered_map>

namespace caspar { namespace accelerator { namespace vulkan {

std::vector<vk::PipelineShaderStageCreateInfo> create_shader_program(vk::Device device)
{
    // Helper to create shader module
    auto createShaderModule = [&](const uint8_t* code, size_t size) {
        vk::ShaderModuleCreateInfo createInfo{};
        createInfo.codeSize = size;
        createInfo.pCode    = reinterpret_cast<const uint32_t*>(code);
        return device.createShaderModule(createInfo);
    };

    auto vertShaderModule = createShaderModule(vertex_shader, sizeof(vertex_shader) - 1);
    auto fragShaderModule = createShaderModule(fragment_shader, sizeof(fragment_shader) - 1);

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
    vertShaderStageInfo.stage  = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName  = "main";

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
    fragShaderStageInfo.stage  = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName  = "main";

    return {vertShaderStageInfo, fragShaderStageInfo};
}

std::array<vk::VertexInputAttributeDescription, 2> get_attribute_descriptions(uint32_t binding)
{
    std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions{
        {{0, binding, vk::Format::eR32G32Sfloat, 0}, {1, binding, vk::Format::eR32G32B32A32Sfloat, 2 * sizeof(float)}}};

    return attributeDescriptions;
}

const int BindlessTextureCount = 8;

struct pipeline::impl
{
    vk::Device device_;
    vk::Format format_;

    // Stand-in for absent plane textures on MoltenVK (no null-descriptor support).
    vk::ImageView dummyView_;

    vk::Sampler             textureSampler_;
    vk::Sampler             keySampler_;
    vk::DescriptorSetLayout descriptorSetLayout_;

    vk::PipelineLayout pipelineLayout_;
    vk::Pipeline       pipeline_;

    impl(const impl&)            = delete;
    impl& operator=(const impl&) = delete;

    void setup_descriptors()
    {
        // Binding 0: bindless texture array for planes (up to 4), local_key, and layer_key
        vk::DescriptorSetLayoutBinding texturesLayoutBinding{};
        texturesLayoutBinding.binding         = 0;
        texturesLayoutBinding.descriptorType  = vk::DescriptorType::eCombinedImageSampler;
        texturesLayoutBinding.descriptorCount = BindlessTextureCount;
        texturesLayoutBinding.stageFlags      = vk::ShaderStageFlagBits::eFragment;

        // Binding 1: input attachment for background
        vk::DescriptorSetLayoutBinding backgroundLayoutBinding{};
        backgroundLayoutBinding.binding         = 1;
        backgroundLayoutBinding.descriptorType  = vk::DescriptorType::eInputAttachment;
        backgroundLayoutBinding.descriptorCount = 1;
        backgroundLayoutBinding.stageFlags      = vk::ShaderStageFlagBits::eFragment;

        vk::DescriptorSetLayoutCreateInfo layoutInfo{};
        std::array                        bindings{texturesLayoutBinding, backgroundLayoutBinding};
        layoutInfo.setBindings(bindings);

        std::array<vk::DescriptorBindingFlags, 2>     bindingFlags{vk::DescriptorBindingFlagBits::ePartiallyBound,
                                                               vk::DescriptorBindingFlags{}};
        vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo;
        bindingFlagsInfo.setBindingFlags(bindingFlags);
        layoutInfo.pNext = &bindingFlagsInfo;

        descriptorSetLayout_ = device_.createDescriptorSetLayout(layoutInfo);
    }

    // Per-set descriptor counts a pool must provide to allocate this pipeline's
    // layout: the bindless texture array (binding 0) plus one input attachment
    // (binding 1).
    std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes() const
    {
        return {{vk::DescriptorType::eCombinedImageSampler, BindlessTextureCount},
                {vk::DescriptorType::eInputAttachment, 1}};
    }

    void setup_sampler()
    {
        vk::SamplerCreateInfo samplerInfo{};

        samplerInfo.magFilter               = vk::Filter::eLinear;
        samplerInfo.minFilter               = vk::Filter::eLinear;
        samplerInfo.mipmapMode              = vk::SamplerMipmapMode::eLinear;
        samplerInfo.addressModeU            = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeV            = vk::SamplerAddressMode::eRepeat;
        samplerInfo.addressModeW            = vk::SamplerAddressMode::eRepeat;
        samplerInfo.mipLodBias              = 0.0f;
        samplerInfo.anisotropyEnable        = VK_FALSE;
        samplerInfo.maxAnisotropy           = 2;
        samplerInfo.compareEnable           = VK_FALSE;
        samplerInfo.compareOp               = vk::CompareOp::eAlways;
        samplerInfo.minLod                  = 0.0f;
        samplerInfo.maxLod                  = 0.0f;
        samplerInfo.borderColor             = vk::BorderColor::eIntOpaqueBlack;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        textureSampler_ = device_.createSampler(samplerInfo);

        samplerInfo.magFilter  = vk::Filter::eNearest;
        samplerInfo.minFilter  = vk::Filter::eNearest;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
        keySampler_            = device_.createSampler(samplerInfo);
    }

  public:
    impl(vk::Device device, vk::Format format, vk::ImageView dummy_view)
        : device_(device)
        , format_(format)
        , dummyView_(dummy_view)
    {
        setup_descriptors();

        setup_sampler();

        // Vertex input
        auto attributeDescriptions = get_attribute_descriptions(0);

        auto vertexBindings = vk::VertexInputBindingDescription(0, sizeof(float) * 6, vk::VertexInputRate::eVertex);
        vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
        vertexInputInfo.setVertexBindingDescriptions(vertexBindings);
        vertexInputInfo.setVertexAttributeDescriptions(attributeDescriptions);

        // Input assembly
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.topology               = vk::PrimitiveTopology::eTriangleFan;
        inputAssembly.primitiveRestartEnable = VK_TRUE;

        vk::PipelineViewportStateCreateInfo viewportState{};
        viewportState.scissorCount  = 1;
        viewportState.viewportCount = 1;
        vk::DynamicState dynamicStates[]{vk::DynamicState::eViewport, vk::DynamicState::eScissor};

        // Rasterizer
        vk::PipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.depthClampEnable        = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode             = vk::PolygonMode::eFill;
        rasterizer.cullMode                = vk::CullModeFlagBits::eNone;
        rasterizer.frontFace               = vk::FrontFace::eClockwise;
        rasterizer.depthBiasEnable         = VK_FALSE;
        rasterizer.lineWidth               = 1.0f;

        // Multisampling
        vk::PipelineMultisampleStateCreateInfo multisampling{};
        multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
        multisampling.sampleShadingEnable  = VK_FALSE;

        // Color blending
        vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.blendEnable = vk::False;

        colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                              vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;

        vk::PipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.logicOpEnable = vk::False;
        colorBlending.logicOp       = vk::LogicOp::eCopy;
        colorBlending.setAttachments(colorBlendAttachment);

        vk::PushConstantRange range{};
        range.stageFlags = vk::ShaderStageFlagBits::eFragment;
        range.offset     = 0;
        range.size       = sizeof(uniform_block);

        // Pipeline layout
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.setSetLayouts(descriptorSetLayout_);
        pipelineLayoutInfo.setPushConstantRanges(range);

        pipelineLayout_ = device_.createPipelineLayout(pipelineLayoutInfo);

        vk::PipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.setDynamicStates(dynamicStates);

        // Graphics pipeline
        vk::GraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.pVertexInputState   = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState      = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pDynamicState       = &dynamicState;
        pipelineInfo.pMultisampleState   = &multisampling;
        pipelineInfo.pColorBlendState    = &colorBlending;
        pipelineInfo.layout              = pipelineLayout_;
        pipelineInfo.renderPass          = nullptr;
        pipelineInfo.subpass             = 0;

        auto shaderStages = std::move(create_shader_program(device_));
        pipelineInfo.setStages(shaderStages);

        vk::PipelineRenderingCreateInfo rendering_info{};
        rendering_info.setColorAttachmentFormats({format});

        pipelineInfo.pNext = &rendering_info;

        pipeline_ = device_.createGraphicsPipeline(nullptr, pipelineInfo).value;

        // Cleanup shader modules after pipeline creation
        for (auto& shaderStage : shaderStages) {
            device_.destroyShaderModule(shaderStage.module);
        }
    }

    void write_descriptor_set(vk::DescriptorSet descriptorSet, const std::array<vk::ImageView, 7>& textures)
    {
        // C++ textures array layout:
        //   [0] = background attachment, [1..4] = planes, [5] = local_key, [6] = layer_key

        // Shader bindless textures[N] layout:
        //   [0..3] = planes, [4] = local_key, [5] = layer_key

        // Bind planes, local_key, and layer_key to the bindless texture array
        std::array<vk::DescriptorImageInfo, 6> textureInfos;
        for (int i = 0; i < 6; ++i) {
            textureInfos[i].sampler   = textureSampler_;
            textureInfos[i].imageView = textures[i + 1];
#ifdef __APPLE__
            // moltenvk doesn't support nullDescriptors, so bind a dummy texture if the plane is not present
            textureInfos[i].imageView = textures[i + 1] ? textures[i + 1] : dummyView_;
#endif
            textureInfos[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        }

        // Override samplers for local_key and layer_key to use nearest filtering
        textureInfos[4].sampler = keySampler_;
        textureInfos[5].sampler = keySampler_;

        vk::WriteDescriptorSet texturesWrite{};
        texturesWrite.dstSet          = descriptorSet;
        texturesWrite.dstBinding      = 0;
        texturesWrite.dstArrayElement = 0;
        texturesWrite.descriptorType  = vk::DescriptorType::eCombinedImageSampler;
        texturesWrite.setImageInfo(textureInfos);
        texturesWrite.descriptorCount = 6;

        // Bind background attachment as input attachment
        vk::DescriptorImageInfo backgroundInfo{};
        backgroundInfo.imageLayout = vk::ImageLayout::eRenderingLocalRead;
        backgroundInfo.imageView   = textures[0];

        vk::WriteDescriptorSet backgroundWrite{};
        backgroundWrite.dstSet          = descriptorSet;
        backgroundWrite.dstBinding      = 1;
        backgroundWrite.dstArrayElement = 0;
        backgroundWrite.descriptorType  = vk::DescriptorType::eInputAttachment;
        backgroundWrite.setImageInfo(backgroundInfo);

        vk::WriteDescriptorSet descriptorWrites[]{backgroundWrite, texturesWrite};
        device_.updateDescriptorSets(descriptorWrites, nullptr);
    }

    void draw(vk::CommandBuffer                   commandBuffer,
              vk::DescriptorSet                   descriptorSet,
              vk::Buffer                          vertexBuffer,
              uint32_t                            coords_count,
              uint32_t                            vertex_buffer_offset,
              const uniform_block&                params,
              const std::array<vk::ImageView, 7>& textures)
    {
        write_descriptor_set(descriptorSet, textures);
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline_);
        commandBuffer.bindVertexBuffers(0, vertexBuffer, {vertex_buffer_offset});
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout_, 0, descriptorSet, nullptr);
        commandBuffer.pushConstants(
            pipelineLayout_, vk::ShaderStageFlagBits::eFragment, 0, sizeof(uniform_block), &params);
        commandBuffer.draw(coords_count, 1, 0, 0);
    }

    ~impl()
    {
        device_.destroyDescriptorSetLayout(descriptorSetLayout_);
        device_.destroySampler(textureSampler_);
        device_.destroySampler(keySampler_);

        device_.destroyPipeline(pipeline_);
        device_.destroyPipelineLayout(pipelineLayout_);
    }
};

pipeline::pipeline(vk::Device device, vk::Format format, vk::ImageView dummy_view)
    : impl_(new impl(device, format, dummy_view))
{
}
pipeline::~pipeline() {}

void pipeline::draw(vk::CommandBuffer                   commandBuffer,
                    vk::DescriptorSet                   descriptorSet,
                    vk::Buffer                          vertexBuffer,
                    uint32_t                            coords_count,
                    uint32_t                            vertex_buffer_offset,
                    const uniform_block&                params,
                    const std::array<vk::ImageView, 7>& textures)
{
    impl_->draw(commandBuffer, descriptorSet, vertexBuffer, coords_count, vertex_buffer_offset, params, textures);
}

vk::Pipeline pipeline::id() const { return impl_->pipeline_; }

vk::DescriptorSetLayout pipeline::descriptor_set_layout() const { return impl_->descriptorSetLayout_; }

std::vector<vk::DescriptorPoolSize> pipeline::descriptor_pool_sizes() const { return impl_->descriptor_pool_sizes(); }

}}} // namespace caspar::accelerator::vulkan
