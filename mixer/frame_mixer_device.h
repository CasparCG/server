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

#include <core/producer/frame/frame_factory.h>
#include <core/producer/frame/pixel_format.h>

#include "image/image_mixer.h"
#include "audio/audio_mixer.h"

#include <common/memory/safe_ptr.h>

#include <boost/signals2.hpp>

#include <functional>

namespace caspar { namespace core {
	
struct video_format_desc;
class read_frame;
class write_frame;

class frame_mixer_device : public frame_factory
{
public:
	typedef boost::signals2::signal<void(const safe_ptr<const read_frame>&)> output_t;
	 
	boost::signals2::connection connect(const output_t::slot_type& subscriber);
	
	frame_mixer_device(const video_format_desc& format_desc);
	frame_mixer_device(frame_mixer_device&& other); // nothrow
		
	void send(const std::vector<safe_ptr<basic_frame>>& frames); // nothrow
		
	safe_ptr<write_frame> create_frame(void* tag, const pixel_format_desc& desc);		
	safe_ptr<write_frame> create_frame(void* tag, size_t width, size_t height, pixel_format::type pix_fmt = pixel_format::bgra);			
	safe_ptr<write_frame> create_frame(void* tag, pixel_format::type pix_fmt = pixel_format::bgra);
	
	const video_format_desc& get_video_format_desc() const; // nothrow

	void set_image_transform(const image_transform& transform, int mix_duration = 0, const std::wstring& tween = L"linear");
	void set_image_transform(int index, const image_transform& transform, int mix_duration = 0, const std::wstring& tween = L"linear");

	void set_audio_transform(const audio_transform& transform, int mix_duration = 0, const std::wstring& tween = L"linear");
	void set_audio_transform(int index, const audio_transform& transform, int mix_duration = 0, const std::wstring& tween = L"linear");
	
	void apply_image_transform(const std::function<image_transform(image_transform)>& transform, int mix_duration = 0, const std::wstring& tween = L"linear");
	void apply_image_transform(int index, const std::function<image_transform(image_transform)>& transform, int mix_duration = 0, const std::wstring& tween = L"linear");

	void apply_audio_transform(const std::function<audio_transform(audio_transform)>& transform, int mix_duration = 0, const std::wstring& tween = L"linear");
	void apply_audio_transform(int index, const std::function<audio_transform(audio_transform)>& transform, int mix_duration = 0, const std::wstring& tween = L"linear");

	void reset_image_transform(int mix_duration = 0, const std::wstring& tween = L"linear");
	void reset_audio_transform(int mix_duration = 0, const std::wstring& tween = L"linear");

private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}