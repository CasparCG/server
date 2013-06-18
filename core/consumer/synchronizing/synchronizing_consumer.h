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

#include "../frame_consumer.h"

#include <vector>
#include <memory>

namespace caspar { namespace core {

class read_frame;
struct video_format_desc;

class synchronizing_consumer : public frame_consumer
{
public:
	synchronizing_consumer(
			const std::vector<safe_ptr<frame_consumer>>& consumers);
	virtual boost::unique_future<bool> send(
			const safe_ptr<read_frame>& frame) override;
	virtual void initialize(
			const video_format_desc& format_desc, int channel_index) override;
	virtual int64_t presentation_frame_age_millis() const override;
	virtual std::wstring print() const override;
	virtual boost::property_tree::wptree info() const override;
	virtual bool has_synchronization_clock() const override;
	virtual size_t buffer_depth() const override;
	virtual int index() const override;
private:
	struct implementation;
	std::unique_ptr<implementation> impl_;
};

}}
