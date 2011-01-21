#include "../stdafx.h"

#include "audio_mixer.h"
#include "audio_transform.h"

namespace caspar { namespace core {
	
struct audio_mixer::implementation
{
	std::vector<short> audio_data_;
	std::stack<audio_transform> transform_stack_;

public:
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

		double gain = transform_stack_.top().get_gain();
		tbb::parallel_for
		(
			tbb::blocked_range<size_t>(0, audio_data.size()),
			[&](const tbb::blocked_range<size_t>& r)
			{
				for(size_t n = r.begin(); n < r.end(); ++n)
				{
					int sample = static_cast<int>(audio_data[n]);
					sample = (static_cast<int>(gain*8192.0)*sample)/8192;
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
};

audio_mixer::audio_mixer() : impl_(new implementation()){}
void audio_mixer::begin(const audio_transform& transform){impl_->begin(transform);}
void audio_mixer::process(const std::vector<short>& audio_data){impl_->process(audio_data);}
void audio_mixer::end(){impl_->end();}
std::vector<short> audio_mixer::begin_pass(){return impl_->begin_pass();}	
void audio_mixer::end_pass(){}

}}