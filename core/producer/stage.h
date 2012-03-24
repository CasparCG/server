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

#include "../monitor/monitor.h"

#include <common/forward.h>
#include <common/future_fwd.h>
#include <common/memory.h>
#include <common/tweener.h>

#include <boost/optional.hpp>
#include <boost/property_tree/ptree_fwd.hpp>

#include <functional>
#include <map>
#include <tuple>
#include <vector>

FORWARD2(caspar, diagnostics, class graph);

namespace caspar { namespace core {

typedef reactive::observable<std::map<int, class draw_frame>> frame_observable;
	
class stage sealed : public monitor::observable, public frame_observable
{
	stage(const stage&);
	stage& operator=(const stage&);
public:	

	// Static Members
	
	typedef std::function<struct frame_transform(struct frame_transform)> transform_func_t;
	typedef std::tuple<int, transform_func_t, unsigned int, tweener> transform_tuple_t;

	// Constructors

	explicit stage(spl::shared_ptr<diagnostics::graph> graph);
	
	// Methods

	std::map<int, class draw_frame> operator()(const struct video_format_desc& format_desc);

	boost::unique_future<void>			apply_transforms(const std::vector<transform_tuple_t>& transforms);
	boost::unique_future<void>			apply_transform(int index, const transform_func_t& transform, unsigned int mix_duration = 0, const tweener& tween = L"linear");
	boost::unique_future<void>			clear_transforms(int index);
	boost::unique_future<void>			clear_transforms();				
	boost::unique_future<void>			load(int index, const spl::shared_ptr<class frame_producer>& producer, bool preview = false, const boost::optional<int32_t>& auto_play_delta = nullptr);
	boost::unique_future<void>			pause(int index);
	boost::unique_future<void>			play(int index);
	boost::unique_future<void>			stop(int index);
	boost::unique_future<std::wstring>	call(int index, const std::wstring& params);
	boost::unique_future<void>			clear(int index);
	boost::unique_future<void>			clear();	
	boost::unique_future<void>			swap_layers(stage& other);
	boost::unique_future<void>			swap_layer(int index, int other_index);
	boost::unique_future<void>			swap_layer(int index, int other_index, stage& other);	

	// monitor::observable

	void subscribe(const monitor::observable::observer_ptr& o) override;
	void unsubscribe(const monitor::observable::observer_ptr& o) override;
	
	// frame_observable

	void subscribe(const frame_observable::observer_ptr& o) override;
	void unsubscribe(const frame_observable::observer_ptr& o) override;

	// Properties

	boost::unique_future<spl::shared_ptr<class frame_producer>>	foreground(int index);
	boost::unique_future<spl::shared_ptr<class frame_producer>>	background(int index);

	boost::unique_future<boost::property_tree::wptree>			info() const;
	boost::unique_future<boost::property_tree::wptree>			info(int index) const;

private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}