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
 * Author: Julian Waller, julian@supergly.tv
 */

#pragma once

#include <functional>
#include <future>
#include <string>

namespace caspar::html {

const std::string REMOVE_MESSAGE_NAME = "CasparCGRemove";
const std::string LOG_MESSAGE_NAME    = "CasparCGLog";

void              invoke(const std::function<void()>& func);
std::future<void> begin_invoke(const std::function<void()>& func);

} // namespace caspar::html
