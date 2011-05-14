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

#include <core/video_format.h>

#include <tbb/cache_aligned_allocator.h>

#include <boost/noncopyable.hpp>

#include <memory>
#include <vector>

struct AVCodecContext;

namespace caspar {
	
typedef std::vector<unsigned char, tbb::cache_aligned_allocator<unsigned char>> aligned_buffer;

class audio_decoder : boost::noncopyable
{
public:
	explicit audio_decoder(AVCodecContext* codec_context, const core::video_format_desc& format_desc);
	std::vector<std::vector<short>> execute(const std::shared_ptr<aligned_buffer>& audio_packet);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}