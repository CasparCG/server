/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#pragma once

#include <common/memory/safe_ptr.h>

#include <core/video_format.h>

struct AVStream;

namespace caspar {

namespace core {
	struct frame_factory;
	class write_frame;
}

class video_decoder : boost::noncopyable
{
public:
	explicit video_decoder(AVStream* stream, const safe_ptr<core::frame_factory>& frame_factory);
	
	void push(const std::shared_ptr<AVPacket>& packet);
	bool ready() const;
	std::vector<safe_ptr<core::write_frame>> poll();

	core::video_mode::type mode();

	double fps() const;
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}