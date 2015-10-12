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
* Author: Helge Norberg, helge.norberg@svt.se
*/

#pragma once

#include <common/except.h>
#include <common/memory.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>
#include <type_traits>

namespace caspar { namespace core { namespace text {

struct freetype_exception : virtual caspar_exception { };

spl::shared_ptr<std::remove_pointer<FT_Library>::type> get_lib_for_thread();
spl::shared_ptr<std::remove_pointer<FT_Face>::type> get_new_face(const std::string& font_file);

}}}
