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
* Author: Cambell Prince, cambell.prince@gmail.com
*/

#pragma once

#include <common/memory/safe_ptr.h>

#include <stdint.h>
#include <string>
#include <vector>

namespace caspar {
	
namespace ffmpeg {

const int MAX_GOP_SIZE = 256;

enum FFMPEG_Resource {
	FFMPEG_FILE,
	FFMPEG_DEVICE,
	FFMPEG_STREAM
};

struct option
{
	std::string name;
	std::string value;

	option(std::string name, std::string value)
		: name(std::move(name))
		, value(std::move(value))
	{
	}
};

struct ffmpeg_producer_params
{
	bool                loop;
	uint32_t            start;
	uint32_t            length;
	std::wstring        filter_str;

	FFMPEG_Resource     resource_type;
	std::wstring        resource_name;

	std::vector<option> options;

	ffmpeg_producer_params() 
		: loop(false)
		, start(0)
		, length(std::numeric_limits<uint32_t>::max())
		, filter_str(L"")
		, resource_type(FFMPEG_FILE)
		, resource_name(L"")
	{
	}

};

}}
