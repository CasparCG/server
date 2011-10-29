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

#include "image/blend_modes.h"

#include "../producer/frame/frame_factory.h"

#include <common/concurrency/governor.h>
#include <common/memory/safe_ptr.h>

#include <agents.h>

#include <map>

namespace caspar { namespace core {

class read_frame;
class write_frame;
class basic_frame;
struct frame_transform;
struct frame_transform;
class video_channel_context;;
struct pixel_format;
class ogl_device;

class mixer : public core::frame_factory
{
public:	
	
	typedef std::pair<std::map<int, safe_ptr<basic_frame>>, ticket_t>	source_element_t;
	typedef std::pair<safe_ptr<core::read_frame>, ticket_t>				target_element_t;

	typedef Concurrency::ISource<source_element_t>								source_t;
	typedef Concurrency::ITarget<target_element_t>								target_t;

	explicit mixer(source_t& source, target_t& target, const video_format_desc& format_desc, ogl_device& ogl);
						
	virtual safe_ptr<write_frame> create_frame(const void* video_stream_tag, const pixel_format_desc& desc);

	core::video_format_desc get_video_format_desc() const; // nothrow
	
	void set_frame_transform(int index, const core::frame_transform& transform, unsigned int mix_duration = 0, const std::wstring& tween = L"linear");
	void apply_frame_transform(int index, const std::function<core::frame_transform(core::frame_transform)>& transform, unsigned int mix_duration = 0, const std::wstring& tween = L"linear");
	void clear_transforms();

	void set_blend_mode(int index, blend_mode::type value);

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}