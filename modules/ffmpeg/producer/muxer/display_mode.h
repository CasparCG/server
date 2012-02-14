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
	
enum display_mode
{
	simple,
	duplicate,
	half,
	interlace,
	deinterlace_bob,
	deinterlace_bob_reinterlace,
	deinterlace,
	count,
	invalid
};

template< typename CharT, typename TraitsT >
std::basic_ostream< CharT, TraitsT >& operator<< (std::basic_ostream<CharT, TraitsT>& o, display_mode value)
{	
	switch(value)
	{
		case simple:						return o << L"simple";
		case duplicate:						return o << L"duplicate";
		case half:							return o << L"half";
		case interlace:						return o << L"interlace";
		case deinterlace_bob:				return o << L"deinterlace_bob";
		case deinterlace_bob_reinterlace:	return o << L"deinterlace_bob_reinterlace";
		case deinterlace:					return o << L"deinterlace";
		default:							return o << L"invalid";
	}
}

static display_mode get_display_mode(const core::field_mode in_mode, double in_fps, const core::field_mode out_mode, double out_fps)
{		
	static const auto epsilon = 2.0;

	if(in_fps < 20.0 || in_fps > 80.0)
	{
		//if(out_mode != core::field_mode::progressive && in_mode == core::field_mode::progressive)
		//	return display_mode::interlace;
		
		if(out_mode == core::field_mode::progressive && in_mode != core::field_mode::progressive)
		{
			if(in_fps < 35.0)
				return display_mode::deinterlace;
			else
				return display_mode::deinterlace_bob;
		}
	}

	if(std::abs(in_fps - out_fps) < epsilon)
	{
		if(in_mode != core::field_mode::progressive && out_mode == core::field_mode::progressive)
			return display_mode::deinterlace;
		//else if(in_mode == core::field_mode::progressive && out_mode != core::field_mode::progressive)
		//	simple(); // interlace_duplicate();
		else
			return display_mode::simple;
	}
	else if(std::abs(in_fps/2.0 - out_fps) < epsilon)
	{
		if(in_mode != core::field_mode::progressive)
			return display_mode::invalid;

		if(out_mode != core::field_mode::progressive)
			return display_mode::interlace;
		else
			return display_mode::half;
	}
	else if(std::abs(in_fps - out_fps/2.0) < epsilon)
	{
		if(out_mode != core::field_mode::progressive)
			return display_mode::invalid;

		if(in_mode != core::field_mode::progressive)
			return display_mode::deinterlace_bob;
		else
			return display_mode::duplicate;
	}

	return display_mode::invalid;
}

}}