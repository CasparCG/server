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
* Author: Niklas P Andersson, niklas.p.andersson@svt.se
*/

#pragma once

#include <vector>
#include <map>
#include <string>
#include <memory>
#include "util\bigendian_file_input_stream.h"

namespace caspar { namespace psd {

	struct descriptor_item
	{
		unsigned long type;

		std::wstring enum_key;
		std::wstring enum_val;

		std::wstring text_text;

		unsigned long long_value;
		
		std::vector<char> rawdata_data;
	};

class descriptor
{
	typedef std::map<std::wstring, descriptor_item> items_map;

public:
	bool populate(BEFileInputStream& stream);

private:
	items_map items_;
};

}	//namespace psd
}	//namespace caspar