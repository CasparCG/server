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

#include <common/forward.h>
#include <common/spl/memory.h>
#include <common/tweener.h>

#include <boost/property_tree/ptree_fwd.hpp>

#include <functional>
#include <tuple>
#include <vector>

FORWARD2(caspar, diagnostics, class graph);
FORWARD1(boost, template<typename> class unique_future);

namespace caspar { namespace core {
	
class stage sealed
{
	stage(const stage&);
	stage& operator=(const stage&);
public:	
	typedef std::function<struct frame_transform(struct frame_transform)> transform_func_t;
	typedef std::tuple<int, transform_func_t, unsigned int, tweener> transform_tuple_t;

	stage();
		
	std::map<int, spl::shared_ptr<class draw_frame>> operator()(const struct video_format_desc& format_desc);

	void apply_transforms(const std::vector<transform_tuple_t>& transforms);
	void apply_transform(int index, const transform_func_t& transform, unsigned int mix_duration = 0, const tweener& tween = L"linear");
	void clear_transforms(int index);
	void clear_transforms();
				
	void load(int index, const spl::shared_ptr<struct frame_producer>& producer, const boost::optional<int32_t>& auto_play_delta = nullptr);
	void pause(int index);
	void play(int index);
	void stop(int index);
	void clear(int index);
	void clear();	
	void swap_layers(const spl::shared_ptr<stage>& other);
	void swap_layer(int index, int other_index);
	void swap_layer(int index, int other_index, const spl::shared_ptr<stage>& other);
	
	boost::unique_future<spl::shared_ptr<struct frame_producer>>	foreground(int index);
	boost::unique_future<spl::shared_ptr<struct frame_producer>>	background(int index);

	boost::unique_future<boost::property_tree::wptree> info() const;
	boost::unique_future<boost::property_tree::wptree> info(int index) const;
	
private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}