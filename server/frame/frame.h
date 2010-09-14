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

#include <memory>
#include <vector>

#include "audio_chunk.h"
#include "frame_format.h"

#include "../../common/image/image.h"

#include <tbb/parallel_invoke.h>

#include <boost/range/algorithm.hpp>

namespace caspar {

// NOTE: audio data is ALWAYS shallow copy
class frame : boost::noncopyable
{
public:
	virtual ~frame(){}
	
	virtual const unsigned char* data() const { return const_cast<frame&>(*this).data(); }
	virtual unsigned char* data() = 0;
	virtual size_t size() const = 0;
	virtual void* tag() const { return nullptr; }
	virtual const std::vector<audio_chunk_ptr>& audio_data() const { return audioData_; }	
	virtual std::vector<audio_chunk_ptr>& audio_data() { return audioData_; }			

	static std::shared_ptr<frame> null()
	{
		class null_frame : public frame
		{
			unsigned char* data() { return nullptr; };
			size_t size() const { return 0; };
		};
		static auto my_null_frame = std::make_shared<null_frame>();
		return my_null_frame;
	}
private:	
	std::vector<audio_chunk_ptr> audioData_;
};
typedef std::shared_ptr<frame> frame_ptr;
typedef std::shared_ptr<const frame> frame_const_ptr;
typedef std::unique_ptr<frame> frame_uptr;
typedef std::unique_ptr<const frame> frame_const_uptr;

inline bool operator==(const frame& lhs, const frame& rhs)
{
	return lhs.data() == rhs.data() && (lhs.data() == nullptr || lhs.size() == rhs.size());
}

template<typename frame_ptr_type>
frame_ptr_type& set_frame_volume(frame_ptr_type& result_frame, float volume)
{
	assert(result_frame != nullptr);
	assert(boost::range::find(result_frame->audio_data(), nullptr) == result_frame->audio_data().end());
	boost::range::for_each(result_frame->audio_data(), std::bind(&audio_chunk::set_volume, std::placeholders::_1, volume));
	return result_frame;
}

template<typename frame_ptr_type>
frame_ptr_type& clear_frame(frame_ptr_type& result_frame)
{
	assert(result_frame != nullptr);
	common::image::clear(result_frame->data(), result_frame->size());
	result_frame->audio_data().clear();
	return result_frame;
}

template<typename frame_ptr_type>
frame_ptr_type& pre_over_frame(frame_ptr_type& result_frame, const frame_const_ptr& frame1, const frame_const_ptr& frame2)
{
	assert(result_frame != nullptr && frame1 != nullptr && frame2 != nullptr);
	assert(result_frame->size() == frame1->size());
	assert(result_frame->size() == frame2->size());
	assert(boost::range::find(frame1->audio_data(), nullptr) == frame1->audio_data().end());
	assert(boost::range::find(frame2->audio_data(), nullptr) == frame2->audio_data().end());
	tbb::parallel_invoke(
	[&]{common::image::pre_over(result_frame->data(), frame1->data(), frame2->data(), result_frame->size());},
	[&]
	{
		if(result_frame != frame1)
			boost::range::copy(frame1->audio_data(), std::back_inserter(result_frame->audio_data()));
		if(result_frame != frame2)
			boost::range::copy(frame2->audio_data(), std::back_inserter(result_frame->audio_data()));
	});
	return result_frame;
}

template<typename frame_ptr_type>
frame_ptr_type& copy_frame(frame_ptr_type& result_frame, const frame_const_ptr& frame)
{	
	assert(result_frame != nullptr && frame != nullptr);
	assert(result_frame->size() == frame->size());
	if(result_frame == frame)
		return result_frame;
	tbb::parallel_invoke(
	[&]{common::image::copy(result_frame->data(), frame->data(), result_frame->size());},
	[&]{boost::range::copy(frame->audio_data(), std::back_inserter(result_frame->audio_data()));});
	return result_frame;
} 

template<typename frame_ptr_type>
frame_ptr_type& copy_frame(frame_ptr_type& result_frame, const frame_const_ptr& frame, const frame_format_desc& format_desc)
{	
	assert(result_frame != nullptr && frame != nullptr);
	assert(result_frame->size() == format_desc.size);
	assert(frame->size() == format_desc.size);
	if(result_frame == frame)
		return result_frame;
	tbb::parallel_invoke(
	[&]
	{
		if(format_desc.mode == video_mode::progressive)
			common::image::copy(result_frame->data(), frame->data(), result_frame->size());
		else
			common::image::copy_field(result_frame->data(), frame->data(), format_desc.mode == video_mode::upper ? 1 : 0, format_desc.width, format_desc.height);
	},
	[&]{boost::range::copy(frame->audio_data(), std::back_inserter(result_frame->audio_data()));});
	return result_frame;
} 

template<typename frame_ptr_type, typename frame_container>
frame_ptr_type& compose_frames(frame_ptr_type& result_frame, const frame_container& frames)
{
	assert(boost::range::find(frames, nullptr) == frames.end());
	assert(boost::range::find_if(frames, [&](const frame_ptr& frame) { return frame->size() != result_frame->size();}) == frames.end());
	if(frames.empty())	
		clear_frame(result_frame);	
	else if(frames.size() == 1)	
		copy_frame(result_frame, frames[0]);	
	else if(frames.size() == 2)	
		pre_over_frame(result_frame, frames[0], frames[1]);	
	else
	{
		for(size_t n = 0; n < frames.size() - 2; ++n)
			pre_over_frame(frames[0], frames[n], frames[n+1]);
		pre_over_frame(result_frame, frames[0], frames[frames.size()-1]);
	}
	return result_frame;
}

}