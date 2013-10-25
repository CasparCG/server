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

#include "freehand_producer.h"

#include <core/producer/frame_producer.h>
#include <core/frame/frame.h>
#include <core/frame/draw_frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/pixel_format.h>
#include <core/monitor/monitor.h>

namespace caspar { namespace core {

std::vector<uint8_t, tbb::cache_aligned_allocator<uint8_t>> empty_drawing(
		int width, int height)
{
	std::vector<uint8_t, tbb::cache_aligned_allocator<uint8_t>> result;

	result.resize(width * height * 4);
	std::fill(result.begin(), result.end(), 0);

	return std::move(result);
}

class freehand_producer : public frame_producer_base
{
	monitor::subject											monitor_subject_;
	std::vector<uint8_t, tbb::cache_aligned_allocator<uint8_t>>	drawing_;
	spl::shared_ptr<core::frame_factory>						frame_factory_;
	constraints													constraints_;
	draw_frame													frame_;
	bool														button_pressed_;
	bool														modified_;
public:
	explicit freehand_producer(
			const spl::shared_ptr<core::frame_factory>& frame_factory,
			int width,
			int height)
		: drawing_(std::move(empty_drawing(width, height)))
		, frame_factory_(frame_factory)
		, constraints_(width, height)
		, frame_(create_frame())
		, button_pressed_(false)
		, modified_(false)
	{
		CASPAR_LOG(info) << print() << L" Initialized";
	}

	draw_frame create_frame()
	{
		pixel_format_desc desc(pixel_format_def::bgra);
		desc.planes.push_back(pixel_format_desc::plane(
				static_cast<int>(constraints_.width.get()),
				static_cast<int>(constraints_.height.get()),
				4));
		auto frame = frame_factory_->create_frame(this, desc);

		std::memcpy(frame.image_data().begin(), drawing_.data(), drawing_.size());

		return draw_frame(std::move(frame));
	}

	// frame_producer

	void on_interaction(const interaction_event::ptr& event) override
	{
		if (is<mouse_move_event>(event) && button_pressed_)
		{
			auto mouse_move = as<mouse_move_event>(event);

			int x = static_cast<int>(mouse_move->x * constraints_.width.get());
			int y = static_cast<int>(mouse_move->y * constraints_.height.get());

			if (x >= constraints_.width.get() 
					|| y >= constraints_.height.get()
					|| x < 0
					|| y < 0)
				return;

			uint8_t* b = drawing_.data() + (y * static_cast<int>(constraints_.width.get()) + x) * 4;
			uint8_t* g = b + 1;
			uint8_t* r = b + 2;
			uint8_t* a = b + 3;

			*b = 255;
			*g = 255;
			*r = 255;
			*a = 255;

			modified_ = true;
		}
		else if (is<mouse_button_event>(event))
		{
			auto button_event = as<mouse_button_event>(event);

			if (button_event->button == 0)
				button_pressed_ = button_event->pressed;
			else if (button_event->button == 1 && button_event->pressed)
			{
				std::memset(drawing_.data(), 0, drawing_.size());
				modified_ = true;
			}
		}
	}

	bool collides(double x, double y) const override
	{
		return true;
	}
			
	draw_frame receive_impl() override
	{
		if (modified_)
		{
			frame_ = create_frame();
			modified_ = false;
		}

		return frame_;
	}

	constraints& pixel_constraints() override
	{
		return constraints_;
	}
	
	std::wstring print() const override
	{
		return L"freehand[]";
	}

	std::wstring name() const override
	{
		return L"freehand";
	}
	
	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"freehand");
		return info;
	}

	monitor::subject& monitor_output()
	{
		return monitor_subject_;
	}
};

spl::shared_ptr<frame_producer> create_freehand_producer(const spl::shared_ptr<frame_factory>& frame_factory, const std::vector<std::wstring>& params)
{
	if(params.size() < 3 || !boost::iequals(params.at(0), L"[FREEHAND]"))
		return core::frame_producer::empty();

	int width = boost::lexical_cast<int>(params.at(1));
	int height = boost::lexical_cast<int>(params.at(2));

	return spl::make_shared<freehand_producer>(frame_factory, width, height);
}

}}
