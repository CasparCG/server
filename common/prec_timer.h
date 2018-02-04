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

#include <cstdint>

namespace caspar {

class prec_timer
{
  public:
    prec_timer();

    void tick(double interval) { tick_nanos(static_cast<int64_t>(interval * 1000000000.0)); }

    void tick_millis(int64_t interval) { tick_nanos(interval * 1000000); }

    // Author: Ryan M. Geiss
    // http://www.geisswerks.com/ryan/FAQS/timing.html
    void tick_nanos(int64_t interval);

  private:
    int64_t time_;
};

} // namespace caspar
