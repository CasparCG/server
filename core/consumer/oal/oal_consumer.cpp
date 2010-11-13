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
 
#include "../../StdAfx.h"

#include "oal_consumer.h"

#include "../../processor/frame.h"
#include "../../format/video_format.h"

#include <SFML/Audio.hpp>

#include <boost/circular_buffer.hpp>

namespace caspar { namespace core { namespace oal {	

struct consumer::implementation : public sf::SoundStream, boost::noncopyable
{
	implementation() : container_(5), underrun_count_(0)
	{
		input_.set_capacity(2);
		sf::SoundStream::Initialize(2, 48000);
	}

	~implementation()
	{
		input_.clear();
		Stop();
	}
	
	void push(const frame_ptr& frame)
	{
		// NOTE: tbb::concurrent_queue does not have rvalue support. 
		// Use shared_ptr to emulate move semantics
		input_.push(std::make_shared<std::vector<short>>(std::move(frame->audio_data()))); 
		
		if(GetStatus() != Playing && input_.size() > 1)
			Play();
	}
	
	bool OnStart() 
	{
		input_.clear();
		return true;
	}

	bool OnGetData(sf::SoundStream::Chunk& data)
	{
		static std::vector<short> silence(1920*2, 0);
		
		std::shared_ptr<std::vector<short>> audio_data;
		
		if(!input_.try_pop(audio_data))
		{
			if(underrun_count_ == 0)
				CASPAR_LOG(warning) << "### Sound Input underflow has STARTED.";
			++underrun_count_;
			input_.pop(audio_data);
		}
		else if(underrun_count_ > 0)
		{
			CASPAR_LOG(trace) << "### Sound Input Underrun has ENDED with " << underrun_count_ << " ticks.";
			underrun_count_ = 0;
		}
			
		if(audio_data->empty())
		{	
			data.Samples = silence.data();
			data.NbSamples = silence.size();
		}
		else
		{
			container_.push_back(std::move(*audio_data));
			data.Samples = container_.back().data();
			data.NbSamples = container_.back().size();
		}
		return true;
	}

	long underrun_count_;
	boost::circular_buffer<std::vector<short>> container_;
	tbb::concurrent_bounded_queue<std::shared_ptr<std::vector<short>>> input_;
};

consumer::consumer(const video_format_desc&) : impl_(new implementation()){}
void consumer::prepare(const frame_ptr& frame){impl_->push(frame);}
}}}
