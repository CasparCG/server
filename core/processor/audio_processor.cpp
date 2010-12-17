#include "../stdafx.h"

#include "audio_processor.h"

namespace caspar { namespace core {
	
audio_transform& audio_transform::operator*=(const audio_transform &other) 
{
	volume *= other.volume;
	return *this;
}

const audio_transform audio_transform::operator*(const audio_transform &other) const 
{
	return audio_transform(*this) *= other;
}

struct audio_processor::implementation
{
	implementation()
	{
		transform_stack_.push(audio_transform());
	}

	void begin(const audio_transform& transform)
	{
		transform_stack_.push(transform_stack_.top()*transform);
	}

	void process(const std::vector<short>& audio_data)
	{		
		if(audio_data_.empty())
			audio_data_.resize(audio_data.size(), 0);

		tbb::parallel_for
		(
			tbb::blocked_range<size_t>(0, audio_data.size()),
			[&](const tbb::blocked_range<size_t>& r)
			{
				for(size_t n = r.begin(); n < r.end(); ++n)
				{
					int sample = static_cast<int>(audio_data[n]);
					sample = (static_cast<int>(transform_stack_.top().volume*8192.0)*sample)/8192;
					audio_data_[n] = static_cast<short>((static_cast<int>(audio_data_[n]) + sample) & 0xFFFF);
				}
			}
		);
	}

	void end()
	{
		transform_stack_.pop();
	}

	std::vector<short> begin_pass()
	{
		return std::move(audio_data_);
	}

	std::vector<short> audio_data_;
	std::stack<audio_transform> transform_stack_;
};

audio_processor::audio_processor() : impl_(new implementation()){}
void audio_processor::begin(const audio_transform& transform){impl_->begin(transform);}
void audio_processor::process(const std::vector<short>& audio_data){impl_->process(audio_data);}
void audio_processor::end(){impl_->end();}
std::vector<short> audio_processor::begin_pass(){return impl_->begin_pass();}	
void audio_processor::end_pass(){}

}}