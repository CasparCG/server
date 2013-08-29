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
* Author: Helge Norberg, helge.norberg@svt.se
*/

#include "../../stdafx.h"

#include "const_producer.h"

#include "../frame_producer.h"
#include "../../frame/draw_frame.h"

namespace caspar { namespace core {

class const_producer : public frame_producer_base
{
	std::vector<draw_frame> frames_;
	std::vector<draw_frame>::const_iterator seek_position_;
	constraints constraints_;
public:
	const_producer(const draw_frame& frame, int width, int height)
		: constraints_(width, height)
	{
		frames_.push_back(frame);
		seek_position_ = frames_.begin();
		CASPAR_LOG(info) << print() << L" Initialized";
	}

	const_producer(std::vector<draw_frame>&& frames, int width, int height)
		: frames_(std::move(frames))
		, seek_position_(frames_.begin())
		, constraints_(width, height)
	{
		CASPAR_LOG(info) << print() << L" Initialized";
	}

	draw_frame receive_impl() override
	{
		auto result = *seek_position_;

		if (seek_position_ + 1 != frames_.end())
			++seek_position_;

		return result;
	}

	constraints& pixel_constraints() override
	{
		return constraints_;
	}
	
	std::wstring print() const override
	{
		return L"const[]";
	}

	std::wstring name() const override
	{
		return L"const";
	}
	
	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"const");
		return info;
	}

	void subscribe(const monitor::observable::observer_ptr& o) override															
	{
	}

	void unsubscribe(const monitor::observable::observer_ptr& o) override		
	{
	}
};

spl::shared_ptr<class frame_producer> create_const_producer(
		const class draw_frame& frame, int width, int height)
{
	return spl::make_shared<const_producer>(frame, width, height);
}

spl::shared_ptr<class frame_producer> create_const_producer(
		std::vector<class draw_frame>&& frames, int width, int height)
{
	return spl::make_shared<const_producer>(std::move(frames), width, height);
}

}}
