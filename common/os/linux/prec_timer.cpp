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

#include "../../stdafx.h"

#include "../../prec_timer.h"

#include <time.h>

#include <chrono>

using namespace std::chrono;

namespace caspar {

prec_timer::prec_timer()
    : time_(0)
{
}

void prec_timer::tick_nanos(int64_t ticks_to_wait)
{
    auto t = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();

    if (time_ != 0) {
        bool done = 0;
        do {
            auto ticks_passed = t - time_;
            auto ticks_left   = ticks_to_wait - ticks_passed;

            if (t < time_) // time wrap
                done = 1;
            if (ticks_passed >= ticks_to_wait)
                done = 1;

            if (!done) {
                timespec spec;

                if (ticks_left > 2000000) {
                    spec.tv_sec  = ticks_left / 1000000000;
                    spec.tv_nsec = 1000000;
                } else {
                    spec.tv_sec  = 0;
                    spec.tv_nsec = ticks_left / 100;
                }

                nanosleep(&spec, nullptr);
            }

            t = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
        } while (!done);
    }

    time_ = t;
}

} // namespace caspar
