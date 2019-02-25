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

#include <boost/lexical_cast.hpp>
#include <boost/variant.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>

namespace caspar { namespace core { namespace monitor {

using data_t     = boost::variant<bool, std::int32_t, std::int64_t, float, double, std::string, std::wstring>;
using vector_t   = boost::container::small_vector<data_t, 2>;
using data_map_t = boost::container::flat_map<std::string, vector_t>;

class state
{
    data_map_t data_;

    class state_proxy
    {
        std::string key_;
        data_map_t& data_;

      public:
        state_proxy(const std::string& key, data_map_t& data)
            : key_(key)
            , data_(data)
        {
        }

        state_proxy& operator=(data_t data)
        {
            data_[key_] = {std::move(data)};
            return *this;
        }

        state_proxy& operator=(vector_t data)
        {
            data_[key_] = std::move(data);
            return *this;
        }

        template <typename T>
        state_proxy operator[](const T& key)
        {
            return state_proxy(key_ + "/" + boost::lexical_cast<std::string>(key), data_);
        }

        template <typename T>
        state_proxy& operator=(const std::vector<T>& data)
        {
            data_[key_] = vector_t(data.begin(), data.end());
            return *this;
        }

        state_proxy& operator=(std::initializer_list<data_t> data)
        {
            data_[key_] = vector_t(std::move(data));
            return *this;
        }

        state_proxy& operator=(const state& other)
        {
            for (auto& p : other) {
                data_[key_ + "/" + p.first] = std::move(p.second);
            }
            return *this;
        }
    };

  public:
    state() = default;
    state(const state& other)
        : data_(other.data_)
    {
    }
    state(data_map_t data)
        : data_(std::move(data))
    {
    }
    state& operator=(const state& other)
    {
        data_ = other.data_;
        return *this;
    }

    template <typename T>
    state_proxy operator[](const T& key)
    {
        return state_proxy(boost::lexical_cast<std::string>(key), data_);
    }

    data_map_t::const_iterator begin() const { return data_.begin(); }

    data_map_t::const_iterator end() const { return data_.end(); }
};

}}} // namespace caspar::core::monitor
