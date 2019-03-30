/*
 * Copyright 2018
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
 * Author: Krzysztof Zegzula, zegzulakrzysztof@gmail.com
 * based on work of Robert Nagy, ronag89@gmail.com
 */

#pragma once

#include <common/memory.h>

#include <core/fwd.h>

#include <boost/property_tree/ptree_fwd.hpp>

#include <string>

namespace caspar { namespace newtek {

spl::shared_ptr<core::frame_producer> create_ndi_producer(const core::frame_producer_dependencies& dependencies,
                                                          const std::vector<std::wstring>&         params);

}} // namespace caspar::newtek
