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

#include "../video_format.h"
#include "../mixer/gpu/ogl_device.h"
#include "../mixer/read_frame.h"

#include <common/concurrency/executor.h>
#include <common/utility/assert.h>
#include <common/utility/timer.h>
#include <common/memory/memshfl.h>
#include <common/env.h>

#include <boost/circular_buffer.hpp>
#include <boost/timer.hpp>

namespace caspar { namespace core {

struct output::implementation
{	
	typedef std::pair<safe_ptr<read_frame>, safe_ptr<read_frame>> fill_and_key;
	
	const int						channel_index_;
	safe_ptr<diagnostics::graph>	graph_;
	boost::timer					consume_timer_;

	video_format_desc				format_desc_;

	std::map<int, safe_ptr<frame_consumer>> consumers_;
	typedef std::map<int, safe_ptr<frame_consumer>>::value_type layer_t;
	
	high_prec_timer timer_;

	boost::circular_buffer<safe_ptr<read_frame>> frames_;

	tbb::concurrent_bounded_queue<std::shared_ptr<read_frame>> input_;

	executor executor_;
		
public:
	implementation(const safe_ptr<diagnostics::graph>& graph, const video_format_desc& format_desc, int channel_index) 
		: channel_index_(channel_index)
		, graph_(graph)
		, format_desc_(format_desc)
		, executor_(L"output")
	{
		graph_->set_color("consume-time", diagnostics::color(1.0f, 0.4f, 0.0f));
	}	
	
	void add(int index, safe_ptr<frame_consumer>&& consumer)
	{		
		executor_.invoke([&]
		{
			consumers_.erase(index);
		});

		consumer->initialize(format_desc_, channel_index_, index);

		executor_.invoke([&]
		{
			consumers_.insert(std::make_pair(index, consumer));

			CASPAR_LOG(info) << print() << L" " << consumer->print() << L" Added.";
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
	
	void set_video_format_desc(const video_format_desc& format_desc)
	{
		executor_.invoke([&]
		{
			format_desc_ = format_desc;

			auto it = consumers_.begin();
			while(it != consumers_.end())
			{						
				try
				{
					it->second->initialize(format_desc_, channel_index_, it->first);
					++it;
				}
				catch(...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
					consumers_.erase(it++);
				}
			}

			frames_.clear();
		});
	}
			
	std::pair<size_t, size_t> minmax_buffer_depth() const
	{		
		if(consumers_.empty())
			return std::make_pair(0, 0);
		std::vector<size_t> buffer_depths;
		std::transform(consumers_.begin(), consumers_.end(), std::back_inserter(buffer_depths), [](const decltype(*consumers_.begin())& pair)
		{
			return pair.second->buffer_depth();
		});
		std::sort(buffer_depths.begin(), buffer_depths.end());
		auto min = buffer_depths.front();
		auto max = buffer_depths.back();
		return std::make_pair(min, max);
	}

	void send(const std::pair<safe_ptr<read_frame>, std::shared_ptr<void>>& packet)
	{
		executor_.begin_invoke([=]
		{
			try
			{
				consume_timer_.restart();

				auto input_frame = packet.first;

				if(!has_synchronization_clock())
					timer_.tick(1.0/format_desc_.fps);

				if(input_frame->image_size() != format_desc_.size)
				{
					timer_.tick(1.0/format_desc_.fps);
					return;
				}
					
				const auto minmax = minmax_buffer_depth();

				frames_.set_capacity(minmax.second - minmax.first + 1);
				frames_.push_back(input_frame);

				if(!frames_.full())
					return;

				auto it = consumers_.begin();
				while(it != consumers_.end())
				{
					auto consumer	= it->second;
					auto frame		= frames_.at(consumer->buffer_depth()-minmax.first);
						
					try
					{
						if(consumer->send(frame))
							++it;
						else
							consumers_.erase(it++);
					}
					catch(...)
					{
						CASPAR_LOG_CURRENT_EXCEPTION();
						try
						{
							consumer->initialize(format_desc_, channel_index_, it->first);
							if(consumer->send(frame))
								++it;
							else
								consumers_.erase(it++);
						}
						catch(...)
						{
							CASPAR_LOG_CURRENT_EXCEPTION();
							CASPAR_LOG(error) << "Failed to recover consumer: " << consumer->print() << L". Removing it.";
							consumers_.erase(it++);
						}
					}
				}
						
				graph_->update_value("consume-time", consume_timer_.elapsed()*format_desc_.fps*0.5);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		});
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

output::output(const safe_ptr<diagnostics::graph>& graph, const video_format_desc& format_desc, int channel_index) : impl_(new implementation(graph, format_desc, channel_index)){}
void output::add(int index, safe_ptr<frame_consumer>&& consumer){impl_->add(index, std::move(consumer));}
void output::remove(int index){impl_->remove(index);}
void output::send(const std::pair<safe_ptr<read_frame>, std::shared_ptr<void>>& frame) {impl_->send(frame); }
void output::set_video_format_desc(const video_format_desc& format_desc){impl_->set_video_format_desc(format_desc);}
}}