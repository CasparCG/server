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
 
#include "oal_consumer.h"

#include <common/diagnostics/graph.h>
#include <common/log/log.h>
#include <common/utility/timer.h>

#include <core/video_format.h>

#include <core/mixer/read_frame.h>

#include <SFML/Audio.hpp>

#include <boost/circular_buffer.hpp>
#include <boost/timer.hpp>

#include <tbb/concurrent_queue.h>

namespace caspar {

struct oal_consumer::implementation : public sf::SoundStream, boost::noncopyable
{
	safe_ptr<diagnostics::graph> graph_;
	boost::timer perf_timer_;

	tbb::concurrent_bounded_queue<std::vector<short>> input_;
	boost::circular_buffer<std::vector<short>> container_;
	tbb::atomic<bool> is_running_;

	core::video_format_desc format_desc_;
	int preroll_count_;
public:
	implementation(const core::video_format_desc& format_desc) 
		: graph_(diagnostics::create_graph(narrow(print())))
		, container_(5)
		, format_desc_(format_desc)
		, preroll_count_(0)
	{
		graph_->add_guide("tick-time", 0.5);
		graph_->set_color("tick-time", diagnostics::color(0.1f, 0.7f, 0.8f));
		is_running_ = true;
		input_.set_capacity(CONSUMER_BUFFER_DEPTH-2);
		
		sf::SoundStream::Initialize(2, 48000);
		Play();		
		CASPAR_LOG(info) << print() << " Sucessfully initialized.";
	}

	~implementation()
	{
		is_running_ = false;
		input_.try_push(std::vector<short>());
		input_.try_push(std::vector<short>());
		Stop();
		CASPAR_LOG(info) << print() << L" Shutting down.";	
	}
	
	void send(const safe_ptr<core::read_frame>& frame)
	{			
		if(preroll_count_ < input_.capacity())
		{
			while(input_.try_push(std::vector<short>(format_desc_.audio_samples_per_frame, 0)))
				++preroll_count_;
		}

		if(!frame->audio_data().empty())
			input_.push(std::vector<short>(frame->audio_data().begin(), frame->audio_data().end())); 	
		else
			input_.push(std::vector<short>(format_desc_.audio_samples_per_frame, 0)); //static_cast<size_t>(48000.0f/format_desc_.fps)*2
	}

	size_t buffer_depth() const{return 3;}

	virtual bool OnGetData(sf::SoundStream::Chunk& data)
	{		
		std::vector<short> audio_data;		
		input_.pop(audio_data);
				
		container_.push_back(std::move(audio_data));
		data.Samples = container_.back().data();
		data.NbSamples = container_.back().size();	
		
		graph_->update_value("tick-time", perf_timer_.elapsed()*format_desc_.fps*0.5);		
		perf_timer_.restart();

		return is_running_;
	}

	std::wstring print() const
	{
		return L"oal[" + format_desc_.name + L"]";
	}
};

oal_consumer::oal_consumer(){}
oal_consumer::oal_consumer(oal_consumer&& other) : impl_(std::move(other.impl_)){}
void oal_consumer::send(const safe_ptr<core::read_frame>& frame){impl_->send(frame);}
size_t oal_consumer::buffer_depth() const{return impl_->buffer_depth();}
void oal_consumer::initialize(const core::video_format_desc& format_desc){impl_.reset(new implementation(format_desc));}
std::wstring oal_consumer::print() const { return impl_->print(); }
const core::video_format_desc& oal_consumer::get_video_format_desc() const{return impl_->format_desc_;}

safe_ptr<core::frame_consumer> create_oal_consumer(const std::vector<std::wstring>& params)
{
	if(params.size() < 1 || params[0] != L"OAL")
		return core::frame_consumer::empty();

	return make_safe<oal_consumer>();
}
}
