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

namespace caspar { namespace core {

struct transition
{
	enum type
	{

		cut = 1,
		mix,
		push,
		slide,
		wipe
	};
};

struct transition_direction
{
	enum type
	{
		from_left = 1,
		from_right
	};
};

struct transition_info
{
	transition_info() : type(transition::cut), duration(0), direction(transition_direction::from_left){}
	
	size_t						duration;
	transition_direction::type	direction;
	transition::type			type;
};

class transition_producer : public frame_producer
{
public:
	transition_producer(const safe_ptr<frame_producer>& destination, const transition_info& info);
	transition_producer(transition_producer&& other) : impl_(std::move(other.impl_)){}
	safe_ptr<draw_frame> receive();

	safe_ptr<frame_producer> get_following_producer() const;
	void set_leading_producer(const safe_ptr<frame_producer>& producer);
	virtual void initialize(const frame_processor_device_ptr& frame_processor);
	virtual std::wstring print() const;
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}