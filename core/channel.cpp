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

#include "channel.h"

#include "video_format.h"
#include "consumer/frame_consumer_device.h"
#include "mixer/frame_mixer_device.h"
#include "producer/frame_producer_device.h"
#include "producer/layer.h"

#include <common/concurrency/executor.h>

#include <boost/range/algorithm_ext/erase.hpp>

#include <tbb/parallel_for.h>

#include <memory>

#ifdef _MSC_VER
#pragma warning(disable : 4355)
#endif

namespace caspar { namespace core {

struct channel::implementation : boost::noncopyable
{					
	const int index_;
	video_format_desc format_desc_;
	
	safe_ptr<mixer::frame_mixer_device> mixer_; // Mixer must be destroyed last in order to make sure that all frames have been returned to the pool.
	safe_ptr<frame_consumer_device> consumer_;
	safe_ptr<frame_producer_device> producer_;

	boost::signals2::scoped_connection mixer_connection_;
	boost::signals2::scoped_connection producer_connection_;

public:
	implementation(int index, const video_format_desc& format_desc)  
		: index_(index)
		, format_desc_(format_desc)
		, consumer_(new frame_consumer_device(format_desc))
		, mixer_(new mixer::frame_mixer_device(format_desc))
		, producer_(new frame_producer_device(format_desc_))	
		, mixer_connection_(mixer_->connect([=](const safe_ptr<const read_frame>& frame){consumer_->send(frame);}))
		, producer_connection_(producer_->connect([=](const std::map<int, safe_ptr<basic_frame>>& frames){mixer_->send(frames);}))
	{
		CASPAR_LOG(info) << print() << " Successfully Initialized.";
	}
		
	std::wstring print() const
	{
		return L"channel[" + boost::lexical_cast<std::wstring>(index_+1) + L"-" +  format_desc_.name + L"]";
	}

	void set_video_format_desc(const video_format_desc& format_desc)
	{
		format_desc_ = format_desc;
		producer_connection_.disconnect();
		mixer_connection_.disconnect();

		consumer_->set_video_format_desc(format_desc_);
		mixer_ = make_safe<mixer::frame_mixer_device>(format_desc_);
		producer_ = make_safe<frame_producer_device>(format_desc_);

		mixer_connection_ = mixer_->connect([=](const safe_ptr<const read_frame>& frame){consumer_->send(frame);});
		producer_connection_ = producer_->connect([=](const std::map<int, safe_ptr<basic_frame>>& frames){mixer_->send(frames);});
	}
};

channel::channel(int index, const video_format_desc& format_desc) : impl_(new implementation(index, format_desc)){}
channel::channel(channel&& other) : impl_(std::move(other.impl_)){}
const safe_ptr<frame_producer_device>& channel::producer() { return impl_->producer_;} 
const safe_ptr<mixer::frame_mixer_device>& channel::mixer() { return impl_->mixer_;} 
const safe_ptr<frame_consumer_device>& channel::consumer() { return impl_->consumer_;} 
const video_format_desc& channel::get_video_format_desc() const{return impl_->format_desc_;}
void channel::set_video_format_desc(const video_format_desc& format_desc){impl_->set_video_format_desc(format_desc);}
std::wstring channel::print() const { return impl_->print();}

}}