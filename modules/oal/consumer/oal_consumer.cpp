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

#include <common/exception/exceptions.h>
#include <common/diagnostics/graph.h>
#include <common/log/log.h>
#include <common/utility/timer.h>
#include <common/utility/string.h>

#include <core/consumer/frame_consumer.h>
#include <core/mixer/audio/audio_util.h>
#include <core/video_format.h>

#include <core/mixer/read_frame.h>

#include <SFML/Audio.hpp>

#include <boost/circular_buffer.hpp>
#include <boost/timer.hpp>

#include <tbb/concurrent_queue.h>

namespace caspar { namespace oal {

struct oal_consumer : public core::frame_consumer,  public sf::SoundStream
{
	safe_ptr<diagnostics::graph>						graph_;
	boost::timer										perf_timer_;

	tbb::concurrent_bounded_queue<std::shared_ptr<std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>>>>	input_;
	boost::circular_buffer<std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>>>			container_;
	tbb::atomic<bool>									is_running_;

	core::video_format_desc								format_desc_;
	int													preroll_count_;
public:
	oal_consumer() 
		: container_(16)
		, preroll_count_(0)
	{
		graph_->add_guide("tick-time", 0.5);
		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));	
		graph_->set_text(print());
		diagnostics::register_graph(graph_);

		is_running_ = true;
		input_.set_capacity(1);
	}

	~oal_consumer()
	{
		is_running_ = false;
		input_.try_push(std::make_shared<std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>>>());
		input_.try_push(std::make_shared<std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>>>());
		Stop();
		input_.try_push(std::make_shared<std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>>>());
		input_.try_push(std::make_shared<std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>>>());
		CASPAR_LOG(info) << print() << L" Shutting down.";	
	}

	virtual void initialize(const core::video_format_desc& format_desc)
	{
		format_desc_ = format_desc;		
		if(Status() != Playing)
		{
			sf::SoundStream::Initialize(2, 48000);
			Play();		
		}
		CASPAR_LOG(info) << print() << " Sucessfully initialized.";
	}
	
	virtual bool send(const safe_ptr<core::read_frame>& frame)
	{			
		input_.push(std::make_shared<std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>>>(core::audio_32_to_16_sse(frame->audio_data())));

		return true;
	}
	
	virtual bool OnGetData(sf::SoundStream::Chunk& data)
	{		
		std::shared_ptr<std::vector<int16_t, tbb::cache_aligned_allocator<int16_t>>> audio_data;		
		input_.pop(audio_data);
				
		container_.push_back(std::move(*audio_data));
		data.Samples = container_.back().data();
		data.NbSamples = container_.back().size();	
		
		graph_->update_value("tick-time", perf_timer_.elapsed()*format_desc_.fps*0.5);		
		perf_timer_.restart();

		return is_running_;
	}

	virtual std::wstring print() const
	{
		return L"oal[" + format_desc_.name + L"]";
	}

	virtual const core::video_format_desc& get_video_format_desc() const
	{
		return format_desc_;
	}

	virtual size_t buffer_depth() const
	{
		return 2;
	}
};

safe_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>& params)
{
	if(params.size() < 1 || params[0] != L"AUDIO")
		return core::frame_consumer::empty();

	return make_safe<oal_consumer>();
}

safe_ptr<core::frame_consumer> create_consumer()
{
	return make_safe<oal_consumer>();
}

}}
