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

#include <memory>
#include <stack>
#include <vector>

namespace caspar::core {

struct side_data_item
{
    side_data_transform transform;
    const_frame         frame; // avoid copying side_data
};

struct side_data_dedup final
{
    std::set<const_frame_side_data> last;
    std::set<const_frame_side_data> cur;

    void next_frame()
    {
        last.clear();
        last.swap(cur);
    }
    bool is_duplicate_and_add(const const_frame_side_data& side_data)
    {
        if (!cur.insert(side_data).second)
            return true;
        return last.count(side_data);
    }
};

struct side_data_mixer::impl final
{
    std::stack<side_data_transform>     transform_stack_;
    std::vector<side_data_item>         items_;
    side_data_dedup                     side_data_dedup_;
    spl::shared_ptr<diagnostics::graph> graph_;
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

std::vector<const_frame_side_data> side_data_mixer::mixed()
{
    std::vector<const_frame_side_data> mixed;

    // TODO: implement proper handling for mixing/switching between different a53 cc sources

    // allow multiple a53_cc side_data from one source, but not multiple sources
    bool has_a53_cc_source = false;
    for (auto item : impl_->items_) {
        bool has_a53_cc_side_data = false;
        for (auto side_data : item.frame.side_data()) {
            if (!frame_side_data_include_on_duplicate_frames(side_data.type())) {
                if (impl_->side_data_dedup_.is_duplicate_and_add(side_data))
                    continue;
            }
            switch (side_data.type()) {
                case frame_side_data_type::a53_cc:
                    if (item.transform.use_closed_captions) {
                        has_a53_cc_side_data = true;
                        if (has_a53_cc_source) {
                            impl_->graph_->set_tag(diagnostics::tag_severity::WARNING,
                                                   "multiple-simultaneous-a53-cc-sources");
                        } else {
                            mixed.push_back(std::move(side_data));
                        }
                    }
                    break;
            }
        }
        has_a53_cc_source |= has_a53_cc_side_data;
    }
    impl_->items_.clear();
    impl_->side_data_dedup_.next_frame();
    return mixed;
}

void side_data_mixer::push(const frame_transform& transform)
{
    impl_->transform_stack_.push(impl_->transform_stack_.top() * transform.side_data_transform);
}

void side_data_mixer::visit(const const_frame& frame)
{
    if (!impl_->transform_stack_.top().use_closed_captions || frame.side_data().empty())
        return;

    impl_->items_.push_back(std::move(side_data_item{impl_->transform_stack_.top(), frame}));
}

void side_data_mixer::pop() { impl_->transform_stack_.pop(); }
} // namespace caspar::core
