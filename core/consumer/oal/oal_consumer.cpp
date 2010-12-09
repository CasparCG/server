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

#include "../../processor/write_frame.h"
#include "../../format/video_format.h"

#include <SFML/Audio.hpp>

#include <boost/circular_buffer.hpp>

namespace caspar { namespace core { namespace oal {	

struct consumer::implementation : public sf::SoundStream, boost::noncopyable
{
	implementation() : container_(5), underrun_count_(0)
	{
		sf::SoundStream::Initialize(2, 48000);
	}

	~implementation()
	{
		Stop();
	}
	
	boost::unique_future<void> push(const consumer_frame& frame)
	{
		input_.push(frame.audio_data()); 

		auto promise = std::make_shared<boost::promise<void>>();
		
		if(GetStatus() != Playing && input_.size() > 2)
			Play();
		
		if(GetStatus() == Playing)
			promises_.push(promise);
		else
			promise->set_value();

		return promise->get_future();
	}
	
	bool OnStart() 
	{
		input_.clear();
		return true;
	}

	bool OnGetData(sf::SoundStream::Chunk& data)
	{
		static std::vector<short> silence(1920*2, 0);
		
		std::shared_ptr<boost::promise<void>> promise;
		promises_.pop(promise);
		promise->set_value();

		std::vector<short> audio_data;		
		if(!input_.try_pop(audio_data))
		{
			if(underrun_count_ == 0)
				CASPAR_LOG(trace) << "### Sound Input underflow has STARTED.";
			++underrun_count_;
			input_.pop(audio_data);
		}
		else if(underrun_count_ > 0)
		{
			CASPAR_LOG(trace) << "### Sound Input Underrun has ENDED with " << underrun_count_ << " ticks.";
			underrun_count_ = 0;
		}
			
		if(audio_data.empty())
		{	
			data.Samples = silence.data();
			data.NbSamples = silence.size();
		}
		else
		{
			container_.push_back(std::move(audio_data));
			data.Samples = container_.back().data();
			data.NbSamples = container_.back().size();
		}

		return true;
	}

	tbb::concurrent_bounded_queue<std::shared_ptr<boost::promise<void>>> promises_;
	tbb::concurrent_bounded_queue<std::vector<short>> input_;

	long underrun_count_;
	boost::circular_buffer<std::vector<short>> container_;
};

consumer::consumer(const video_format_desc&) : impl_(new implementation()){}
boost::unique_future<void> consumer::prepare(const consumer_frame& frame){return impl_->push(frame);}
}}}
