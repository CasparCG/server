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
 */

#pragma once

#include <locale>

namespace caspar {

// Prefer a guaranteed Unicode-capable locale that requires no OS locale generation (available on
// any glibc >= 2.35), rather than trusting the deployment environment to have one configured -
// falling back to the environment, and finally to "C" (ASCII-only, but never throws), only if
// that isn't recognized (e.g. on Windows/musl). Constructed once and cached. See GitHub issues
// #1364, #1018, #1772.
const std::locale& safe_utf8_locale();

} // namespace caspar
