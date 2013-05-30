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
* Author: Julian Waller
*/
#pragma once

#include <string>

namespace caspar {
namespace ffmpeg {

enum FFMPEG_Resource {
	FFMPEG_FILE,
	FFMPEG_DEVICE,
	FFMPEG_STREAM
};

struct ffmpeg_params
{
	std::wstring  size_str;
	std::wstring  pixel_format;
	std::wstring  frame_rate;


	ffmpeg_params() 
		: size_str(L"")
		, pixel_format(L"")
		, frame_rate(L"")
	{
	}

};

}}