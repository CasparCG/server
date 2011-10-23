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

#include "video_format.h"

#include "consumer/output.h"
#include "mixer/mixer.h"
#include "producer/stage.h"

#include <common/concurrency/executor.h>
#include <common/diagnostics/graph.h>

#include "mixer/gpu/ogl_device.h"

#include <agents_extras.h>
#include <semaphore.h>

#include <boost/timer.hpp>

#ifdef _MSC_VER
#pragma warning(disable : 4355)
#endif

using namespace Concurrency;

namespace caspar { namespace core {

struct video_channel::implementation : boost::noncopyable
{
	unbounded_buffer<safe_ptr<message<std::map<int, safe_ptr<basic_frame>>>>>	stage_frames_;
	unbounded_buffer<safe_ptr<message<safe_ptr<read_frame>>>>					mixer_frames_;
	
	const video_format_desc				format_desc_;
	
	safe_ptr<semaphore>					semaphore_;
	safe_ptr<caspar::core::output>		output_;
	safe_ptr<caspar::core::mixer>		mixer_;
	safe_ptr<caspar::core::stage>		stage_;

	safe_ptr<diagnostics::graph>		graph_;
	boost::timer						frame_timer_;
	boost::timer						tick_timer_;
	boost::timer						output_timer_;
	
public:
	implementation(int index, const video_format_desc& format_desc, ogl_device& ogl)  
		: graph_(diagnostics::create_graph(narrow(print()), false))
		, format_desc_(format_desc)
		, semaphore_(make_safe<semaphore>(3))
		, output_(new caspar::core::output(mixer_frames_, format_desc))
		, mixer_(new caspar::core::mixer(stage_frames_, mixer_frames_, format_desc, ogl))
		, stage_(new caspar::core::stage(stage_frames_, semaphore_))	
	{
		graph_->add_guide("produce-time", 0.5f);	
		graph_->set_color("produce-time", diagnostics::color(0.0f, 1.0f, 0.0f));
		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));	
		graph_->set_color("output-time", diagnostics::color(1.0f, 0.5f, 0.0f));
		graph_->set_color("mix-time", diagnostics::color(1.0f, 1.0f, 0.9f));
		graph_->start();

		CASPAR_LOG(info) << print() << " Successfully Initialized.";
	}
			
	std::wstring print() const
	{
		return L"video_channel";
	}
};

video_channel::video_channel(int index, const video_format_desc& format_desc, ogl_device& ogl) : impl_(new implementation(index, format_desc, ogl)){}
video_channel::video_channel(video_channel&& other) : impl_(std::move(other.impl_)){}
safe_ptr<stage> video_channel::stage() { return impl_->stage_;} 
safe_ptr<mixer> video_channel::mixer() { return impl_->mixer_;} 
safe_ptr<output> video_channel::output() { return impl_->output_;} 
video_format_desc video_channel::get_video_format_desc() const{return impl_->format_desc_;}
std::wstring video_channel::print() const { return impl_->print();}

}}