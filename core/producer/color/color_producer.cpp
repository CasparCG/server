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

#include "../../format/video_format.h"

#include <intrin.h>
#pragma intrinsic(__movsd, __stosd)

namespace caspar { namespace core {

class color_producer : public frame_producer
{
public:
	explicit color_producer(unsigned int color_value) 
		: color_value_(color_value){}

	~color_producer()
	{
		if(frame_processor_)
			frame_processor_->release_tag(this);
	}

	frame_ptr render_frame()
	{ 
		return frame_;
	}

	void initialize(const frame_processor_device_ptr& frame_processor)
	{
		frame_processor_ = frame_processor;
		frame_ = frame_processor->create_frame(this);
		__stosd(reinterpret_cast<unsigned long*>(frame_->data()), color_value_, frame_->size() / sizeof(unsigned long));
	}

	frame_processor_device_ptr frame_processor_;
	frame_ptr frame_;
	unsigned int color_value_;
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

	unsigned int value;
};

unsigned int get_pixel_color_value(const std::wstring& parameter)
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

frame_producer_ptr create_color_producer(const std::vector<std::wstring>& params)
{
	if(params.empty() || params[0].at(0) != '#')
		return nullptr;
	return std::make_shared<color_producer>(get_pixel_color_value(params[0]));
}

}}