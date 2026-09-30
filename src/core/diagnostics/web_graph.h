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
 */

#pragma once

#include <string>

namespace caspar { namespace core { namespace diagnostics { namespace web {

// Registers a graph_sink factory that mirrors every caspar::diagnostics::graph's text/values/colors/tags
// into a small in-memory snapshot, for serving over HTTP (see protocol/util/diag_http_server.h). This is
// independent of, and much cheaper than, the SFML osd window - it does no rendering at all.
void register_sink();

// A JSON snapshot of every currently-live graph's text/values/colors/tags.
std::string snapshot_json();

}}}} // namespace caspar::core::diagnostics::web
