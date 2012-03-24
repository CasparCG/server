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
#include <core/video_channel.h>
#include <core/producer/stage.h>

#include <common/except.h>
#include <common/diagnostics/graph.h>
#include <common/log.h>
#include <common/reactive.h>

#include <asmlib.h>

#include <tbb/concurrent_queue.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/adaptor/map.hpp>

#include <queue>

namespace caspar { namespace reroute {
		
class reroute_producer : public reactive::observer<std::map<int, core::draw_frame>>
					   , public core::frame_producer_base
{
	const spl::shared_ptr<diagnostics::graph>						graph_;
	
	tbb::concurrent_bounded_queue<std::map<int, core::draw_frame>>	input_buffer_;
public:
	explicit reroute_producer() 
	{
		graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
		graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
		graph_->set_text(print());
		diagnostics::register_graph(graph_);

		input_buffer_.set_capacity(1);
	}
		
	// observable

	void on_next(const std::map<int, core::draw_frame>& frames)
	{
		if(!input_buffer_.try_push(frames))
			graph_->set_tag("dropped-frame");		
	}

	// frame_producer
			
	core::draw_frame receive_impl() override
	{		
		std::map<int, core::draw_frame> frames;
		if(!input_buffer_.try_pop(frames))
		{
			graph_->set_tag("late-frame");
			return core::draw_frame::late();		
		}

		return boost::accumulate(frames | boost::adaptors::map_values, core::draw_frame::empty(), core::draw_frame::over);
	}	
		
	std::wstring print() const override
	{
		return L"reroute[]";
	}

	std::wstring name() const override
	{
		return L"reroute";
	}

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"rerotue-producer");
		return info;
	}
		
	void subscribe(const monitor::observable::observer_ptr& o) override
	{
	}

	void unsubscribe(const monitor::observable::observer_ptr& o) override
	{
	}
};

spl::shared_ptr<core::frame_producer> create_producer(core::video_channel& channel)
{
	auto producer = spl::make_shared<reroute_producer>();
	
	std::weak_ptr<reactive::observer<std::map<int, core::draw_frame>>> o = producer;

	channel.stage().subscribe(o);

	return producer;
}

}}