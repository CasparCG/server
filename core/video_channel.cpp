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

#include "StdAfx.h"

#include "video_channel.h"

#include "video_channel_context.h"
#include "video_format.h"

#include "consumer/output.h"
#include "mixer/mixer.h"
#include "producer/stage.h"

#include <common/concurrency/executor.h>
#include <common/diagnostics/graph.h>

#include "mixer/gpu/ogl_device.h"

#include <boost/timer.hpp>

#ifdef _MSC_VER
#pragma warning(disable : 4355)
#endif

namespace caspar { namespace core {

struct video_channel::implementation : boost::noncopyable
{
	video_channel_context			context_;

	safe_ptr<caspar::core::output>			output_;
	std::shared_ptr<caspar::core::mixer>	mixer_;
	safe_ptr<caspar::core::stage>			stage_;

	safe_ptr<diagnostics::graph>	diag_;
	boost::timer					frame_timer_;
	boost::timer					tick_timer_;
	boost::timer					output_timer_;
	
public:
	implementation(int index, const video_format_desc& format_desc, ogl_device& ogl)  
		: context_(index, ogl, format_desc)
		, diag_(diagnostics::create_graph(narrow(print())))
		, output_(new caspar::core::output(context_))
		, mixer_(new caspar::core::mixer(context_))
		, stage_(new caspar::core::stage(context_))	
	{
		diag_->add_guide("produce-time", 0.5f);	
		diag_->set_color("produce-time", diagnostics::color(0.0f, 1.0f, 0.0f));
		diag_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));	
		diag_->set_color("output-time", diagnostics::color(1.0f, 0.5f, 0.0f));
		diag_->set_color("mix-time", diagnostics::color(1.0f, 1.0f, 0.9f));

		CASPAR_LOG(info) << print() << " Successfully Initialized.";
		context_.execution().begin_invoke([this]{tick();});
	}

	~implementation()
	{
		// Stop context before destroying devices.
		context_.execution().stop();
		context_.execution().join();
		context_.destruction().stop();
		context_.destruction().join();
	}

	void tick()
	{
		try
		{
			// Produce

			frame_timer_.restart();

			auto simple_frames = stage_->execute();

			diag_->update_value("produce-time", frame_timer_.elapsed()*context_.get_format_desc().fps*0.5);
		
			// Mix

			frame_timer_.restart();

			auto finished_frame = mixer_->execute(simple_frames);
		
			diag_->update_value("mix-time", frame_timer_.elapsed()*context_.get_format_desc().fps*0.5);
		
			// Consume
		
			output_timer_.restart();

			output_->execute(finished_frame);
		
			diag_->update_value("output-time", frame_timer_.elapsed()*context_.get_format_desc().fps*0.5);

		
			diag_->update_value("tick-time", tick_timer_.elapsed()*context_.get_format_desc().fps*0.5);
			tick_timer_.restart();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			CASPAR_LOG(error) << context_.print() << L" Unexpected exception. Clearing stage and freeing memory";

			stage_->clear();
			context_.ogl().gc().wait();

			mixer_ = nullptr;
			mixer_.reset(new caspar::core::mixer(context_));
		}

		context_.execution().begin_invoke([this]{tick();});
	}
		
	std::wstring print() const
	{
		return context_.print();
	}

	void set_video_format_desc(const video_format_desc& format_desc)
	{
		context_.execution().begin_invoke([=]
		{
			stage_->clear();
			context_.ogl().gc().wait();
			context_.set_format_desc(format_desc);
		});
	}
};

video_channel::video_channel(int index, const video_format_desc& format_desc, ogl_device& ogl) : impl_(new implementation(index, format_desc, ogl)){}
video_channel::video_channel(video_channel&& other) : impl_(std::move(other.impl_)){}
safe_ptr<stage> video_channel::stage() { return impl_->stage_;} 
safe_ptr<mixer> video_channel::mixer() { return make_safe(impl_->mixer_);} 
safe_ptr<output> video_channel::output() { return impl_->output_;} 
video_format_desc video_channel::get_video_format_desc() const{return impl_->context_.get_format_desc();}
void video_channel::set_video_format_desc(const video_format_desc& format_desc){impl_->set_video_format_desc(format_desc);}
std::wstring video_channel::print() const { return impl_->print();}
video_channel_context& video_channel::context(){return impl_->context_;}

}}