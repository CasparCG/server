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

#include "../../video_format.h"
#include "../../consumer/frame_consumer.h"

namespace caspar { namespace core {
	
class ffmpeg_consumer : public frame_consumer
{
public:	
	explicit ffmpeg_consumer(const std::wstring& filename);
	ffmpeg_consumer(ffmpeg_consumer&& other);
	
	virtual void initialize(const video_format_desc& format_desc);
	virtual void set_parent_printer(const printer& parent_printer);
	virtual void send(const safe_ptr<const read_frame>&);
	virtual size_t buffer_depth() const;
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

safe_ptr<frame_consumer> create_ffmpeg_consumer(const std::vector<std::wstring>& params);

}}