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

#include "consumer/frame_consumer_device.h"
#include "mixer/frame_mixer_device.h"
#include "producer/frame_producer_device.h"

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>

namespace caspar { namespace core {
	
class channel : boost::noncopyable
{
public:
	explicit channel(int index, const video_format_desc& format_desc);
	channel(channel&& other);

	safe_ptr<frame_producer_device> producer();
	safe_ptr<frame_mixer_device>	mixer();
	safe_ptr<frame_consumer_device> consumer();

	const video_format_desc& get_video_format_desc() const;
	void set_video_format_desc(const video_format_desc& format_desc);

	std::wstring print() const;

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}