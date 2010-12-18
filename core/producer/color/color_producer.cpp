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

#include "../../processor/draw_frame.h"
#include "../../format/video_format.h"

#include <intrin.h>
#pragma intrinsic(__movsd, __stosd)

namespace caspar { namespace core {
	
unsigned int get_pixel_color_value(const std::wstring& parameter)
{
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

	std::wstring color_code;
	if(parameter.length() != 9 || parameter[0] != '#')
		BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("parameter") << arg_value_info(narrow(parameter)) << msg_info("Invalid color code"));
	
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

class color_producer : public frame_producer
{
public:
	color_producer(color_producer&& other) : frame_(std::move(other.frame_)), color_value_(std::move(other.color_value_)), color_str_(std::move(other.color_str_)){}
	explicit color_producer(const std::wstring& color) : color_str_(color), color_value_(get_pixel_color_value(color)), frame_(draw_frame::empty()){}
	
	safe_ptr<draw_frame> receive()
	{ 
		return frame_;
	}

	void initialize(const safe_ptr<frame_processor_device>& frame_processor)
	{
		auto frame = std::move(frame_processor->create_frame());
		__stosd(reinterpret_cast<unsigned long*>(frame->pixel_data().begin()), color_value_, frame->pixel_data().size() / sizeof(unsigned long));
		frame_ = std::move(frame);
	}
	
	std::wstring print() const
	{
		return + L"color[" + color_str_ + L"]";
	}

	safe_ptr<draw_frame> frame_;
	unsigned int color_value_;
	std::wstring color_str_;
};

safe_ptr<frame_producer> create_color_producer(const std::vector<std::wstring>& params)
{
	if(params.empty() || params[0].at(0) != '#')
		return frame_producer::empty();
	return make_safe<color_producer>(params[0]);
}

}}