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

#include "../StdAfx.h"

#ifdef _MSC_VER
#pragma warning (disable : 4244)
#endif

#include "output.h"

#include "frame_consumer.h"

#include "../video_format.h"
#include "../frame/data_frame.h"

#include <common/concurrency/async.h>
#include <common/concurrency/executor.h>
#include <common/diagnostics/graph.h>
#include <common/prec_timer.h>
#include <common/memory/memshfl.h>
#include <common/env.h>

#include <common/assert.h>
#include <boost/circular_buffer.hpp>
#include <boost/timer.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/property_tree/ptree.hpp>

namespace caspar { namespace core {
	
struct output::impl
{		
	spl::shared_ptr<diagnostics::graph>							graph_;
	const int													channel_index_;
	video_format_desc											format_desc_;
	std::map<int, spl::shared_ptr<frame_consumer>>				consumers_;	
	prec_timer													sync_timer_;
	boost::circular_buffer<spl::shared_ptr<const data_frame>>	frames_;
	executor													executor_;		
public:
	impl(spl::shared_ptr<diagnostics::graph> graph, const video_format_desc& format_desc, int channel_index) 
		: graph_(std::move(graph))
		, channel_index_(channel_index)
		, format_desc_(format_desc)
		, executor_(L"output")
	{
		graph_->set_color("consume-time", diagnostics::color(1.0f, 0.4f, 0.0f, 0.8));
	}	
	
	void add(int index, spl::shared_ptr<frame_consumer> consumer)
	{		
		remove(index);

		consumer->initialize(format_desc_, channel_index_);

		executor_.begin_invoke([=]
		{
			consumers_.insert(std::make_pair(index, consumer));
			CASPAR_LOG(info) << print() << L" " << consumer->print() << L" Added.";
		}, task_priority::high_priority);
	}

	void add(const spl::shared_ptr<frame_consumer>& consumer)
	{
		add(consumer->index(), consumer);
	}

	void remove(int index)
	{		
		executor_.begin_invoke([=]
		{
			auto it = consumers_.find(index);
			if(it != consumers_.end())
				consumers_.erase(it);					
		}, task_priority::high_priority);
	}

	void remove(const spl::shared_ptr<frame_consumer>& consumer)
	{
		remove(consumer->index());
	}
	
	void set_video_format_desc(const video_format_desc& format_desc)
	{
		executor_.invoke([&]
		{
			boost::timer frame_timer;

			auto it = consumers_.begin();
			while(it != consumers_.end())
			{						
				try
				{
					it->second->initialize(format_desc, channel_index_);
					++it;
				}
				catch(...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
					CASPAR_LOG(info) << print() << L" " << it->second->print() << L" Removed.";
					consumers_.erase(it++);
				}
			}
			
			format_desc_ = format_desc;
			frames_.clear();

			graph_->set_value("consume-time", frame_timer.elapsed()*format_desc.fps*0.5);
		});
	}

	std::pair<int, int> minmax_buffer_depth() const
	{		
		if(consumers_.empty())
			return std::make_pair(0, 0);
		
		auto buffer_depths = consumers_ | 
							 boost::adaptors::map_values | // std::function is MSVC workaround
							 boost::adaptors::transformed(std::function<int(const spl::shared_ptr<frame_consumer>&)>([](const spl::shared_ptr<frame_consumer>& x){return x->buffer_depth();})); 
		

		return std::make_pair(*boost::range::min_element(buffer_depths), *boost::range::max_element(buffer_depths));
	}

	bool has_synchronization_clock() const
	{
		return boost::range::count_if(consumers_ | boost::adaptors::map_values, [](const spl::shared_ptr<frame_consumer>& x){return x->has_synchronization_clock();}) > 0;
	}
		
	void operator()(spl::shared_ptr<const data_frame> input_frame, const video_format_desc& format_desc)
	{
		executor_.invoke([=]
		{			
			if(!has_synchronization_clock())
				sync_timer_.tick(1.0/format_desc_.fps);
				
			if(format_desc_ != format_desc)
				set_video_format_desc(format_desc);

			if(input_frame->image_data().size() != format_desc_.size)
			{
				sync_timer_.tick(1.0/format_desc_.fps);
				return;
			}
					
			auto minmax = minmax_buffer_depth();

			frames_.set_capacity(std::max(1, minmax.second - minmax.first) + 1); // std::max(1, x) since we want to guarantee some pipeline depth for asycnhronous mixer read-back.
			frames_.push_back(input_frame);

			if(!frames_.full())
				return;

			for(auto it = consumers_.begin(); it != consumers_.end();)
			{
				auto consumer	= it->second;
				auto frame		= frames_.at(consumer->buffer_depth()-minmax.first);
						
				if(consumer->send(frame))
					++it;
				else
				{
					CASPAR_LOG(info) << print() << L" " << it->second->print() << L" Removed.";
					consumers_.erase(it++);
				}
			}
		});
	}

	std::wstring print() const
	{
		return L"output[" + boost::lexical_cast<std::wstring>(channel_index_) + L"]";
	}

	boost::unique_future<boost::property_tree::wptree> info()
	{
		return std::move(executor_.begin_invoke([&]() -> boost::property_tree::wptree
		{			
			boost::property_tree::wptree info;
			BOOST_FOREACH(auto& consumer, consumers_)
			{
				info.add_child(L"consumers.consumer", consumer.second->info())
					.add(L"index", consumer.first); 
			}
			return info;
		}, task_priority::high_priority));
	}
};

output::output(spl::shared_ptr<diagnostics::graph> graph, const video_format_desc& format_desc, int channel_index) : impl_(new impl(std::move(graph), format_desc, channel_index)){}
void output::add(int index, const spl::shared_ptr<frame_consumer>& consumer){impl_->add(index, consumer);}
void output::add(const spl::shared_ptr<frame_consumer>& consumer){impl_->add(consumer);}
void output::remove(int index){impl_->remove(index);}
void output::remove(const spl::shared_ptr<frame_consumer>& consumer){impl_->remove(consumer);}
boost::unique_future<boost::property_tree::wptree> output::info() const{return impl_->info();}
void output::operator()(spl::shared_ptr<const data_frame> frame, const video_format_desc& format_desc){(*impl_)(std::move(frame), format_desc);}
}}