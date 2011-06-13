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
#include <core/producer/frame/audio_transform.h>

namespace caspar { namespace core {
	
struct audio_mixer::implementation
{
	std::deque<std::vector<int16_t>> audio_data_;
	std::stack<core::audio_transform> transform_stack_;

	std::map<int, core::audio_transform> prev_audio_transforms_;
	std::map<int, core::audio_transform> next_audio_transforms_;

public:
	implementation()
	{
		transform_stack_.push(core::audio_transform());
		audio_data_.push_back(std::vector<int16_t>()); // One frame delay
	}
	
	void begin(const core::basic_frame& frame)
	{
		transform_stack_.push(transform_stack_.top()*frame.get_audio_transform());
	}

	void visit(const core::write_frame& frame)
	{
		if(!transform_stack_.top().get_has_audio())
			return;

		const auto& audio_data = frame.audio_data();
		const auto tag = frame.tag(); // Get the identifier for the audio-stream.

		if(audio_data_.back().empty())
			audio_data_.back().resize(audio_data.size(), 0);
		
		const auto next = transform_stack_.top();
		auto prev = next;

		const auto it = prev_audio_transforms_.find(tag);
		if(it != prev_audio_transforms_.end())
			prev = it->second;
				
		next_audio_transforms_[tag] = next; // Store all active tags, inactive tags will be removed in end_pass.
				
		if(next.get_gain() < 0.001 && prev.get_gain() < 0.001)
			return;
		
		static const int BASE = 1<<15;

		const auto next_gain = static_cast<int>(next.get_gain()*BASE);
		const auto prev_gain = static_cast<int>(prev.get_gain()*BASE);
		
		const int n_samples = audio_data_.back().size();

		tbb::parallel_for
		(
			tbb::blocked_range<size_t>(0, audio_data.size()),
			[&](const tbb::blocked_range<size_t>& r)
			{
				for(size_t n = r.begin(); n < r.end(); ++n)
				{
					const int sample_gain = (prev_gain - (prev_gain * n)/n_samples) + (next_gain * n)/n_samples;
					const int sample = (static_cast<int>(audio_data[n])*sample_gain)/BASE;
					audio_data_.back()[n] = static_cast<int16_t>((static_cast<int>(audio_data_.back()[n]) + sample) & 0xFFFF);
				}
			}
		);
	}

	void begin(const core::audio_transform& transform)
	{
		transform_stack_.push(transform_stack_.top()*transform);
	}
		
	void end()
	{
		transform_stack_.pop();
	}
	
	std::vector<int16_t> mix()
	{
		prev_audio_transforms_ = std::move(next_audio_transforms_);	
		auto result = std::move(audio_data_.front());
		audio_data_.pop_front();
		audio_data_.push_back(std::vector<int16_t>());
		return std::move(result);
	}
};

audio_mixer::audio_mixer() : impl_(new implementation()){}
void audio_mixer::begin(const core::basic_frame& frame){impl_->begin(frame);}
void audio_mixer::visit(core::write_frame& frame){impl_->visit(frame);}
void audio_mixer::end(){impl_->end();}
std::vector<int16_t> audio_mixer::mix(){return impl_->mix();}
audio_mixer& audio_mixer::operator=(audio_mixer&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}

}}