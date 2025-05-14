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
#include <stack>
#include <vector>

#include <boost/log/utility/manipulators/dump.hpp>

namespace caspar::core {

struct side_data_item
{
    side_data_transform      transform;
    frame_side_data_in_queue side_data;
};

struct side_data_mixer::impl final
{
    std::stack<side_data_transform>        transform_stack_;
    std::vector<side_data_item>            items_;
    spl::shared_ptr<diagnostics::graph>    graph_;
    std::shared_ptr<frame_side_data_queue> output_queue_ = std::make_shared<frame_side_data_queue>();

    std::map<std::weak_ptr<frame_side_data_queue>, frame_side_data_queue::position, std::owner_less<>> last_queue_pos_;

    explicit impl(spl::shared_ptr<diagnostics::graph> graph)
        : graph_(std::move(graph))
    {
        transform_stack_.push(side_data_transform());
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

    // TODO: implement proper handling for mixing/switching between different a53 cc sources

    // allow multiple a53_cc side_data from one source, but not multiple sources
    bool has_a53_cc_source = false;

    std::optional<mutable_frame_side_data> a53_cc_side_data;
    for (auto item : impl_->items_) {
        auto process_side_data = [&](const const_frame_side_data& side_data, bool is_old) {
            switch (side_data.type()) {
                case frame_side_data_type::a53_cc:
                    CASPAR_LOG(trace) << L"side_data_mixer: got A53_CC side data: "
                                      << boost::log::dump(side_data.data().data(), side_data.data().size(), 16);
                    if (item.transform.use_closed_captions) {
                        if (has_a53_cc_source) {
                            impl_->graph_->set_tag(diagnostics::tag_severity::WARNING,
                                                   "multiple-simultaneous-a53-cc-sources");
                        } else if (a53_cc_side_data.has_value()) {
                            // combine the side data, a53_cc side data can just be concatenated,
                            // it will later be passed through ccrepack
                            a53_cc_side_data->data().insert(
                                a53_cc_side_data->data().end(), side_data.data().begin(), side_data.data().end());
                        } else {
                            a53_cc_side_data.emplace(side_data);
                        }
                    }
                    break;
            }
        };
        auto [iter, inserted] = impl_->last_queue_pos_.emplace(item.side_data.queue, item.side_data.pos);
        auto [start, _]       = item.side_data.queue->valid_position_range();

        if (!inserted) {
            CASPAR_LOG(trace) << L"side_data_mixer: new queue " << (void*)item.side_data.queue.get();
            start        = std::max(start, iter->second + 1);
            iter->second = item.side_data.pos;
        }

        CASPAR_LOG(trace) << L"side_data_mixer: processing from " << start << L" to " << item.side_data.pos;

        for (auto pos = start; pos <= item.side_data.pos; pos++) {
            if (auto side_data_for_frame = item.side_data.queue->get(pos)) {
                if (!side_data_for_frame->empty()) {
                    CASPAR_LOG(trace) << L"side_data_mixer: side data for frame at " << pos;
                }
                for (auto& side_data : *side_data_for_frame) {
                    process_side_data(side_data, pos != item.side_data.pos);
                }
            }
        }
        has_a53_cc_source |= a53_cc_side_data.has_value();
    }
    impl_->items_.clear();

    // remove expired queues so we can reclaim memory
    for (auto iter = impl_->last_queue_pos_.begin(); iter != impl_->last_queue_pos_.end();) {
        if (iter->first.expired()) {
            iter = impl_->last_queue_pos_.erase(iter);
        } else {
            ++iter;
        }
    }

    if (a53_cc_side_data.has_value()) {
        mixed.emplace_back(*std::move(a53_cc_side_data));
    }

    auto pos = impl_->output_queue_->add_frame(std::move(mixed));

    return {pos, impl_->output_queue_};
}

void side_data_mixer::push(const frame_transform& transform)
{
    impl_->transform_stack_.push(impl_->transform_stack_.top() * transform.side_data_transform);
}

void side_data_mixer::visit(const const_frame& frame)
{
    if (!impl_->transform_stack_.top().use_closed_captions || !frame.side_data().queue)
        return;

    impl_->items_.push_back(std::move(side_data_item{impl_->transform_stack_.top(), frame.side_data()}));
}

void side_data_mixer::pop() { impl_->transform_stack_.pop(); }
} // namespace caspar::core
