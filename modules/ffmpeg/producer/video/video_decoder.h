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

#include <core/mixer/frame_mixer_device.h>

struct AVCodecContext;

namespace caspar {
	
typedef std::vector<unsigned char, tbb::cache_aligned_allocator<unsigned char>> aligned_buffer;

class video_decoder : boost::noncopyable
{
public:
	explicit video_decoder(AVCodecContext* codec_context, const safe_ptr<core::frame_factory>& frame_factory);
	std::vector<safe_ptr<core::write_frame>> execute(void* tag, const std::shared_ptr<aligned_buffer>& video_packet);	
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}