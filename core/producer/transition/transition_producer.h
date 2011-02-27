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
#include <memory>

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

	std::wstring name() const
	{
		switch(type)
		{
		case transition::cut: return L"cut";
		case transition::mix: return L"mix";
		case transition::push: return L"push";
		case transition::slide: return L"slide";
		case transition::wipe: return L"wipe";
		default: return L"";
		}
	}
	
	size_t						duration;
	transition_direction::type	direction;
	transition::type			type;
};

class transition_producer : public frame_producer
{
public:
	explicit transition_producer(const safe_ptr<frame_producer>& destination, const transition_info& info);
	transition_producer(transition_producer&& other);

	virtual safe_ptr<draw_frame> receive();

	virtual safe_ptr<frame_producer> get_following_producer() const;
	virtual void set_leading_producer(const safe_ptr<frame_producer>& producer);
	virtual void initialize(const safe_ptr<frame_factory>& frame_factory);
	virtual void set_parent_printer(const printer& parent_printer);
	virtual std::wstring print() const;
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}