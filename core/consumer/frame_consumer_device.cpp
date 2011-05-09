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
#include "../StdAfx.h"

#ifdef _MSC_VER
#pragma warning (disable : 4244)
#endif

#include "frame_consumer_device.h"

#include "../video_format.h"

#include <common/concurrency/executor.h>
#include <common/diagnostics/graph.h>
#include <common/utility/assert.h>

#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/timer.hpp>

namespace caspar { namespace core {
	
struct frame_consumer_device::implementation
{	
	boost::circular_buffer<safe_ptr<const read_frame>> buffer_;

	std::map<int, std::shared_ptr<frame_consumer>> consumers_; // Valid iterators after erase
	
	safe_ptr<diagnostics::graph> diag_;

	video_format_desc format_desc_;

	boost::timer frame_timer_;
	boost::timer tick_timer_;
	
	executor executor_;	
public:
	implementation( const video_format_desc& format_desc) 
		: format_desc_(format_desc)
		, diag_(diagnostics::create_graph(std::string("frame_consumer_device")))
		, executor_(L"frame_consumer_device", true)
	{		
		diag_->set_color("input-buffer", diagnostics::color(1.0f, 1.0f, 0.0f));	
		diag_->add_guide("frame-time", 0.5f);	
		diag_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));
		diag_->set_color("tick-time", diagnostics::color(0.1f, 0.7f, 0.8f));

		executor_.set_capacity(1);
		executor_.begin_invoke([]
		{
			SetThreadPriority(GetCurrentThread(), ABOVE_NORMAL_PRIORITY_CLASS);
		});
	}

	void add(int index, safe_ptr<frame_consumer>&& consumer)
	{		
		consumer->initialize(format_desc_);
		executor_.invoke([&]
		{
			if(buffer_.capacity() < consumer->buffer_depth())
				buffer_.set_capacity(consumer->buffer_depth());
			consumers_[index] = std::move(consumer);
		});
	}

	void remove(int index)
	{
		executor_.invoke([&]
		{
			auto it = consumers_.find(index);
			if(it != consumers_.end())
			{
				CASPAR_LOG(info) << print() << L" " << it->second->print() << L" Removed.";
				consumers_.erase(it);
			}
		});
	}
				
	void send(const safe_ptr<const read_frame>& frame)
	{		
		executor_.begin_invoke([=]
		{
			diag_->set_value("input-buffer", static_cast<float>(executor_.size())/static_cast<float>(executor_.capacity()));
			frame_timer_.restart();

			buffer_.push_back(std::move(frame));

			if(!buffer_.full())
				return;
	
			auto it = consumers_.begin();
			while(it != consumers_.end())
			{
				try
				{
					it->second->send(buffer_[it->second->buffer_depth()-1]);
					++it;
				}
				catch(...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
					consumers_.erase(it++);
					CASPAR_LOG(error) << print() << L" " << it->second->print() << L" Removed.";
				}
			}
			diag_->update_value("frame-time", static_cast<float>(frame_timer_.elapsed()/format_desc_.interval*0.5));
			
			diag_->update_value("tick-time", static_cast<float>(tick_timer_.elapsed()/format_desc_.interval*0.5));
			tick_timer_.restart();
		});
		diag_->set_value("input-buffer", static_cast<float>(executor_.size())/static_cast<float>(executor_.capacity()));
	}

	std::wstring print() const
	{
		return L"frame_consumer_device";
	}
	
	void set_video_format_desc(const video_format_desc& format_desc)
	{
		executor_.invoke([&]
		{
			format_desc_ = format_desc;
			buffer_.clear();
			
			auto it = consumers_.begin();
			while(it != consumers_.end())
			{
				try
				{
					it->second->initialize(format_desc_);
					++it;
				}
				catch(...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
					consumers_.erase(it++);
					CASPAR_LOG(error) << print() << L" " << it->second->print() << L" Removed.";
				}
			}
		});
	}
};

frame_consumer_device::frame_consumer_device(const video_format_desc& format_desc) : impl_(new implementation(format_desc)){}
void frame_consumer_device::add(int index, safe_ptr<frame_consumer>&& consumer){impl_->add(index, std::move(consumer));}
void frame_consumer_device::remove(int index){impl_->remove(index);}
void frame_consumer_device::send(const safe_ptr<const read_frame>& future_frame) { impl_->send(future_frame); }
void frame_consumer_device::set_video_format_desc(const video_format_desc& format_desc){impl_->set_video_format_desc(format_desc);}
}}