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
 * Author: Julian Waller, julian@superfly.tv
 */

#pragma once

#include "../fwd.h"
#include "../monitor/monitor.h"

#include <common/except.h>
#include <common/memory.h>

#include <core/frame/draw_frame.h>
#include <core/video_format.h>

#include <cstdint>
#include <functional>
#include <future>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace caspar { namespace core {

using producer_factory_t = std::function<spl::shared_ptr<core::frame_producer>(const frame_producer_dependencies&,
                                                                               const std::vector<std::wstring>&)>;

class frame_producer_registry
{
  public:
    frame_producer_registry();
    void register_producer_factory(std::wstring name, const producer_factory_t& factoryr); // Not thread-safe.
    spl::shared_ptr<core::frame_producer> create_producer(const frame_producer_dependencies&,
                                                          const std::vector<std::wstring>& params) const;
    spl::shared_ptr<core::frame_producer> create_producer(const frame_producer_dependencies&,
                                                          const std::wstring& params) const;

  private:
    std::vector<producer_factory_t> producer_factories_;

    frame_producer_registry(const frame_producer_registry&)            = delete;
    frame_producer_registry& operator=(const frame_producer_registry&) = delete;
};

void destroy_producers_synchronously();

}} // namespace caspar::core
