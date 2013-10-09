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

#include <common/memory.h>

#include <string>
#include <vector>

#include "../frame_producer.h"

namespace caspar { namespace core {

class hotswap_producer : public frame_producer_base
{
public:
	hotswap_producer(int width, int height);
	~hotswap_producer();

	draw_frame receive_impl() override;
	constraints& pixel_constraints() override;
	std::wstring print() const override;
	std::wstring name() const override;
	boost::property_tree::wptree info() const override;
	monitor::source& monitor_output();

	binding<std::shared_ptr<frame_producer>>& producer();
private:
	struct impl;
	std::unique_ptr<impl> impl_;
};

}}
