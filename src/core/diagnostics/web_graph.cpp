/*
 * Copyright (c) 2026 Sveriges Television AB <info@casparcg.com>
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

#include "../StdAfx.h"

#include "web_graph.h"

#include <common/diagnostics/graph.h>
#include <common/memory.h>
#include <common/utf.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace caspar { namespace core { namespace diagnostics { namespace web {

namespace {

std::string json_escape(const std::string& s)
{
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

// Mirrors every value/text/color/tag set on its owning caspar::diagnostics::graph into plain members
// guarded by a mutex, so a JSON snapshot can be built at any time from any thread (the HTTP server's).
class web_graph_sink : public caspar::diagnostics::spi::graph_sink
{
    mutable std::mutex                      mutex_;
    std::string                             text_;
    std::unordered_map<std::string, double> values_;
    std::unordered_map<std::string, int>    colors_;
    std::vector<std::string>                tags_;

  public:
    void activate() override {}

    void set_text(const std::wstring& value) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        text_ = u8(value);
    }

    void set_value(const std::string& name, double value) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        values_[name] = value;
    }

    void set_color(const std::string& name, int color) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        colors_[name] = color;
    }

    void set_tag(caspar::diagnostics::tag_severity /*severity*/, const std::string& name) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (std::find(tags_.begin(), tags_.end(), name) == tags_.end())
            tags_.push_back(name);
    }

    void auto_reset() override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        tags_.clear();
    }

    std::string to_json() const
    {
        std::lock_guard<std::mutex> lock(mutex_);

        std::ostringstream os;
        os << "{\"text\":\"" << json_escape(text_) << "\",\"values\":{";
        bool first = true;
        for (auto& p : values_) {
            if (!first)
                os << ",";
            first = false;
            os << "\"" << json_escape(p.first) << "\":" << p.second;
        }
        os << "},\"colors\":{";
        first = true;
        for (auto& p : colors_) {
            if (!first)
                os << ",";
            first = false;
            os << "\"" << json_escape(p.first) << "\":" << p.second;
        }
        os << "},\"tags\":[";
        first = true;
        for (auto& t : tags_) {
            if (!first)
                os << ",";
            first = false;
            os << "\"" << json_escape(t) << "\"";
        }
        os << "]}";
        return os.str();
    }
};

std::mutex                                 g_registry_mutex;
std::vector<std::weak_ptr<web_graph_sink>> g_registry;

} // namespace

void register_sink()
{
    caspar::diagnostics::spi::register_sink_factory([]() -> spl::shared_ptr<caspar::diagnostics::spi::graph_sink> {
        auto sink = std::make_shared<web_graph_sink>();

        {
            std::lock_guard<std::mutex> lock(g_registry_mutex);
            g_registry.push_back(sink);
        }

        return spl::make_shared_ptr(std::static_pointer_cast<caspar::diagnostics::spi::graph_sink>(sink));
    });
}

std::string snapshot_json()
{
    std::vector<std::shared_ptr<web_graph_sink>> live;
    {
        std::lock_guard<std::mutex> lock(g_registry_mutex);
        g_registry.erase(std::remove_if(g_registry.begin(),
                                        g_registry.end(),
                                        [&](const std::weak_ptr<web_graph_sink>& w) {
                                            auto s = w.lock();
                                            if (s)
                                                live.push_back(s);
                                            return !s;
                                        }),
                         g_registry.end());
    }

    std::ostringstream os;
    os << "{\"t\":" << std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count()
       << ",\"graphs\":[";
    bool first = true;
    for (auto& s : live) {
        if (!first)
            os << ",";
        first = false;
        os << s->to_json();
    }
    os << "]}";
    return os.str();
}

}}}} // namespace caspar::core::diagnostics::web
