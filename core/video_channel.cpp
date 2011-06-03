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
#include "producer/layer.h"

#include <common/concurrency/executor.h>

#include <boost/range/algorithm_ext/erase.hpp>

#ifdef _MSC_VER
#pragma warning(disable : 4355)
#endif

namespace caspar { namespace core {

struct video_channel::implementation : boost::noncopyable
{
	video_channel_context			context_;

	safe_ptr<frame_consumer_device>	consumer_;
	safe_ptr<frame_mixer_device>	mixer_;
	safe_ptr<frame_producer_device>	producer_;
	
public:
	implementation(int index, const video_format_desc& format_desc, ogl_device& ogl)  
		: context_(index, ogl, format_desc)
		, consumer_(new frame_consumer_device(context_))
		, mixer_(new frame_mixer_device(context_))
		, producer_(new frame_producer_device(context_))	
	{
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
		auto simple_frames = producer_->execute();
		auto finished_frame = mixer_->execute(simple_frames);
		consumer_->execute(finished_frame);

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
			context_.set_format_desc(format_desc);
		});
	}
};

video_channel::video_channel(int index, const video_format_desc& format_desc, ogl_device& ogl) : impl_(new implementation(index, format_desc, ogl)){}
video_channel::video_channel(video_channel&& other) : impl_(std::move(other.impl_)){}
safe_ptr<frame_producer_device> video_channel::producer() { return impl_->producer_;} 
safe_ptr<frame_mixer_device> video_channel::mixer() { return impl_->mixer_;} 
safe_ptr<frame_consumer_device> video_channel::consumer() { return impl_->consumer_;} 
video_format_desc video_channel::get_video_format_desc() const{return impl_->context_.get_format_desc();}
void video_channel::set_video_format_desc(const video_format_desc& format_desc){impl_->set_video_format_desc(format_desc);}
std::wstring video_channel::print() const { return impl_->print();}

}}