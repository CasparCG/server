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
	const std::string color_str_;

public:
	explicit color_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::string& color) 
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

	virtual std::string print() const override
	{
		return "color[" + color_str_ + "]";
	}

	boost::property_tree::ptree info() const override
	{
		boost::property_tree::ptree info;
		info.add("type", "color-producer");
		info.add("color", color_str_);
		return info;
	}
};

std::string get_hex_color(const std::string& str)
{
	if(str.at(0) == '#')
		return str.length() == 7 ? "#FF" + str.substr(1) : str;
	
	if(iequals(str, "EMPTY"))
		return "#00000000";

	if(iequals(str, "BLACK"))
		return "#FF000000";
	
	if(iequals(str, "WHITE"))
		return "#FFFFFFFF";
	
	if(iequals(str, "RED"))
		return "#FFFF0000";
	
	if(iequals(str, "GREEN"))
		return "#FF00FF00";
	
	if(iequals(str, "BLUE"))
		return "#FF0000FF";	
	
	if(iequals(str, "ORANGE"))
		return "#FFFFA500";	
	
	if(iequals(str, "YELLOW"))
		return "#FFFFFF00";
	
	if(iequals(str, "BROWN"))
		return "#FFA52A2A";
	
	if(iequals(str, "GRAY"))
		return "#FF808080";
	
	if(iequals(str, "TEA"))
		return "#FF008080";
	
	return str;
}

safe_ptr<frame_producer> create_color_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::string>& params)
{
	if(params.size() < 0)
		return core::frame_producer::empty();

	auto color2 = get_hex_color(params[0]);
	if(color2.length() != 9 || color2[0] != '#')
		return core::frame_producer::empty();

	return make_safe<color_producer>(frame_factory, color2);
}
safe_ptr<core::write_frame> create_color_frame(void* tag, const safe_ptr<core::frame_factory>& frame_factory, const std::string& color)
{
	auto color2 = get_hex_color(color);
	if(color2.length() != 9 || color2[0] != '#')
		BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("color") << arg_value_info((color2)) << msg_info("Invalid color."));
	
	core::pixel_format_desc desc;
	desc.pix_fmt = pixel_format::bgra;
	desc.planes.push_back(core::pixel_format_desc::plane(1, 1, 4));
	auto frame = frame_factory->create_frame(tag, desc);
		
	// Read color from hex-string and write to frame pixel.

	auto& value = *reinterpret_cast<uint32_t*>(frame->image_data().begin());
	std::stringstream str(color2.substr(1));
	if(!(str >> std::hex >> value) || !str.eof())
		BOOST_THROW_EXCEPTION(invalid_argument() << arg_name_info("color") << arg_value_info((color2)) << msg_info("Invalid color."));

	frame->commit();
		
	return frame;
}

}}