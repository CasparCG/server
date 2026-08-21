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

#include "command_context.h"

#include "vulkan_queue.h"

namespace caspar { namespace accelerator { namespace vulkan {

command_context::command_context(vk::Device device, std::shared_ptr<vulkan_queue> queue)
    : device_(device)
    , queue_(std::move(queue))
{
    vk::CommandPoolCreateInfo pool_info;
    pool_info.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    pool_info.queueFamilyIndex = queue_->family_index();
    pool_                      = device_.createCommandPool(pool_info);

    vk::SemaphoreTypeCreateInfo timeline_info{};
    timeline_info.semaphoreType = vk::SemaphoreType::eTimeline;
    timeline_info.initialValue  = 0;
    vk::SemaphoreCreateInfo semaphore_info{};
    semaphore_info.pNext = &timeline_info;
    timeline_            = device_.createSemaphore(semaphore_info);
}

command_context::~command_context()
{
    inflight_.clear();
    device_.destroySemaphore(timeline_);
    device_.destroyCommandPool(pool_);
}

vk::CommandBuffer command_context::acquire_command_buffer()
{
    if (inflight_.size() > 1) {
        auto completed = device_.getSemaphoreCounterValue(timeline_);

        // try to reuse the oldest existing command buffer
        if (inflight_.front().value <= completed) {
            auto cmd = inflight_.front().cmd;
            cmd.reset();
            inflight_.pop_front();
            return cmd;
        }
    }

    vk::CommandBufferAllocateInfo alloc_info{};
    alloc_info.commandPool        = pool_;
    alloc_info.level              = vk::CommandBufferLevel::ePrimary;
    alloc_info.commandBufferCount = 1;
    return device_.allocateCommandBuffers(alloc_info)[0];
}

completion_token command_context::record_and_submit(const std::function<void(vk::CommandBuffer)>& record)
{
    return record_and_submit(record, {});
}

completion_token command_context::record_and_submit(const std::function<void(vk::CommandBuffer)>& record,
                                                    vk::ArrayProxy<const completion_token>        wait_tokens)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Cross-queue waits: keep only tokens on a foreign timeline. A token on our own
    // timeline means the producer ran on this same queue (queue_distance::same_queue),
    // where submission order plus the producer's barriers already order it before
    // these commands, so waiting it would be redundant — and self-waiting the value
    // we are about to signal could deadlock.
    std::vector<vk::Semaphore>          wait_semaphores;
    std::vector<uint64_t>               wait_values;
    std::vector<vk::PipelineStageFlags> wait_stages;
    for (const auto& token : wait_tokens) {
        if (!token || token.timeline == timeline_)
            continue;
        wait_semaphores.push_back(token.timeline);
        wait_values.push_back(token.value);
        // vk::SubmitInfo carries v1 stage flags (not the v2 masks the barriers use);
        // eAllCommands is the conservative, translation-safe wait scope.
        wait_stages.push_back(vk::PipelineStageFlagBits::eAllCommands);
    }

    auto cmd = acquire_command_buffer();

    cmd.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    record(cmd);
    cmd.end();

    auto signal_value = ++value_;

    vk::TimelineSemaphoreSubmitInfo timeline_submit{};
    timeline_submit.setWaitSemaphoreValues(wait_values);
    timeline_submit.setSignalSemaphoreValues(signal_value);

    vk::SubmitInfo submit_info{};
    submit_info.setCommandBuffers(cmd);
    submit_info.setWaitSemaphores(wait_semaphores);
    submit_info.setWaitDstStageMask(wait_stages);
    submit_info.setSignalSemaphores(timeline_);
    submit_info.pNext = &timeline_submit;
    queue_->submit(submit_info);

    inflight_.push_back({cmd, signal_value});

    return completion_token{timeline_, signal_value};
}

bool command_context::wait(const completion_token& token, uint64_t timeout_ns) const
{
    if (!token)
        return true;

    vk::SemaphoreWaitInfo wait_info{};
    wait_info.setSemaphores(token.timeline);
    wait_info.setValues(token.value);
    return device_.waitSemaphores(wait_info, timeout_ns) == vk::Result::eSuccess;
}

}}} // namespace caspar::accelerator::vulkan
