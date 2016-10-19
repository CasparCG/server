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

#include <core/video_format.h>

#include <ostream>

namespace caspar { namespace ffmpeg {
	
enum class display_mode
{
	simple,
	deinterlace_bob,
	invalid
};

template< typename CharT, typename TraitsT >
std::basic_ostream< CharT, TraitsT >& operator<< (std::basic_ostream<CharT, TraitsT>& o, display_mode value)
{	
	switch(value)
	{
	case display_mode::simple:						return o << L"simple";
	case display_mode::deinterlace_bob:				return o << L"deinterlace_bob";
	default:										return o << L"invalid";
	}
}

static display_mode get_display_mode(const core::field_mode in_mode)
{
	return in_mode == core::field_mode::progressive ? display_mode::simple : display_mode::deinterlace_bob;
}

}}