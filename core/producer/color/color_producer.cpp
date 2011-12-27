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

#include "color_producer.h"

#include "../frame/basic_frame.h"
#include "../frame/frame_factory.h"
#include "../../mixer/write_frame.h"

#include <common/exception/exceptions.h>

#include <boost/algorithm/string.hpp>

#include <sstream>

namespace caspar { namespace core {
	
class color_producer : public frame_producer
{
	safe_ptr<basic_frame> frame_;
	const std::wstring color_str_;

public:
	explicit color_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& color) 
		: color_str_(color)
		, frame_(create_color_frame(this, frame_factory, color))
	{}

	// frame_producer
			
	virtual safe_ptr<basic_frame> receive(int) override
	{
		return frame_;
	}	

	virtual safe_ptr<basic_frame> last_frame() const override
	{
		return frame_; 
	}	

	virtual std::wstring print() const override
	{
		return L"color[" + color_str_ + L"]";
	}

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"color-producer");
		info.add(L"color", color_str_);
		return info;
	}
};

std::wstring get_hex_color(const std::wstring& str)
{
	if(str.at(0) == '#')
		return str.length() == 7 ? L"#FF" + str.substr(1) : str;
	
	if(boost::iequals(str, L"EMPTY"))
		return L"#00000000";

	if(boost::iequals(str, L"BLACK"))
		return L"#FF000000";
	
	if(boost::iequals(str, L"WHITE"))
		return L"#FFFFFFFF";
	
	if(boost::iequals(str, L"RED"))
		return L"#FFFF0000";
	
	if(boost::iequals(str, L"GREEN"))
		return L"#FF00FF00";
	
	if(boost::iequals(str, L"BLUE"))
		return L"#FF0000FF";	
	
	if(boost::iequals(str, L"ORANGE"))
		return L"#FFFFA500";	
	
	if(boost::iequals(str, L"YELLOW"))
		return L"#FFFFFF00";
	
	if(boost::iequals(str, L"BROWN"))
		return L"#FFA52A2A";
	
	if(boost::iequals(str, L"GRAY"))
		return L"#FF808080";
	
	if(boost::iequals(str, L"TEAL"))
		return L"#FF008080";
	
	return str;
}

safe_ptr<frame_producer> create_color_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params)
{
	if(params.size() < 0)
		return core::frame_producer::empty();

	auto color2 = get_hex_color(params[0]);
	if(color2.length() != 9 || color2[0] != '#')
		return core::frame_producer::empty();

	return create_producer_print_proxy(
			make_safe<color_producer>(frame_factory, color2));
}
safe_ptr<core::write_frame> create_color_frame(void* tag, const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& color)
{
	auto color2 = get_hex_color(color);
	if(color2.length() != 9 || color2[0] != '#')
		BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("color") << arg_value_info(narrow(color2)) << msg_info("Invalid color."));
	
	core::pixel_format_desc desc;
	desc.pix_fmt = pixel_format::bgra;
	desc.planes.push_back(core::pixel_format_desc::plane(1, 1, 4));
	auto frame = frame_factory->create_frame(tag, desc);
		
	// Read color from hex-string and write to frame pixel.

	auto& value = *reinterpret_cast<uint32_t*>(frame->image_data().begin());
	std::wstringstream str(color2.substr(1));
	if(!(str >> std::hex >> value) || !str.eof())
		BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("color") << arg_value_info(narrow(color2)) << msg_info("Invalid color."));

	frame->commit();
		
	return frame;
}

}}