/*
 * Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include <boost/variant.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace caspar { namespace core { namespace monitor {

typedef boost::variant<bool, std::int32_t, std::int64_t, float, double, std::string> data_t;

typedef boost::small_vector<data_t, 4> data_vector_t;

// TODO (perf) Optimize
class state
{
    typedef boost::flat_map<std::string, data_vector_t> data_map_t;

    mutable std::mutex mutex_;
    data_map_t         data_;

  public:
    template <typename F>
    void update(const F& func)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        func(data_);
    }

    template <typename F>
    void set(const F& func)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        data_.clear();
        func(data_);
    }

    auto get() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_;
    }
};

void assign(state& dst, const std::string& name, const state& src)
{
    auto data = src.get();
    dst.update([&](auto& state) {
        for (auto& p : data) {
            state[name + "/" + p.first] = std::move(p.second);
        }
    });
}

void assign(state& dst, const state& src)
{
    auto data = src.get();
    dst.update([&](auto& state) {
        for (auto& p : data) {
            data_.insert(std::move(p));
        }
    });
}

}}} // namespace caspar::core::monitor
