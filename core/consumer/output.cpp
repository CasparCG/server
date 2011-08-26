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
// TODO: Try to recover consumer from bad_alloc...
#include "../StdAfx.h"

#ifdef _MSC_VER
#pragma warning (disable : 4244)
#endif

#include "output.h"

#include "../video_channel_context.h"

#include "../video_format.h"
#include "../mixer/gpu/ogl_device.h"
#include "../mixer/read_frame.h"

#include <common/concurrency/executor.h>
#include <common/utility/assert.h>
#include <common/utility/timer.h>
#include <common/memory/memshfl.h>

#include <tbb/mutex.h>

namespace caspar { namespace core {

struct output::implementation
{	
	typedef std::pair<safe_ptr<read_frame>, safe_ptr<read_frame>> fill_and_key;
	
	video_channel_context& channel_;

	std::map<int, safe_ptr<frame_consumer>> consumers_;
	typedef std::map<int, safe_ptr<frame_consumer>>::value_type layer_t;
	
	high_prec_timer timer_;
		
public:
	implementation(video_channel_context& video_channel) 
		: channel_(video_channel){}	
	
	void add(int index, safe_ptr<frame_consumer>&& consumer)
	{		
		channel_.execution().invoke([&]
		{
			consumers_.erase(index);
		});

		consumer->initialize(channel_.get_format_desc());

		channel_.execution().invoke([&]
		{
			consumers_.insert(std::make_pair(index, consumer));

			CASPAR_LOG(info) << print() << L" " << consumer->print() << L" Added.";
		});
	}

	void remove(int index)
	{
		channel_.execution().invoke([&]
		{
			auto it = consumers_.find(index);
			if(it != consumers_.end())
			{
				CASPAR_LOG(info) << print() << L" " << it->second->print() << L" Removed.";
				consumers_.erase(it);
			}
		});
	}
						
	void execute(const safe_ptr<read_frame>& frame)
	{			
		if(!has_synchronization_clock())
			timer_.tick(1.0/channel_.get_format_desc().fps);

		if(frame->image_size() != channel_.get_format_desc().size)
		{
			timer_.tick(1.0/channel_.get_format_desc().fps);
			return;
		}
		
		auto it = consumers_.begin();
		while(it != consumers_.end())
		{
			auto consumer = it->second;

			if(consumer->get_video_format_desc() != channel_.get_format_desc())
				consumer->initialize(channel_.get_format_desc());

			if(consumer->send(frame))
				++it;
			else
				consumers_.erase(it++);
		}
	}

private:
	
	bool has_synchronization_clock()
	{
		return std::any_of(consumers_.begin(), consumers_.end(), [](const decltype(*consumers_.begin())& p)
		{
			return p.second->has_synchronization_clock();
		});
	}
	
	std::wstring print() const
	{
		return L"output";
	}
};

output::output(video_channel_context& video_channel) : impl_(new implementation(video_channel)){}
void output::add(int index, safe_ptr<frame_consumer>&& consumer){impl_->add(index, std::move(consumer));}
void output::remove(int index){impl_->remove(index);}
void output::execute(const safe_ptr<read_frame>& frame) {impl_->execute(frame); }
}}