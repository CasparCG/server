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

#include "../producer/frame/frame_factory.h"

#include <common/memory/safe_ptr.h>

#include <map>

namespace caspar { 

class executor;
	
namespace core {

class read_frame;
class write_frame;
class basic_frame;
class audio_transform;
class image_transform;
class video_channel_context;;
struct pixel_format;

class mixer : public core::frame_factory
{
public:	
	explicit mixer(video_channel_context& video_channel);
		
	safe_ptr<core::read_frame> execute(const std::map<int, safe_ptr<core::basic_frame>>& frames); // nothrow
		
	safe_ptr<core::write_frame> create_frame(const void* tag, const core::pixel_format_desc& desc);		
	safe_ptr<core::write_frame> create_frame(const void* tag, size_t width, size_t height, core::pixel_format::type pix_fmt = core::pixel_format::bgra);		
	
	core::video_format_desc get_video_format_desc() const; // nothrow

	void reset_transforms();

	void set_image_transform(const core::image_transform& transform, unsigned int mix_duration = 0, const std::wstring& tween = L"linear");
	void set_image_transform(int index, const core::image_transform& transform, unsigned int mix_duration = 0, const std::wstring& tween = L"linear");

	void set_audio_transform(const core::audio_transform& transform, unsigned int mix_duration = 0, const std::wstring& tween = L"linear");
	void set_audio_transform(int index, const core::audio_transform& transform, unsigned int mix_duration = 0, const std::wstring& tween = L"linear");
	
	void apply_image_transform(const std::function<core::image_transform(core::image_transform)>& transform, unsigned int mix_duration = 0, const std::wstring& tween = L"linear");
	void apply_image_transform(int index, const std::function<core::image_transform(core::image_transform)>& transform, unsigned int mix_duration = 0, const std::wstring& tween = L"linear");

	void apply_audio_transform(const std::function<core::audio_transform(core::audio_transform)>& transform, unsigned int mix_duration = 0, const std::wstring& tween = L"linear");
	void apply_audio_transform(int index, const std::function<core::audio_transform(core::audio_transform)>& transform, unsigned int mix_duration = 0, const std::wstring& tween = L"linear");

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}