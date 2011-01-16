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

namespace caspar { namespace core {

struct oal_consumer::implementation : public sf::SoundStream, boost::noncopyable
{
	tbb::concurrent_bounded_queue<std::vector<short>> input_;
	boost::circular_buffer<std::vector<short>> container_;
	tbb::atomic<bool> is_running_;
public:
	implementation() 
		: container_(5)
	{
		is_running_ = true;
		sf::SoundStream::Initialize(2, 48000);
		Play();		
		CASPAR_LOG(info) << "Sucessfully started oal_consumer";
	}

	~implementation()
	{
		is_running_ = false;
		input_.try_push(std::vector<short>());
		input_.try_push(std::vector<short>());
		CASPAR_LOG(info) << "Sucessfully ended oal_consumer";
	}
	
	virtual void send(const safe_ptr<const read_frame>& frame)
	{				
		if(frame->audio_data().empty())
			return;

		input_.push(std::vector<short>(frame->audio_data().begin(), frame->audio_data().end())); 	
	}

	virtual size_t buffer_depth() const{return 3;}

	virtual bool OnGetData(sf::SoundStream::Chunk& data)
	{		
		std::vector<short> audio_data;		
		input_.pop(audio_data);
				
		container_.push_back(std::move(audio_data));
		data.Samples = container_.back().data();
		data.NbSamples = container_.back().size();		

		return is_running_;
	}
};

oal_consumer::oal_consumer(oal_consumer&& other) : impl_(std::move(other.impl_)){}
oal_consumer::oal_consumer(const video_format_desc&) : impl_(new implementation()){}
void oal_consumer::send(const safe_ptr<const read_frame>& frame){impl_->send(frame);}
size_t oal_consumer::buffer_depth() const{return impl_->buffer_depth();}

safe_ptr<frame_consumer> create_oal_consumer(const std::vector<std::wstring>& params)
{
	if(params.size() < 2 || params[0] != L"OAL")
		return frame_consumer::empty();

	auto format_desc = video_format_desc::get(params[1]);
	if(format_desc.format == video_format::invalid)
		return frame_consumer::empty();

	return make_safe<oal_consumer>(format_desc);
}
}}
