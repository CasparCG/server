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

#pragma once

#include "../fwd.h"
#include "../monitor/monitor.h"

#include <common/forward.h>
#include <common/memory.h>

#include <future>

FORWARD2(caspar, diagnostics, class graph);

namespace caspar { namespace core {

class output final
{
    output(const output&);
    output& operator=(const output&);

  public:
    explicit output(spl::shared_ptr<caspar::diagnostics::graph> graph,
                    const video_format_desc&                    format_desc,
                    int                                         channel_index);

    // Returns when submitted to consumers, but the future indicates when the consumers are ready for a new frame.
    std::future<void> operator()(const_frame frame, const video_format_desc& format_desc);

    void add(const spl::shared_ptr<frame_consumer>& consumer);
    void add(int index, const spl::shared_ptr<frame_consumer>& consumer);
    void remove(const spl::shared_ptr<frame_consumer>& consumer);
    void remove(int index);

    monitor::subject& monitor_output();

    std::vector<spl::shared_ptr<const frame_consumer>> get_consumers() const;

  private:
    struct impl;
    spl::shared_ptr<impl> impl_;
};

}} // namespace caspar::core
