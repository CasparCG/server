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

#include <core/producer/frame_producer.h>
#include <core/producer/stage.h>
#include <core/video_format.h>
#include <core/video_channel.h>

#include <boost/thread/future.hpp>

#include <string>

namespace caspar { namespace flash {
		
class cg_proxy
{
public:
	static const unsigned int DEFAULT_LAYER = 9999;

	explicit cg_proxy(const spl::shared_ptr<core::frame_producer>& producer);
	cg_proxy(cg_proxy&& other);
	
	//cg_proxy

	void add(int layer, const std::wstring& template_name,  bool play_on_load, const std::wstring& start_from_label = L"", const std::wstring& data = L"");
	void remove(int layer);
	void play(int layer);
	void stop(int layer, unsigned int mix_out_duration);
	void next(int layer);
	void update(int layer, const std::wstring& data);
	std::wstring invoke(int layer, const std::wstring& label);
	std::wstring description(int layer);
	std::wstring template_host_info();

private:
	struct impl;
	std::shared_ptr<impl> impl_;
};
cg_proxy create_cg_proxy(const spl::shared_ptr<core::video_channel>& video_channel, int layer_index = cg_proxy::DEFAULT_LAYER);

spl::shared_ptr<core::frame_producer> create_ct_producer(const spl::shared_ptr<core::frame_factory> frame_factory, const core::video_format_desc& format_desc, const std::vector<std::wstring>& params);
spl::shared_ptr<core::frame_producer> create_swf_producer(const spl::shared_ptr<core::frame_factory> frame_factory, const core::video_format_desc& format_desc, const std::vector<std::wstring>& params);

}}
