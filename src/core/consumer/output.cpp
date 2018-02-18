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
 * Author: Robert Nagy, ronag89@gmail.com
 */
#include "output.h"

#include "frame_consumer.h"

#include "../frame/frame.h"
#include "../video_format.h"

#include <common/assert.h>
#include <common/diagnostics/graph.h>
#include <common/env.h>
#include <common/executor.h>
#include <common/future.h>
#include <common/memshfl.h>
#include <common/prec_timer.h>
#include <common/timer.h>

#include <boost/lexical_cast.hpp>

#include <functional>

namespace caspar { namespace core {

struct output::impl
{
    monitor::state                                 state_;
    spl::shared_ptr<diagnostics::graph>            graph_;
    const int                                      channel_index_;
    video_format_desc                              format_desc_;
    std::map<int, spl::shared_ptr<frame_consumer>> consumers_;
    prec_timer                                     sync_timer_;
    executor executor_{L"output " + boost::lexical_cast<std::wstring>(channel_index_)};

  public:
    impl(spl::shared_ptr<diagnostics::graph> graph, const video_format_desc& format_desc, int channel_index)
        : graph_(std::move(graph))
        , channel_index_(channel_index)
        , format_desc_(format_desc)
    {
    }

    void add(int index, spl::shared_ptr<frame_consumer> consumer)
    {
        remove(index);

        consumer->initialize(format_desc_, channel_index_);

        executor_.begin_invoke([=] { consumers_.emplace(index, std::move(consumer)); });
    }

    void add(const spl::shared_ptr<frame_consumer>& consumer) { add(consumer->index(), consumer); }

    void remove(int index)
    {
        executor_.begin_invoke([=] {
            auto it = consumers_.find(index);
            if (it != consumers_.end()) {
                consumers_.erase(it);
            }
        });
    }

    void remove(const spl::shared_ptr<frame_consumer>& consumer) { remove(consumer->index()); }

    void change_channel_format(const core::video_format_desc& format_desc)
    {
        for (auto it = consumers_.begin(); it != consumers_.end();) {
            try {
                it->second->initialize(format_desc, it->first);
                ++it;
            } catch (...) {
                CASPAR_LOG_CURRENT_EXCEPTION();
                it = consumers_.erase(it);
            }
        }

        format_desc_ = format_desc;
    }

    std::future<void> operator()(const_frame input_frame, const core::video_format_desc& format_desc)
    {
        return executor_.begin_invoke([=, input_frame = std::move(input_frame)] {
            if (input_frame && input_frame.size() != format_desc_.size) {
                CASPAR_LOG(warning) << print() << L" Invalid input frame dimension.";
                return;
            }

            if (!input_frame) {
                return;
            }

            if (format_desc_ != format_desc) {
                change_channel_format(format_desc);
            }

            std::map<int, std::future<bool>> futures;

            for (auto it = consumers_.begin(); it != consumers_.end();) {
                try {
                    futures.emplace(it->first, it->second->send(input_frame));
                    ++it;
                } catch (...) {
                    CASPAR_LOG_CURRENT_EXCEPTION();
                    it = consumers_.erase(it);
                }
            }

            for (auto& p : futures) {
                try {
                    if (!p.second.get()) {
                        consumers_.erase(p.first);
                    }
                } catch (...) {
                    CASPAR_LOG_CURRENT_EXCEPTION();
                    consumers_.erase(p.first);
                }
            }

            state_.clear();
            for (auto& p : consumers_) {
                state_.insert_or_assign("port/" + boost::lexical_cast<std::string>(p.first), p.second->state());
            }

            const auto needs_sync = std::all_of(consumers_.begin(), consumers_.end(), [](auto& p) {
                return !p.second->has_synchronization_clock();
            });

            if (needs_sync) {
                sync_timer_.tick(1.0 / format_desc_.fps);
            }
        });
    }

    std::wstring print() const { return L"output[" + boost::lexical_cast<std::wstring>(channel_index_) + L"]"; }
};

output::output(spl::shared_ptr<diagnostics::graph> graph, const video_format_desc& format_desc, int channel_index)
    : impl_(new impl(std::move(graph), format_desc, channel_index))
{
}
output::~output() {}
void output::add(int index, const spl::shared_ptr<frame_consumer>& consumer) { impl_->add(index, consumer); }
void output::add(const spl::shared_ptr<frame_consumer>& consumer) { impl_->add(consumer); }
void output::remove(int index) { impl_->remove(index); }
void output::remove(const spl::shared_ptr<frame_consumer>& consumer) { impl_->remove(consumer); }
std::future<void> output::operator()(const_frame frame, const video_format_desc& format_desc)
{
    return (*impl_)(std::move(frame), format_desc);
}
const monitor::state& output::state() const { return impl_->state_; }
}} // namespace caspar::core
