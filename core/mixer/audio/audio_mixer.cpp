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

#include <tbb/parallel_for.h>

#include <safeint.h>

#include <stack>
#include <deque>

namespace caspar { namespace core {

struct audio_item
{
	const void*				tag;
	frame_transform			transform;
	std::vector<int32_t>	audio_data;
};
	
struct audio_mixer::implementation
{
	std::stack<core::frame_transform>				transform_stack_;
	std::map<const void*, core::frame_transform>	prev_frame_transforms_;
	const core::video_format_desc					format_desc_;
	std::vector<audio_item>							items;

public:
	implementation(const core::video_format_desc& format_desc)
		: format_desc_(format_desc)
	{
		transform_stack_.push(core::frame_transform());
	}
	
	void begin(core::basic_frame& frame)
	{
		transform_stack_.push(transform_stack_.top()*frame.get_frame_transform());
	}

	void visit(const core::write_frame& frame)
	{
		// We only care about the last field.
		if(format_desc_.field_mode == field_mode::upper && transform_stack_.top().field_mode == field_mode::upper)
			return;

		if(format_desc_.field_mode == field_mode::lower && transform_stack_.top().field_mode == field_mode::lower)
			return;

		// Skip empty audio.
		if(transform_stack_.top().volume < 0.002 || frame.audio_data().empty())
			return;

		audio_item item;
		item.tag		= frame.tag();
		item.transform	= transform_stack_.top();
		item.audio_data = std::vector<int32_t>(frame.audio_data().begin(), frame.audio_data().end());

		items.push_back(item);		
	}

	void begin(const core::frame_transform& transform)
	{
		transform_stack_.push(transform_stack_.top()*transform);
	}
		
	void end()
	{
		transform_stack_.pop();
	}
	
	std::vector<int32_t> mix()
	{
		auto result = std::vector<int32_t>(format_desc_.audio_samples_per_frame);

		std::map<const void*, core::frame_transform> next_frame_transforms;

		BOOST_FOREACH(auto& item, items)
		{				
			const auto next = item.transform;
			auto prev = next;

			const auto it = prev_frame_transforms_.find(item.tag);
			if(it != prev_frame_transforms_.end())
				prev = it->second;
				
			next_frame_transforms[item.tag] = next; // Store all active tags, inactive tags will be removed at the end.
				
			if(next.volume < 0.001 && prev.volume < 0.001)
				continue;
		
			static const int BASE = 1<<31;

			const auto next_volume = static_cast<int64_t>(next.volume*BASE);
			const auto prev_volume = static_cast<int64_t>(prev.volume*BASE);
		
			const int n_samples = result.size();
		
			const auto in_size = static_cast<size_t>(item.audio_data.size());
			CASPAR_VERIFY(in_size == 0 || in_size == result.size());

			if(in_size > result.size())
				continue;

			tbb::parallel_for
			(
				tbb::blocked_range<size_t>(0, item.audio_data.size()),
				[&](const tbb::blocked_range<size_t>& r)
				{
					for(size_t n = r.begin(); n < r.end(); ++n)
					{
						const auto sample_volume = (prev_volume - (prev_volume * n)/n_samples) + (next_volume * n)/n_samples;
						const auto sample = static_cast<int32_t>((static_cast<int64_t>(item.audio_data[n])*sample_volume)/BASE);
						result[n] = result[n] + sample;
					}
				}
			);
		}

		items.clear();
		prev_frame_transforms_ = std::move(next_frame_transforms);	

		return std::move(result);
	}
};

audio_mixer::audio_mixer(const core::video_format_desc& format_desc) : impl_(new implementation(format_desc)){}
void audio_mixer::begin(core::basic_frame& frame){impl_->begin(frame);}
void audio_mixer::visit(core::write_frame& frame){impl_->visit(frame);}
void audio_mixer::end(){impl_->end();}
std::vector<int32_t> audio_mixer::mix(){return impl_->mix();}
audio_mixer& audio_mixer::operator=(audio_mixer&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}

}}