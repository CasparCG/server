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

#include "freetype_library.h"

#include <boost/thread/tss.hpp>

namespace caspar { namespace core { namespace text {

spl::shared_ptr<std::remove_pointer<FT_Library>::type> get_lib_for_thread()
{
	typedef std::remove_pointer<FT_Library>::type non_ptr_type;
	static boost::thread_specific_ptr<spl::shared_ptr<non_ptr_type>> libs;

	auto result = libs.get();

	if (!result)
	{
		FT_Library raw_lib;

		if (FT_Init_FreeType(&raw_lib))
			CASPAR_THROW_EXCEPTION(freetype_exception() << msg_info("Failed to initialize freetype"));

		auto lib = spl::shared_ptr<non_ptr_type>(raw_lib, FT_Done_FreeType);
		result = new spl::shared_ptr<non_ptr_type>(std::move(lib));

		libs.reset(result);
	}

	return *result;
}

spl::shared_ptr<std::remove_pointer<FT_Face>::type> get_new_face(
		const std::string& font_file, const std::string& font_name)
{
	if (font_file.empty())
		CASPAR_THROW_EXCEPTION(expected_freetype_exception() << msg_info("Failed to find font file for \"" + font_name + "\""));

	auto lib = get_lib_for_thread();
	FT_Face face;
	if (FT_New_Face(lib.get(), u8(font_file).c_str(), 0, &face))
		CASPAR_THROW_EXCEPTION(freetype_exception() << msg_info("Failed to load font file \"" + font_file + "\""));

	return spl::shared_ptr<std::remove_pointer<FT_Face>::type>(face, [lib](FT_Face p)
	{
		FT_Done_Face(p);
	});
}

}}}
