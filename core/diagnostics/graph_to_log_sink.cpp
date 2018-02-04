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
 * Author: Helge Norberg, helge.norberg@svt.se
 */

#include "graph_to_log_sink.h"

#include "call_context.h"

#include <common/diagnostics/graph.h>
#include <common/log.h>
#include <common/memory.h>

#include <mutex>

namespace caspar { namespace core { namespace diagnostics {

class graph_to_log_sink : public caspar::diagnostics::spi::graph_sink
{
    std::mutex   mutex_;
    std::wstring text_;
    std::wstring context_ = call_context::for_thread().to_string();

  public:
    void activate() override {}

    void set_text(const std::wstring& value) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        text_ = value;
    }

    void set_value(const std::string& name, double value) override {}

    void set_color(const std::string& name, int color) override {}

    void set_tag(caspar::diagnostics::tag_severity severity, const std::string& name) override
    {
        std::lock_guard<std::mutex> lock(mutex_);

        switch (severity) {
        case caspar::diagnostics::tag_severity::INFO:
            CASPAR_LOG(trace) << L"[diagnostics] [" << text_ << L"] " << name << L" " << context_;
            break;
        case caspar::diagnostics::tag_severity::WARNING:
            CASPAR_LOG(debug) << L"[diagnostics] [" << text_ << L"] " << name << L" " << context_;
            break;
        }
    }

    void auto_reset() override {}
};

void register_graph_to_log_sink()
{
    caspar::diagnostics::spi::register_sink_factory([] { return spl::make_shared<graph_to_log_sink>(); });
}

}}} // namespace caspar::core::diagnostics
