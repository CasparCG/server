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

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace caspar { namespace core { namespace monitor {

typedef boost::
    variant<bool, std::int32_t, std::int64_t, float, double, std::string, std::wstring, std::vector<std::int8_t>>
        data_t;

class state_proxy
{
    std::string                                 key_;
    std::map<std::string, std::vector<data_t>>& data_;

  public:
    state_proxy(const std::string& key, std::map<std::string, std::vector<data_t>>& data)
        : key_(key)
        , data_(data)
    {
    }

    state_proxy& operator=(data_t data)
    {
        data_[key_] = {std::move(data)};
        return *this;
    }

    state_proxy& operator=(std::initializer_list<data_t> data)
    {
        data_[key_] = std::move(data);
        return *this;
    }
};

// TODO (perf) Optimize
class state
{
    typedef std::map<std::string, std::vector<data_t>> data_map_t;

    mutable std::mutex mutex_;
    data_map_t         data_;

  public:
    state_proxy operator[](const std::string& key) { return state_proxy(key, data_); }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        data_.clear();
    }

    void append(const std::string& name, const state& other)
    {
        std::lock_guard<std::mutex> lock1(mutex_);
        std::lock_guard<std::mutex> lock2(other.mutex_);

        auto name2 = name.empty() ? name : name + "/";
        for (auto& p : other.data_) {
            data_[name2 + p.first] = p.second;
        }
    }

    void append(const state& other)
    {
        std::lock_guard<std::mutex> lock1(mutex_);
        std::lock_guard<std::mutex> lock2(other.mutex_);

        for (auto& p : other.data_) {
            data_[p.first] = p.second;
        }
    }

    auto get() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_;
    }
};

}}} // namespace caspar::core::monitor
