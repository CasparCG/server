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

#include "descriptor.h"
#include "misc.h"

namespace caspar { namespace psd {

bool descriptor::populate(BEFileInputStream& stream)
{
	bool result = true;

	try
	{
		stream.read_unicode_string();
		stream.read_id_string();
		unsigned long element_count = stream.read_long();
		for(int element_index = 0; element_index < element_count; ++element_index)
		{
			descriptor_item item;

			std::wstring key = stream.read_id_string();
			item.type = stream.read_long();

			switch(item.type)
			{
			case 'obj ': break;
			case 'Objc': break;
			case 'VlLs': break;
			case 'doub': break;
			case 'UntF': break;

			case 'TEXT':
				{
					item.text_text = stream.read_unicode_string();
				}
				break;
			case 'enum':
				{
					item.enum_key = stream.read_id_string();
					item.enum_val = stream.read_id_string();
				}
				break;

			case 'long':
				{
					item.long_value = stream.read_long();
				}
				break;

			case 'bool': break;
			case 'GlbO': break;
			case 'type':
			case 'GlbC':
				break;
			case 'alis': break;

			case 'tdta':
				{
					unsigned long rawdata_length = stream.read_long();
					item.rawdata_data.resize(rawdata_length);
					stream.read(item.rawdata_data.data(), rawdata_length);				
				}
				break;

			default:
				//descriptor type not supported yet
				throw PSDFileFormatException();
			}

			items_.insert(std::pair<std::wstring, descriptor_item>(key, item));
		}
	}
	catch(std::exception& ex)
	{
		result = false;
	}

	return result;
}

}	//namespace psd
}	//namespace caspar