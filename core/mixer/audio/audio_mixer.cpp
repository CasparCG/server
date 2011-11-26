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
#include "../../stdafx.h"

#include "audio_mixer.h"

#include <core/mixer/write_frame.h>
#include <core/producer/frame/frame_transform.h>

#include <stack>
#include <map>
#include <vector>

namespace caspar { namespace core {

struct audio_item
{
	const void*			tag;
	frame_transform		transform;
	audio_buffer		audio_data;
};
	
struct audio_mixer::implementation
{
	std::stack<core::frame_transform>				transform_stack_;
	std::map<const void*, core::frame_transform>	prev_frame_transforms_;
	std::vector<audio_item>							items_;

public:
	implementation()
	{
		transform_stack_.push(core::frame_transform());
	}
	
	void begin(core::basic_frame& frame)
	{
		transform_stack_.push(transform_stack_.top()*frame.get_frame_transform());
	}

	void visit(core::write_frame& frame)
	{
		if(transform_stack_.top().volume < 0.002 || frame.audio_data().empty())
			return;

		audio_item item;
		item.tag		= frame.tag();
		item.transform	= transform_stack_.top();
		item.audio_data = std::move(frame.audio_data()); // Note: We don't need to care about upper/lower since audio_data is removed/moved from the last field.

		items_.push_back(item);		
	}

	void begin(const core::frame_transform& transform)
	{
		transform_stack_.push(transform_stack_.top()*transform);
	}
		
	void end()
	{
		transform_stack_.pop();
	}
	
	audio_buffer mix(const video_format_desc& format_desc)
	{	
		//CASPAR_ASSERT(format_desc.audio_channels == 2);
		//CASPAR_ASSERT(format_desc.audio_samples_per_frame % 4 == 0);

		// NOTE: auto data should be larger than format_desc_.audio_samples_per_frame to allow sse to read/write beyond size.

		auto intermediate	= std::vector<float, tbb::cache_aligned_allocator<float>>(format_desc.audio_samples_per_frame+128, 0.0f);
		auto result			= audio_buffer(format_desc.audio_samples_per_frame+128);	
		auto result_128		= reinterpret_cast<__m128i*>(result.data());

		std::map<const void*, core::frame_transform> next_frame_transforms;
				
		BOOST_FOREACH(auto& item, items_)
		{						
			auto next = item.transform;
			auto prev = next;

			const auto it = prev_frame_transforms_.find(item.tag);
			if(it != prev_frame_transforms_.end())
				prev = it->second;
				
			next_frame_transforms[item.tag] = next; // Store all active tags, inactive tags will be removed at the end.
					
			if(prev.volume < 0.001 && next.volume < 0.001)
				continue;

			if(item.audio_data.size() != format_desc.audio_samples_per_frame)
				continue;

			const float prev_volume = static_cast<float>(prev.volume);
			const float next_volume = static_cast<float>(next.volume);
									
			auto alpha		= (next_volume-prev_volume)/static_cast<float>(format_desc.audio_samples_per_frame/format_desc.audio_channels);
			auto alpha_ps	= _mm_set_ps1(alpha*2.0f);
			auto volume_ps	= _mm_setr_ps(prev_volume, prev_volume, prev_volume+alpha, prev_volume+alpha);

			if(&item != &items_.back())
			{
				for(size_t n = 0; n < format_desc.audio_samples_per_frame/4; ++n)
				{		
					auto sample_ps		= _mm_cvtepi32_ps(_mm_load_si128(reinterpret_cast<__m128i*>(&item.audio_data[n*4])));
					auto res_sample_ps	= _mm_load_ps(&intermediate[n*4]);											
					sample_ps			= _mm_mul_ps(sample_ps, volume_ps);	
					res_sample_ps		= _mm_add_ps(sample_ps, res_sample_ps);	

					volume_ps			= _mm_add_ps(volume_ps, alpha_ps);

					_mm_store_ps(&intermediate[n*4], res_sample_ps);
				}
			}
			else
			{
				for(size_t n = 0; n < format_desc.audio_samples_per_frame/4; ++n)
				{		
					auto sample_ps		= _mm_cvtepi32_ps(_mm_load_si128(reinterpret_cast<__m128i*>(&item.audio_data[n*4])));
					auto res_sample_ps	= _mm_load_ps(&intermediate[n*4]);											
					sample_ps			= _mm_mul_ps(sample_ps, volume_ps);	
					res_sample_ps		= _mm_add_ps(sample_ps, res_sample_ps);	
					
					_mm_stream_si128(result_128++, _mm_cvtps_epi32(res_sample_ps));
				}
			}
		}				

		items_.clear();
		prev_frame_transforms_ = std::move(next_frame_transforms);	

		result.resize(format_desc.audio_samples_per_frame);
		return std::move(result);
	}
};

audio_mixer::audio_mixer() : impl_(new implementation()){}
void audio_mixer::begin(core::basic_frame& frame){impl_->begin(frame);}
void audio_mixer::visit(core::write_frame& frame){impl_->visit(frame);}
void audio_mixer::end(){impl_->end();}
audio_buffer audio_mixer::mix(const video_format_desc& format_desc){return impl_->mix(format_desc);}
audio_mixer& audio_mixer::operator=(audio_mixer&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}

}}