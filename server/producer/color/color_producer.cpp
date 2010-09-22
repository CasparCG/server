/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
#include "../../stdafx.h"

#include "color_producer.h"

#include "../../frame/system_frame.h"
#include "../../frame/format.h"

#include <intrin.h>
#pragma intrinsic(__movsd, __stosd)

namespace caspar {

class color_producer : public frame_producer
{
public:
	color_producer(unsigned long color_value, const frame_format_desc& format_desc)
	{
		frame_ = std::make_shared<system_frame>(format_desc);
		__stosd(reinterpret_cast<unsigned long*>(frame_->data()), color_value_, frame_->size() / sizeof(unsigned long));
	}

	frame_ptr get_frame() { return frame_; }

	frame_ptr frame_;
	unsigned long color_value_;
};

union Color 
{
	struct Components 
	{
		unsigned char a;
		unsigned char r;
		unsigned char g;
		unsigned char b;
	} comp;

	unsigned long value;
};

unsigned long get_pixel_color_value(const std::wstring& parameter)
{
	std::wstring color_code;
	if(parameter.length() != 9 || parameter[0] != '#')
		BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("parameter") << arg_value_info(common::narrow(parameter)) << msg_info("Invalid color code"));
	
	color_code = parameter.substr(1);

	Color color;
	color.value = _tcstoul(color_code.c_str(), 0, 16);
	unsigned char temp = color.comp.a;
	color.comp.a = color.comp.b;
	color.comp.b = temp;
	temp = color.comp.r;
	color.comp.r = color.comp.g;
	color.comp.g = temp;

	return color.value;
}

frame_producer_ptr create_color_producer(const std::vector<std::wstring>& params, const frame_format_desc& format_desc)
{
	if(params.empty() || params[0].at(0) != '#')
		return nullptr;
	return std::make_shared<color_producer>(get_pixel_color_value(params[0]), format_desc);
}

}