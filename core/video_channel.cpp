/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "StdAfx.h"

#include "video_channel.h"

#include "video_format.h"

#include "consumer/output.h"
#include "mixer/mixer.h"
#include "mixer/gpu/write_frame.h"
#include "mixer/gpu/accelerator.h"
#include "frame/data_frame.h"
#include "producer/stage.h"
#include "frame/frame_factory.h"

#include <common/diagnostics/graph.h>
#include <common/env.h>
#include <common/concurrency/lock.h>

#include <tbb/spin_mutex.h>

#include <boost/property_tree/ptree.hpp>

#include <string>

namespace caspar { namespace core {

struct video_channel::impl sealed : public frame_factory
{
	reactive::basic_subject<safe_ptr<const data_frame>> frame_subject_;
	const int								index_;

	mutable tbb::spin_mutex					format_desc_mutex_;
	video_format_desc						format_desc_;
	
	const safe_ptr<gpu::accelerator>		ogl_;
	const safe_ptr<diagnostics::graph>		graph_;

	const safe_ptr<caspar::core::output>	output_;
	const safe_ptr<caspar::core::mixer>		mixer_;
	const safe_ptr<caspar::core::stage>		stage_;	

	boost::timer							tick_timer_;
	boost::timer							produce_timer_;
	boost::timer							mix_timer_;
	boost::timer							consume_timer_;

	executor								executor_;
public:
	impl(int index, const video_format_desc& format_desc, const safe_ptr<gpu::accelerator>& ogl)  
		:  index_(index)
		, format_desc_(format_desc)
		, ogl_(ogl)
		, output_(new caspar::core::output(format_desc, index))
		, mixer_(new caspar::core::mixer(ogl))
		, stage_(new caspar::core::stage())	
		, executor_(L"video_channel")
	{
		//graph_->set_color("mix-time", diagnostics::color(1.0f, 0.0f, 0.9f, 0.8));
		graph_->set_color("produce-time", diagnostics::color(0.0f, 1.0f, 0.0f));
		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));	
		graph_->set_color("consume-time", diagnostics::color(1.0f, 0.4f, 0.0f, 0.8));
		graph_->set_text(print());
		diagnostics::register_graph(graph_);

		executor_.begin_invoke([=]{tick();});

		CASPAR_LOG(info) << print() << " Successfully Initialized.";
	}
	
	// frame_factory
						
	virtual safe_ptr<write_frame> create_frame(const void* tag, const core::pixel_format_desc& desc) override
	{		
		return make_safe<write_frame>(ogl_, tag, desc);
	}
	
	virtual core::video_format_desc get_video_format_desc() const override
	{
		return lock(format_desc_mutex_, [&]
		{
			return format_desc_;
		});
	}
	
	// video_channel

	void tick()
	{
		tick_timer_.restart();

		// Produce

		produce_timer_.restart();

		auto stage_frames = (*stage_)(format_desc_);
		
		graph_->set_value("produce-time", produce_timer_.elapsed()*format_desc_.fps*0.5);

		// Mix

		//mix_timer_.restart();

		auto mixed_frame  = (*mixer_)(std::move(stage_frames), format_desc_);
		
		//graph_->set_value("mix-time", mix_timer_.elapsed()*format_desc_.fps*0.5);

		// Consume

		consume_timer_.restart();

		frame_subject_.on_next(mixed_frame);

		graph_->set_value("consume-time", consume_timer_.elapsed()*format_desc_.fps*0.5);

		(*output_)(std::move(mixed_frame), format_desc_);
		
		graph_->set_value("tick-time", tick_timer_.elapsed()*format_desc_.fps*0.5);

		executor_.begin_invoke([=]{tick();});
	}

	void set_video_format_desc(const video_format_desc& format_desc)
	{
		ogl_->gc();
		lock(format_desc_mutex_, [&]
		{
			format_desc_ = format_desc;
		});
	}
		
	std::wstring print() const
	{
		return L"video_channel[" + boost::lexical_cast<std::wstring>(index_) + L"|" +  format_desc_.name + L"]";
	}

	boost::property_tree::wptree info() const
	{
		boost::property_tree::wptree info;

		auto stage_info  = stage_->info();
		auto mixer_info  = mixer_->info();
		auto output_info = output_->info();

		stage_info.timed_wait(boost::posix_time::seconds(2));
		mixer_info.timed_wait(boost::posix_time::seconds(2));
		output_info.timed_wait(boost::posix_time::seconds(2));
		
		info.add(L"video-mode", format_desc_.name);
		info.add_child(L"stage", stage_info.get());
		info.add_child(L"mixer", mixer_info.get());
		info.add_child(L"output", output_info.get());
   
		return info;			   
	}
};

video_channel::video_channel(int index, const video_format_desc& format_desc, const safe_ptr<gpu::accelerator>& ogl) : impl_(new impl(index, format_desc, ogl)){}
safe_ptr<stage> video_channel::stage() { return impl_->stage_;} 
safe_ptr<mixer> video_channel::mixer() { return impl_->mixer_;} 
safe_ptr<frame_factory> video_channel::frame_factory() { return impl_;} 
safe_ptr<output> video_channel::output() { return impl_->output_;} 
video_format_desc video_channel::get_video_format_desc() const{return impl_->format_desc_;}
void video_channel::set_video_format_desc(const video_format_desc& format_desc){impl_->set_video_format_desc(format_desc);}
boost::property_tree::wptree video_channel::info() const{return impl_->info();}
void video_channel::subscribe(const observer_ptr& o) {impl_->frame_subject_.subscribe(o);}
void video_channel::unsubscribe(const observer_ptr& o) {impl_->frame_subject_.unsubscribe(o);}

}}