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

#include <core/consumer/frame_consumer.h>

#include <core/video_format.h>

#include <string>
#include <vector>

namespace caspar { 

class decklink_consumer : public core::frame_consumer
{
public:

	enum key
	{
		external_key,
		internal_key,
		default_key
	};

	explicit decklink_consumer(size_t device_index, bool embed_audio = false, key key = default_key);
	decklink_consumer(decklink_consumer&& other);
	
	virtual void initialize(const core::video_format_desc& format_desc);
	virtual void send(const safe_ptr<const core::read_frame>&);
	virtual size_t buffer_depth() const;
	virtual std::wstring print() const;

private:
	struct implementation;
	std::tr1::shared_ptr<implementation> impl_;
};

safe_ptr<core::frame_consumer> create_decklink_consumer(const std::vector<std::wstring>& params);

}