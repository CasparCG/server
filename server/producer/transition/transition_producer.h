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
#pragma once

#include "../frame_producer.h"

#include <string>
#include <vector>

namespace caspar {

enum transition_type
{
	cut = 1,
	mix,
	push,
	slide,
	wipe
};

enum transition_direction
{
	from_left = 1,
	from_right,
	from_top,
	from_bottom
};

struct transition_info
{
	transition_info() : type(transition_type::cut), duration(0), border_width(0), border_color(TEXT("#00000000")), direction(transition_direction::from_left){}
    
	unsigned short					duration;
	unsigned short					border_width;
	transition_direction			direction;
	transition_type					type;
	std::wstring					border_color;
	std::wstring					border_image;
};

class transition_producer : public frame_producer
{
public:
	transition_producer(const frame_producer_ptr& destination, const transition_info& info, const frame_format_desc& fmt);

	frame_ptr get_frame();

	frame_producer_ptr get_following_producer() const;
	void set_leading_producer(const frame_producer_ptr& producer);
	const frame_format_desc& get_frame_format_desc() const;
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<transition_producer> transition_producer_ptr;

}