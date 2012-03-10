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

#include "../stdafx.h"

#include "reroute_producer.h"

#include <core/producer/frame_producer.h>
#include <core/frame/draw_frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/pixel_format.h>
#include <core/frame/frame.h>

#include <common/except.h>
#include <common/diagnostics/graph.h>
#include <common/log.h>
#include <common/reactive.h>

#include <asmlib.h>

#include <tbb/concurrent_queue.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>

#include <queue>

namespace caspar { namespace reroute {
//		
//class reroute_producer : public reactive::observer<spl::shared_ptr<const core::frame>>
//					   , public core::frame_producer
//{
//	const spl::shared_ptr<diagnostics::graph>									graph_;
//	const spl::shared_ptr<core::frame_factory>									frame_factory_;
//	
//	tbb::concurrent_bounded_queue<std::shared_ptr<const core::frame>>		input_buffer_;
//	std::queue<core::draw_frame>												frame_buffer_;
//	uint64_t																	frame_number_;
//
//	core::draw_frame															last_frame_;
//
//public:
//	explicit reroute_producer(const spl::shared_ptr<core::frame_factory>& frame_factory) 
//		: frame_factory_(frame_factory)
//		, frame_number_(0)
//		, last_frame_(core::draw_frame::empty())
//	{
//		graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
//		graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
//		graph_->set_text(print());
//		diagnostics::register_graph(graph_);
//
//		input_buffer_.set_capacity(1);
//	}
//	
//	// observable
//
//	void on_next(const spl::shared_ptr<const core::frame>& frame)
//	{
//		if(!input_buffer_.try_push(frame))
//			graph_->set_tag("dropped-frame");
//	}
//
//	// frame_producer
//			
//	core::draw_frame receive(int) override
//	{
//		if(!frame_buffer_.empty())
//		{
//			auto frame = std::move(frame_buffer_.front());
//			frame_buffer_.pop();
//			return last_frame_ = frame;
//		}
//		
//		std::shared_ptr<const core::frame> read_frame;
//		if(!input_buffer_.try_pop(read_frame))
//		{
//			graph_->set_tag("late-frame");
//			return core::draw_frame::late();		
//		}
//		
//		frame_number_++;
//		
//		bool double_speed	= std::abs(frame_factory_->video_format_desc().fps / 2.0 - read_frame->frame_rate()) < 0.01;		
//		bool half_speed		= std::abs(read_frame->frame_rate() / 2.0 - frame_factory_->video_format_desc().fps) < 0.01;
//
//		if(half_speed && frame_number_ % 2 == 0) // Skip frame
//			return receive(0);
//
//		auto frame = frame_factory_->create_frame(this, read_frame->pixel_format_desc());
//
//		A_memcpy(frame->image_data(0).begin(), read_frame->image_data().begin(), read_frame->image_data().size());
//		boost::push_back(frame->audio_data(), read_frame->audio_data());
//		
//		auto draw_frame = core::draw_frame(std::move(frame));
//
//		frame_buffer_.push(draw_frame);
//		
//		if(double_speed)	
//			frame_buffer_.push(draw_frame);
//
//		return receive(0);
//	}	
//
//	core::draw_frame last_frame() const override
//	{
//		return core::draw_frame::still(last_frame_);
//	}
//	
//	std::wstring print() const override
//	{
//		return L"reroute[]";
//	}
//
//	std::wstring name() const override
//	{
//		return L"reroute";
//	}
//
//	boost::property_tree::wptree info() const override
//	{
//		boost::property_tree::wptree info;
//		info.add(L"type", L"rerotue-producer");
//		return info;
//	}
//};

spl::shared_ptr<core::frame_producer> create_producer(const spl::shared_ptr<core::frame_factory>& frame_factory, core::video_channel& channel)
{
	BOOST_THROW_EXCEPTION(not_implemented());
	//return core::frame_producer::empty();
	//auto producer = spl::make_shared<reroute_producer>(frame_factory);
	//o.subscribe(producer);
	//return core::wrap_producer(producer);
}

}}