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

#pragma once

#include <queue>
#include <functional>

#include <boost/optional.hpp>

#include "interaction_event.h"
#include "interaction_sink.h"
#include "../frame/frame_transform.h"
#include "util.h"

namespace caspar { namespace core {

typedef std::pair<frame_transform, interaction_sink*> interaction_target;
typedef std::function<boost::optional<interaction_target> (double x, double y)>
		collission_detector;

class interaction_aggregator
{
	std::queue<interaction_event::ptr> events_;
	collission_detector collission_detector_;


	boost::optional<interaction_target> clicked_and_held_;
	int num_buttons_clicked_and_held_;
public:
	interaction_aggregator(const collission_detector& collission_detector)
		: collission_detector_(collission_detector)
		, num_buttons_clicked_and_held_(0)
	{
	}

	void offer(const interaction_event::ptr& event)
	{
		if (!events_.empty()
				&& is<mouse_move_event>(event)
				&& is<mouse_move_event>(events_.back()))
		{
			events_.back() = event;
		}
		else
		{
			events_.push(event);
		}
	}

	void translate_and_send()
	{
		while (!events_.empty())
		{
			translate_and_send(events_.front());
			events_.pop();
		}
	}

	void translate_and_send(const interaction_event::ptr& event)
	{
		if (is<position_event>(event))
		{
			auto pos_event = as<position_event>(event);
			boost::optional<interaction_target> target;

			if (clicked_and_held_)
				target = clicked_and_held_;
			else
				target = collission_detector_(pos_event->x, pos_event->y);

			if (is<mouse_button_event>(event))
			{
				auto button_event = as<mouse_button_event>(event);

				if (button_event->pressed)
				{
					if (num_buttons_clicked_and_held_ == 0)
						clicked_and_held_ = target;

					++num_buttons_clicked_and_held_;
				}
				else
					--num_buttons_clicked_and_held_;

				if (num_buttons_clicked_and_held_ == 0)
					clicked_and_held_.reset();
			}

			if (target)
				target->second->on_interaction(
						pos_event->translate(target->first));
		}
	}
};

}}
