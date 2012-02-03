#include "../stdafx.h"

#include "cadence_guard.h"

#include "frame_consumer.h"

#include <common/env.h>
#include <common/memory/safe_ptr.h>
#include <common/except.h>

#include <core/video_format.h>
#include <core/frame/data_frame.h>

namespace caspar { namespace core {

// This class is used to guarantee that audio cadence is correct. This is important for NTSC audio.
class cadence_guard : public frame_consumer
{
	safe_ptr<frame_consumer>		consumer_;
	std::vector<int>				audio_cadence_;
	boost::circular_buffer<int>	sync_buffer_;
public:
	cadence_guard(const safe_ptr<frame_consumer>& consumer)
		: consumer_(consumer)
	{
	}
	
	virtual void initialize(const video_format_desc& format_desc, int channel_index) override
	{
		audio_cadence_	= format_desc.audio_cadence;
		sync_buffer_	= boost::circular_buffer<int>(format_desc.audio_cadence.size());
		consumer_->initialize(format_desc, channel_index);
	}

	virtual bool send(const safe_ptr<const data_frame>& frame) override
	{		
		if(audio_cadence_.size() == 1)
			return consumer_->send(frame);

		bool result = true;
		
		if(boost::range::equal(sync_buffer_, audio_cadence_) && audio_cadence_.front() == static_cast<int>(frame->audio_data().size())) 
		{	
			// Audio sent so far is in sync, now we can send the next chunk.
			result = consumer_->send(frame);
			boost::range::rotate(audio_cadence_, std::begin(audio_cadence_)+1);
		}
		else
			CASPAR_LOG(trace) << print() << L" Syncing audio.";

		sync_buffer_.push_back(static_cast<int>(frame->audio_data().size()));
		
		return result;
	}

	virtual std::wstring print() const override
	{
		return consumer_->print();
	}

	virtual boost::property_tree::wptree info() const override
	{
		return consumer_->info();
	}

	virtual bool has_synchronization_clock() const override
	{
		return consumer_->has_synchronization_clock();
	}

	virtual int buffer_depth() const override
	{
		return consumer_->buffer_depth();
	}

	virtual int index() const override
	{
		return consumer_->index();
	}
};

safe_ptr<frame_consumer> create_consumer_cadence_guard(const safe_ptr<frame_consumer>& consumer)
{
	return make_safe<cadence_guard>(std::move(consumer));
}

}}