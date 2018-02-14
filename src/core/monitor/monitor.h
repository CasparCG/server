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
#include <string>
#include <mutex>

#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>

namespace caspar { namespace core { namespace monitor {

typedef boost::
    variant<bool, std::int32_t, std::int64_t, float, double, std::string, std::wstring>
        data_t;

typedef boost::container::flat_map<std::string, boost::container::small_vector<data_t, 2>> data_map_t;

class state_proxy
{
    std::string key_;
    data_map_t& data_;
    std::mutex& mutex_;
public:
    state_proxy(const std::string& key, data_map_t& data, std::mutex& mutex)
        : key_(key)
        , data_(data)
        , mutex_(mutex)
    {
    }

    state_proxy& operator=(data_t data)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        data_[key_] = { std::move(data) };
        return *this;
    }

    state_proxy& operator=(std::initializer_list<data_t> data)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        data_[key_] = std::move(data);
        return *this;
    }
};

// TODO (perf) Optimize
class state
{
    mutable std::mutex mutex_;
    data_map_t         data_;
public:
    state() = default;
    state(const state& other)
        : data_(other.data_)
    {
    }
    state(state&& other)
        : data_(std::move(other.data_))
    {
    }

    state_proxy operator[](const std::string& key)
    {
        return state_proxy(key, data_, mutex_);
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        data_.clear();
    }

    state& operator=(const state& other)
    {
        auto data = other.get();
        std::lock_guard<std::mutex> lock(mutex_);
        data_ = std::move(data);
        return *this;
    }

    template<typename T>
    state& operator=(T&& data)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        data_ = std::forward<T>(data);
        return *this;
    }

    data_map_t get() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_;
    }

    void insert_or_assign(const std::string& name, const state& other)
    {
        auto data = other.get();
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& p : data) {
            data_[name + "/" + p.first] = std::move(p.second);
        }
    }

    void insert_or_assign(const state& other)
    {
        auto data = other.get();
        std::lock_guard<std::mutex> lock(mutex_);
        if (data_.empty()) {
            data_ = std::move(data);
        } else {
            for (auto& p : data) {
                data_[p.first] = std::move(p.second);
            }
        }
    }
};

}}} // namespace caspar::core::monitor
