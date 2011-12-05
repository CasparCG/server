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

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>

#include <vector>

namespace caspar {

namespace core {
	class video_channel;
}

class server : boost::noncopyable
{
public:
	server();
	const std::vector<safe_ptr<core::video_channel>> get_channels() const;
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}