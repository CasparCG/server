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
#include "graph.h"

#include <mutex>
#include <vector>

namespace caspar { namespace diagnostics {

int color(float r, float g, float b, float a)
{
    int code = 0;
    code |= static_cast<int>(r * 255.0f + 0.5f) << 24;
    code |= static_cast<int>(g * 255.0f + 0.5f) << 16;
    code |= static_cast<int>(b * 255.0f + 0.5f) << 8;
    code |= static_cast<int>(a * 255.0f + 0.5f) << 0;
    return code;
}

std::tuple<float, float, float, float> color(int code)
{
    float r = static_cast<float>(code >> 24 & 255) / 255.0f;
    float g = static_cast<float>(code >> 16 & 255) / 255.0f;
    float b = static_cast<float>(code >> 8 & 255) / 255.0f;
    float a = static_cast<float>(code >> 0 & 255) / 255.0f;
    return std::make_tuple(r, g, b, a);
}

using sink_factories_t = std::vector<spi::sink_factory_t>;
static std::mutex       g_sink_factories_mutex;
static sink_factories_t g_sink_factories;

std::vector<spl::shared_ptr<spi::graph_sink>> create_sinks()
{
    std::lock_guard<std::mutex> lock(g_sink_factories_mutex);

    std::vector<spl::shared_ptr<spi::graph_sink>> result;
    for (auto sink : g_sink_factories) {
        result.push_back(sink());
    }
    return result;
}

struct graph::impl
{
    std::vector<spl::shared_ptr<spi::graph_sink>> sinks_ = create_sinks();

  public:
    impl() {}

    void activate()
    {
        for (auto& sink : sinks_)
            sink->activate();
    }

    void set_text(const std::wstring& value)
    {
        for (auto& sink : sinks_)
            sink->set_text(value);
    }

    void set_value(const std::string& name, double value)
    {
        for (auto& sink : sinks_)
            sink->set_value(name, value);
    }

    void set_tag(tag_severity severity, const std::string& name)
    {
        for (auto& sink : sinks_)
            sink->set_tag(severity, name);
    }

    void set_color(const std::string& name, int color)
    {
        for (auto& sink : sinks_)
            sink->set_color(name, color);
    }

    void auto_reset()
    {
        for (auto& sink : sinks_)
            sink->auto_reset();
    }

  private:
    impl(impl&);
    impl& operator=(impl&);
};

graph::graph()
    : impl_(new impl)
{
}

void graph::set_text(const std::wstring& value) { impl_->set_text(value); }
void graph::set_value(const std::string& name, double value) { impl_->set_value(name, value); }
void graph::set_color(const std::string& name, int color) { impl_->set_color(name, color); }
void graph::set_tag(tag_severity severity, const std::string& name) { impl_->set_tag(severity, name); }
void graph::auto_reset() { impl_->auto_reset(); }

void register_graph(const spl::shared_ptr<graph>& graph) { graph->impl_->activate(); }

namespace spi {

void register_sink_factory(sink_factory_t factory)
{
    std::lock_guard<std::mutex> lock(g_sink_factories_mutex);

    g_sink_factories.push_back(std::move(factory));
}

} // namespace spi

}} // namespace caspar::diagnostics
