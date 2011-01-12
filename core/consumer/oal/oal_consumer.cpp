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

#include "../../video_format.h"

#include "../../processor/read_frame.h"

#include <SFML/Audio.hpp>

#include <boost/circular_buffer.hpp>

namespace caspar { namespace core { namespace oal {	

struct consumer::implementation : public sf::SoundStream, boost::noncopyable
{
	tbb::concurrent_bounded_queue<std::vector<short>> input_;
	boost::circular_buffer<std::vector<short>> container_;

public:
	implementation() 
		: container_(5)
	{
		sf::SoundStream::Initialize(2, 48000);
		Play();		
	}
	
	void send(const safe_ptr<const read_frame>& frame)
	{				
		if(frame->audio_data().empty())
			return;

		input_.push(std::vector<short>(frame->audio_data().begin(), frame->audio_data().end())); 	
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
		std::vector<short> audio_data;		
		input_.pop(audio_data);
				
		container_.push_back(std::move(audio_data));
		data.Samples = container_.back().data();
		data.NbSamples = container_.back().size();		

		return true;
	}
};

consumer::consumer(consumer&& other) : impl_(std::move(other.impl_)){}
consumer::consumer(const video_format_desc&) : impl_(new implementation()){}
void consumer::send(const safe_ptr<const read_frame>& frame){impl_->send(frame);}
size_t consumer::buffer_depth() const{return impl_->buffer_depth();}
}}}
