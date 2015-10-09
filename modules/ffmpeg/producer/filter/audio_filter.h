/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include <boost/rational.hpp>
#include <boost/noncopyable.hpp>

#include <string>
#include <vector>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C"
{
#include <libavutil/samplefmt.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

struct AVFrame;

namespace caspar { namespace ffmpeg {

class audio_filter : boost::noncopyable
{
public:
	audio_filter(
			boost::rational<int> in_time_base,
			int in_sample_rate,
			AVSampleFormat in_sample_fmt,
			std::int64_t in_audio_channel_layout,
			std::vector<int> out_sample_rates,
			std::vector<AVSampleFormat> out_sample_fmts,
			std::vector<std::int64_t> out_audio_channel_layouts,
			const std::string& filtergraph);
	audio_filter(audio_filter&& other);
	audio_filter& operator=(audio_filter&& other);

	void push(const std::shared_ptr<AVFrame>& frame);
	std::shared_ptr<AVFrame> poll();
	std::vector<spl::shared_ptr<AVFrame>> poll_all();

	std::wstring filter_str() const;
private:
	struct implementation;
	spl::shared_ptr<implementation> impl_;
};

}}
