/*
 * Copyright (c) 2025 Jacob Lifshay <programmerjake@gmail.com>
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
 */

#include "side_data_mixer.h"
#include "core/frame/frame_side_data.h"
#include <core/frame/frame.h>
#include <core/frame/frame_transform.h>

#include <map>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

#include <boost/log/utility/manipulators/dump.hpp>

namespace caspar::core {

struct side_data_transform_tree_node final
{
    side_data_transform                                                                transform;
    std::vector<std::variant<side_data_transform_tree_node, frame_side_data_in_queue>> children;
    explicit side_data_transform_tree_node(side_data_transform transform)
        : transform(std::move(transform))
    {
    }
};

struct side_data_mixer::impl final
{
    std::vector<side_data_transform_tree_node> transform_stack_;
    std::shared_ptr<frame_side_data_queue>     last_a53_cc_source_queue_;
    spl::shared_ptr<diagnostics::graph>        graph_;
    std::shared_ptr<frame_side_data_queue>     output_queue_ = std::make_shared<frame_side_data_queue>();

    std::map<std::weak_ptr<frame_side_data_queue>, frame_side_data_queue::position, std::owner_less<>> last_queue_pos_;

    void start_next_frame()
    {
        transform_stack_ = {side_data_transform_tree_node(side_data_transform())};

        // remove expired queues so we can reclaim memory
        for (auto iter = last_queue_pos_.begin(); iter != last_queue_pos_.end();) {
            if (iter->first.expired()) {
                iter = last_queue_pos_.erase(iter);
            } else {
                ++iter;
            }
        }
    }

    explicit impl(spl::shared_ptr<diagnostics::graph> graph)
        : graph_(std::move(graph))
    {
        start_next_frame();
    }

    struct node_mix_result final
    {
        std::shared_ptr<frame_side_data_queue> cc_source_queue;
        std::optional<mutable_frame_side_data> a53_cc_side_data;
    };

    void mix_side_data(const const_frame_side_data&            side_data,
                       bool                                    is_old,
                       std::optional<mutable_frame_side_data>& a53_cc_side_data,
                       std::vector<const_frame_side_data>&     mixed)
    {
        switch (side_data.type()) {
            case frame_side_data_type::a53_cc:
                CASPAR_LOG(trace) << L"side_data_mixer: got A53_CC side data: "
                                  << boost::log::dump(side_data.data().data(), side_data.data().size(), 16);
                if (a53_cc_side_data.has_value()) {
                    // combine the side data, a53_cc side data can just be concatenated,
                    // it will later be passed through ccrepack
                    a53_cc_side_data->data().insert(
                        a53_cc_side_data->data().end(), side_data.data().begin(), side_data.data().end());
                } else {
                    a53_cc_side_data.emplace(side_data);
                }
                break;
        }
    }

    [[nodiscard]] node_mix_result mix_side_data_in_queue(frame_side_data_in_queue            side_data_in_queue,
                                                         std::vector<const_frame_side_data>& mixed)
    {
        if (!side_data_in_queue.queue)
            return {nullptr, std::nullopt};

        auto [iter, inserted] = last_queue_pos_.emplace(side_data_in_queue.queue, side_data_in_queue.pos);
        auto [start, end]     = side_data_in_queue.position_range_since_last(iter->second, !inserted);

        if (inserted) {
            CASPAR_LOG(trace) << L"side_data_mixer: new queue " << (void*)side_data_in_queue.queue.get();
        }

        CASPAR_LOG(trace) << L"side_data_mixer: processing from " << start << L" to " << end - 1;

        std::optional<mutable_frame_side_data> a53_cc_side_data;

        for (auto pos = start; pos < end; pos++) {
            if (auto side_data_for_frame = side_data_in_queue.queue->get(pos)) {
                if (!side_data_for_frame->empty()) {
                    CASPAR_LOG(trace) << L"side_data_mixer: side data for frame at " << pos;
                }
                for (auto& side_data : *side_data_for_frame) {
                    mix_side_data(side_data, pos != side_data_in_queue.pos, a53_cc_side_data, mixed);
                }
            }
        }

        return {side_data_in_queue.queue, std::move(a53_cc_side_data)};
    }

    [[nodiscard]] node_mix_result mix_node(const side_data_transform_tree_node& node,
                                           std::vector<const_frame_side_data>&  mixed)
    {
        enum class cc_state_t
        {
            none_yet,
            non_node_child,
            node_child,
        };
        cc_state_t                             cc_state = cc_state_t::none_yet;
        closed_captions_priority               cc_priority;
        std::shared_ptr<frame_side_data_queue> cc_source_queue;
        std::optional<mutable_frame_side_data> a53_cc_side_data;

        for (auto& child : node.children) {
            if (auto child_node = std::get_if<side_data_transform_tree_node>(&child)) {
                auto result = mix_node(*child_node, mixed);
                switch (cc_state) {
                    case cc_state_t::none_yet:
                        cc_state         = cc_state_t::node_child;
                        cc_priority      = child_node->transform.closed_captions_priority_;
                        cc_source_queue  = std::move(result.cc_source_queue);
                        a53_cc_side_data = std::move(result.a53_cc_side_data);
                        break;
                    case cc_state_t::non_node_child:
                        // the first non-node child always provides closed captions
                        break;
                    case cc_state_t::node_child:
                        if (cc_priority < child_node->transform.closed_captions_priority_) {
                            cc_priority      = child_node->transform.closed_captions_priority_;
                            cc_source_queue  = std::move(result.cc_source_queue);
                            a53_cc_side_data = std::move(result.a53_cc_side_data);
                        }
                        break;
                }
            } else {
                auto& child_side_data = std::get<frame_side_data_in_queue>(child);
                auto  result          = mix_side_data_in_queue(child_side_data, mixed);
                switch (cc_state) {
                    case cc_state_t::none_yet:
                    case cc_state_t::node_child:
                        cc_state         = cc_state_t::non_node_child;
                        cc_source_queue  = std::move(result.cc_source_queue);
                        a53_cc_side_data = std::move(result.a53_cc_side_data);
                        break;
                    case cc_state_t::non_node_child:
                        // the first non-node child always provides closed captions
                        break;
                }
            }
        }
        return {std::move(cc_source_queue), std::move(a53_cc_side_data)};
    }
};

side_data_mixer::side_data_mixer(spl::shared_ptr<diagnostics::graph> graph)
    : impl_(std::make_unique<impl>(std::move(graph)))
{
}

side_data_mixer::~side_data_mixer() = default;

frame_side_data_in_queue side_data_mixer::mixed()
{
    std::vector<const_frame_side_data> mixed;

    auto mix_result = impl_->mix_node(impl_->transform_stack_[0], mixed);

    if (mix_result.a53_cc_side_data) {
        if (impl_->last_a53_cc_source_queue_ && impl_->last_a53_cc_source_queue_ != mix_result.cc_source_queue) {
            // TODO: implement proper handling for switching between different a53 cc sources
            CASPAR_LOG(warning) << L"side_data_mixer: A/53 closed captions source changed,\n"
                                   L"properly switching between different sources isn't implemented yet,\n"
                                   L"you may end up with a truncated closed captions stream.\n"
                                   L"source queue changed: "
                                << (void*)impl_->last_a53_cc_source_queue_.get() << L" -> "
                                << (void*)mix_result.cc_source_queue.get();
        }
        impl_->last_a53_cc_source_queue_ = std::move(mix_result.cc_source_queue);
        mixed.emplace_back(*std::move(mix_result.a53_cc_side_data));
    }

    auto pos = impl_->output_queue_->add_frame(std::move(mixed));

    impl_->start_next_frame();

    return {pos, impl_->output_queue_};
}

void side_data_mixer::push(const frame_transform& transform)
{
    impl_->transform_stack_.emplace_back(transform.side_data_transform);
}

void side_data_mixer::visit(const const_frame& frame)
{
    impl_->transform_stack_.back().children.push_back(frame.side_data());
}

void side_data_mixer::pop()
{
    impl_->transform_stack_.at(impl_->transform_stack_.size() - 2)
        .children.push_back(std::move(impl_->transform_stack_.back()));
    impl_->transform_stack_.pop_back();
}
} // namespace caspar::core
