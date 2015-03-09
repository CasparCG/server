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
#include "port.h"

#include "../video_format.h"
#include "../frame/frame.h"

#include <common/assert.h>
#include <common/future.h>
#include <common/executor.h>
#include <common/diagnostics/graph.h>
#include <common/prec_timer.h>
#include <common/memshfl.h>
#include <common/env.h>
#include <common/linq.h>

#include <boost/circular_buffer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/timer.hpp>

#include <functional>

namespace caspar { namespace core {

struct output::impl
{		
	spl::shared_ptr<diagnostics::graph>	graph_;
	spl::shared_ptr<monitor::subject>	monitor_subject_	= spl::make_shared<monitor::subject>("/output");
	const int							channel_index_;
	video_format_desc					format_desc_;
	std::map<int, port>					ports_;	
	prec_timer							sync_timer_;
	boost::circular_buffer<const_frame>	frames_;
	executor							executor_			= { L"output" };
public:
	impl(spl::shared_ptr<diagnostics::graph> graph, const video_format_desc& format_desc, int channel_index) 
		: graph_(std::move(graph))
		, channel_index_(channel_index)
		, format_desc_(format_desc)
	{
		graph_->set_color("consume-time", diagnostics::color(1.0f, 0.4f, 0.0f, 0.8));
	}	
	
	void add(int index, spl::shared_ptr<frame_consumer> consumer)
	{		
		remove(index);

		consumer->initialize(format_desc_, channel_index_);
		
		executor_.begin_invoke([this, index, consumer]
		{			
			port p(index, channel_index_, std::move(consumer));
			p.monitor_output().attach_parent(monitor_subject_);
			ports_.insert(std::make_pair(index, std::move(p)));
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
			auto it = ports_.find(index);
			if(it != ports_.end())
				ports_.erase(it);					
		}, task_priority::high_priority);
	}

	void remove(const spl::shared_ptr<frame_consumer>& consumer)
	{
		remove(consumer->index());
	}
	
	void video_format_desc(const core::video_format_desc& format_desc)
	{
		executor_.invoke([&]
		{
			if(format_desc_ == format_desc)
				return;

			auto it = ports_.begin();
			while(it != ports_.end())
			{						
				try
				{
					it->second.video_format_desc(format_desc);
					++it;
				}
				catch(...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
					ports_.erase(it++);
				}
			}
			
			format_desc_ = format_desc;
			frames_.clear();
		});
	}

	std::pair<int, int> minmax_buffer_depth() const
	{		
		if(ports_.empty())
			return std::make_pair(0, 0);

		return cpplinq::from(ports_)
			.select(values())
			.select(std::mem_fn(&port::buffer_depth))
			.aggregate(minmax::initial_value<int>(), minmax());
	}

	bool has_synchronization_clock() const
	{
		return cpplinq::from(ports_)
			.select(values())
			.where(std::mem_fn(&port::has_synchronization_clock))
			.any();
	}
		
	void operator()(const_frame input_frame, const core::video_format_desc& format_desc)
	{
		boost::timer frame_timer;

		video_format_desc(format_desc);		

		executor_.invoke([=]
		{			

			if(!has_synchronization_clock())
				sync_timer_.tick(1.0/format_desc_.fps);
				
			if(input_frame.size() != format_desc_.size)
			{
				CASPAR_LOG(debug) << print() << L" Invalid input frame dimension.";
				return;
			}

			auto minmax = minmax_buffer_depth();

			frames_.set_capacity(std::max(2, minmax.second - minmax.first) + 1); // std::max(2, x) since we want to guarantee some pipeline depth for asycnhronous mixer read-back.
			frames_.push_back(input_frame);

			if(!frames_.full())
				return;

			std::map<int, std::future<bool>> send_results;

			// Start invocations
			for (auto it = ports_.begin(); it != ports_.end();)
			{
				auto& port	= it->second;
				auto& frame	= frames_.at(port.buffer_depth()-minmax.first);
					
				try
				{
					send_results.insert(std::make_pair(it->first, port.send(frame)));
					++it;
				}
				catch (...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
					try
					{
						send_results.insert(std::make_pair(it->first, port.send(frame)));
						++it;
					}
					catch(...)
					{
						CASPAR_LOG_CURRENT_EXCEPTION();
						CASPAR_LOG(error) << "Failed to recover consumer: " << port.print() << L". Removing it.";
						it = ports_.erase(it);
					}
				}
			}

			// Retrieve results
			for (auto it = send_results.begin(); it != send_results.end(); ++it)
			{
				try
				{
					if (!it->second.get())
						ports_.erase(it->first);
				}
				catch (...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
					ports_.erase(it->first);
				}
			}
		});

		graph_->set_value("consume-time", frame_timer.elapsed()*format_desc.fps*0.5);
	}

	std::wstring print() const
	{
		return L"output[" + boost::lexical_cast<std::wstring>(channel_index_) + L"]";
	}

	std::future<boost::property_tree::wptree> info()
	{
		return std::move(executor_.begin_invoke([&]() -> boost::property_tree::wptree
		{			
			boost::property_tree::wptree info;
			for (auto& port : ports_)
			{
				info.add_child(L"consumers.consumer", port.second.info())
					.add(L"index", port.first); 
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
std::future<boost::property_tree::wptree> output::info() const{return impl_->info();}
void output::operator()(const_frame frame, const video_format_desc& format_desc){(*impl_)(std::move(frame), format_desc);}
monitor::subject& output::monitor_output() {return *impl_->monitor_subject_;}
}}