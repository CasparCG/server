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
	implementation() : container_(5)
	{
		sf::SoundStream::Initialize(2, 48000);
		tickets_.set_capacity(3);
	}

	~implementation()
	{
		Stop();
	}
	
	void send(const safe_ptr<const read_frame>& frame)
	{				
		input_.push(frame->audio_data()); 

		if(GetStatus() != Playing && input_.size() > 2)		
			Play();		
	}

	frame_consumer::sync_mode synchronize()
	{
		tickets_.push(true);
		return frame_consumer::clock;
	}

	size_t buffer_depth() const
	{
		return 3;
	}
	
	bool OnStart() 
	{
		input_.clear();
		return true;
	}

	bool OnGetData(sf::SoundStream::Chunk& data)
	{
		static std::vector<short> silence(1920*2, 0);
		
		bool dummy;
		tickets_.pop(dummy);

		std::vector<short> audio_data;		
		input_.pop(audio_data);
			
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

	tbb::concurrent_bounded_queue<bool> tickets_;
	tbb::concurrent_bounded_queue<std::vector<short>> input_;
	boost::circular_buffer<std::vector<short>> container_;
};

consumer::consumer(consumer&& other) : impl_(std::move(other.impl_)){}
consumer::consumer(const video_format_desc&) : impl_(new implementation()){}
void consumer::send(const safe_ptr<const read_frame>& frame){impl_->send(frame);}
frame_consumer::sync_mode consumer::synchronize(){return impl_->synchronize();}
size_t consumer::buffer_depth() const{return impl_->buffer_depth();}
}}}
