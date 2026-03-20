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
 *
 * Author: Dimitry Ishenko <dimitry (dot) ishenko (at) (gee) mail (dot) com>
 */

#pragma once

#include "except.h"

#include <chrono>
#include <cmath> // std::floor
#include <sstream>
#include <string>
#include <string_view>
#include <variant>

namespace caspar {

struct timespan
{
    using duration = std::chrono::high_resolution_clock::duration;
    using storage  = std::variant<int, duration>;

    timespan() : timespan{0} { }
    explicit timespan(int frames) : value_{frames} { }

    template<typename Rep, typename Period>
    explicit timespan(std::chrono::duration<Rep, Period> d)
        : value_{std::chrono::duration_cast<duration>(d)}
    { }

    // accepts: "42s", "42ms", "42us", "42µs", "42ns" or "42" (frames)
    explicit timespan(std::string_view s)
    try
    {
        std::istringstream ss{ std::string{s} };
        double v;
        if (!(ss >> v)) throw std::invalid_argument{"Expected a number"};

        std::string suffix;
        ss >> suffix;
        if (!suffix.empty() && !(ss >> std::ws).eof())
            throw std::invalid_argument{"Unexpected characters after suffix"};

        if (suffix.empty()) {
            if (v != std::trunc(v)) throw std::invalid_argument{"Fractional frame count"};
            value_ = static_cast<int>(v);
        } else if (suffix ==  "s") {
            value_ = std::chrono::duration_cast<duration>(std::chrono::duration<double>{v});
        } else if (suffix == "ms") {
            value_ = std::chrono::duration_cast<duration>(std::chrono::duration<double, std::milli>{v});
        } else if (suffix == "us" || suffix == "µs" || suffix == "μs") {
            value_ = std::chrono::duration_cast<duration>(std::chrono::duration<double, std::micro>{v});
        } else if (suffix == "ns") {
            value_ = std::chrono::duration_cast<duration>(std::chrono::duration<double, std::nano >{v});
        } else {
            throw std::invalid_argument{
                "Invalid suffix '" + suffix + "' - expected s, ms, us/µs, ns, or nothing (frames)"
            };
        }
    } catch (...) {
        CASPAR_THROW_EXCEPTION(user_error()
            << msg_info("Failed to parse timespan '" + std::string{s} + "'")
            << nested_exception(std::current_exception())
        );
    }

    int in_frames(double fps) const
    {
        if (std::holds_alternative<int>(value_)) return std::get<int>(value_);

        auto dura  = std::chrono::duration<double>{1. / fps};
        auto total = std::chrono::duration<double>{std::get<duration>(value_)};
        return static_cast<int>(total / dura + .5);
    }

    duration as_duration(double fps) const
    {
        if (std::holds_alternative<duration>(value_))
            return std::get<duration>(value_);

        auto dura = std::chrono::duration<double>(1. / fps);
        return std::chrono::duration_cast<duration>(dura * std::get<int>(value_));
    }

private:
    storage value_;
};

}
