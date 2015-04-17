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
#include <boost/algorithm/string/case_conv.hpp>

#include <string>
#include <vector>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C"
{
#include <libavutil/pixfmt.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

struct AVFrame;

namespace caspar { namespace ffmpeg {

static std::wstring append_filter(const std::wstring& filters, const std::wstring& filter)
{
	return filters + (filters.empty() ? L"" : L",") + filter;
}

class filter : boost::noncopyable
{
public:
	filter(
		int in_width,
		int in_height,
		boost::rational<int> in_time_base,
		boost::rational<int> in_frame_rate,
		boost::rational<int> in_sample_aspect_ratio,
		AVPixelFormat in_pix_fmt,
		std::vector<AVPixelFormat> out_pix_fmts,
		const std::string& filtergraph);
	filter(filter&& other);
	filter& operator=(filter&& other);

	void push(const std::shared_ptr<AVFrame>& frame);
	std::shared_ptr<AVFrame> poll();
	std::vector<spl::shared_ptr<AVFrame>> poll_all();

	std::wstring filter_str() const;
			
	static bool is_double_rate(const std::wstring& filters)
	{
		if(boost::to_upper_copy(filters).find(L"YADIF=1") != std::string::npos)
			return true;
	
		if(boost::to_upper_copy(filters).find(L"YADIF=3") != std::string::npos)
			return true;

		return false;
	}

	static bool is_deinterlacing(const std::wstring& filters)
	{
		if(boost::to_upper_copy(filters).find(L"YADIF") != std::string::npos)
			return true;	
		return false;
	}	
	
	static int delay(const std::wstring& filters)
	{
		return is_double_rate(filters) ? 1 : 1;
	}

	int delay() const
	{
		return delay(filter_str());
	}

	bool is_double_rate() const
	{
		return is_double_rate(filter_str());
	}
	
	bool is_deinterlacing() const
	{
		return is_deinterlacing(filter_str());
	}
private:
	struct implementation;
	spl::shared_ptr<implementation> impl_;
};

}}
