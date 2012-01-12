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

#pragma once

#include "frame_producer.h"

#include <common/memory/safe_ptr.h>
#include <common/concurrency/target.h>
#include <common/diagnostics/graph.h>

#include <boost/noncopyable.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#include <boost/thread/future.hpp>

#include <functional>

namespace caspar { namespace core {

struct video_format_desc;
struct frame_transform;

class stage : boost::noncopyable
{
public:
	typedef std::function<struct frame_transform(struct frame_transform)>							transform_func_t;
	typedef std::tuple<int, transform_func_t, unsigned int, std::wstring>							transform_tuple_t;
	typedef target<std::pair<std::map<int, safe_ptr<basic_frame>>, std::shared_ptr<void>>> target_t;

	explicit stage(const safe_ptr<diagnostics::graph>& graph, const safe_ptr<target_t>& target, const video_format_desc& format_desc);
	
	// stage
	
	void apply_transforms(const std::vector<transform_tuple_t>& transforms);
	void apply_transform(int index, const transform_func_t& transform, unsigned int mix_duration = 0, const std::wstring& tween = L"linear");
	void clear_transforms(int index);
	void clear_transforms();

	void spawn_token();
			
	void load(int index, const safe_ptr<frame_producer>& producer, bool preview = false, int auto_play_delta = -1);
	void pause(int index);
	void play(int index);
	void stop(int index);
	void clear(int index);
	void clear();	
	void swap_layers(const safe_ptr<stage>& other);
	void swap_layer(int index, size_t other_index);
	void swap_layer(int index, size_t other_index, const safe_ptr<stage>& other);
	
	boost::unique_future<std::wstring>				call(int index, bool foreground, const std::wstring& param);
	boost::unique_future<safe_ptr<frame_producer>>	foreground(int index);
	boost::unique_future<safe_ptr<frame_producer>>	background(int index);

	boost::unique_future<boost::property_tree::wptree> info() const;
	boost::unique_future<boost::property_tree::wptree> info(int layer) const;
	
	void set_video_format_desc(const video_format_desc& format_desc);

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}