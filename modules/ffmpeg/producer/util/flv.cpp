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

#include "../../stdafx.h"

#include "flv.h"

#include <common/except.h>
#include <common/log.h>

#include <boost/filesystem.hpp>

#include <iostream>

#include <unordered_map>

namespace caspar { namespace ffmpeg {
	
std::map<std::string, std::string> read_flv_meta_info(const std::string& filename)
{
	std::map<std::string, std::string>  values;

	if(boost::filesystem::path(filename).extension().string() != ".flv")
		return values;
	
	try
	{
		if(!boost::filesystem::exists(filename))
			CASPAR_THROW_EXCEPTION(caspar_exception());
	
		std::fstream fileStream = std::fstream(filename, std::fstream::in);
		
		std::vector<char> bytes2(256);
		fileStream.read(bytes2.data(), bytes2.size());

		auto ptr = bytes2.data();
		
		ptr += 27;
						
		if(std::string(ptr, ptr+10) == "onMetaData")
		{
			ptr += 16;

			for(int n = 0; n < 16; ++n)
			{
				char name_size = *ptr++;

				if(name_size == 0)
					break;

				auto name = std::string(ptr, ptr + name_size);
				ptr += name_size;

				char data_type = *ptr++;
				switch(data_type)
				{
				case 0: // double
					{
						static_assert(sizeof(double) == 8, "");
						std::reverse(ptr, ptr+8);
						values[name] = boost::lexical_cast<std::string>(*(double*)(ptr));
						ptr += 9;

						break;
					}
				case 1: // bool
					{
						values[name] = boost::lexical_cast<std::string>(*ptr != 0);
						ptr += 2;

						break;
					}
				}
			}
		}
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
	}

    return values;
}

}}