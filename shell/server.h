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

#include <common/memory.h>
#include <common/future_fwd.h>

#include <core/monitor/monitor.h>

#include <boost/noncopyable.hpp>

#include <vector>

namespace caspar {

namespace core {
	class video_channel;
	class thumbnail_generator;
}

class server /* final */ : public boost::noncopyable
{
public:
	explicit server(std::promise<bool>& shutdown_server_now);
	const std::vector<spl::shared_ptr<core::video_channel>> channels() const;
	std::shared_ptr<core::thumbnail_generator> get_thumbnail_generator() const;

	core::monitor::subject& monitor_output();
private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}