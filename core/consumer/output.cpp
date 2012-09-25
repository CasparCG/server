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
#include <boost/range/algorithm.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/property_tree/ptree.hpp>

namespace caspar { namespace core {
	
struct output::implementation
{		
	const int										channel_index_;
	const safe_ptr<diagnostics::graph>				graph_;
	boost::timer									consume_timer_;

	video_format_desc								format_desc_;

	std::map<int, safe_ptr<frame_consumer>>			consumers_;
	
	high_prec_timer									sync_timer_;

	boost::circular_buffer<safe_ptr<read_frame>>	frames_;

	executor										executor_;
		
public:
	implementation(const safe_ptr<diagnostics::graph>& graph, const video_format_desc& format_desc, int channel_index) 
		: channel_index_(channel_index)
		, graph_(graph)
		, format_desc_(format_desc)
		, executor_(L"output")
	{
		graph_->set_color("consume-time", diagnostics::color(1.0f, 0.4f, 0.0f, 0.8));
	}	
	
	void add(int index, safe_ptr<frame_consumer> consumer)
	{		
		remove(index);

		consumer = create_consumer_cadence_guard(consumer);
		consumer->initialize(format_desc_, channel_index_);

		executor_.invoke([&]
		{
			consumers_.insert(std::make_pair(index, consumer));
			CASPAR_LOG(info) << print() << L" " << consumer->print() << L" Added.";
		}, high_priority);
	}

	void add(const safe_ptr<frame_consumer>& consumer)
	{
		add(consumer->index(), consumer);
	}

	void remove(int index)
	{		
		// Destroy  consumer on calling thread:
		std::shared_ptr<frame_consumer> old_consumer;

		executor_.invoke([&]
		{
			auto it = consumers_.find(index);
			if(it != consumers_.end())
			{
				old_consumer = it->second;
				consumers_.erase(it);
			}
		}, high_priority);

		if(old_consumer)
		{
			auto str = old_consumer->print();
			old_consumer.reset();
			CASPAR_LOG(info) << print() << L" " << str << L" Removed.";
		}
	}

	void remove(const safe_ptr<frame_consumer>& consumer)
	{
		remove(consumer->index());
	}
	
	void set_video_format_desc(const video_format_desc& format_desc)
	{
		executor_.invoke([&]
		{
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
		});
	}

	std::pair<size_t, size_t> minmax_buffer_depth() const
	{		
		if(consumers_.empty())
			return std::make_pair(0, 0);
		
		auto buffer_depths = consumers_ | 
							 boost::adaptors::map_values | // std::function is MSVC workaround
							 boost::adaptors::transformed(std::function<int(const safe_ptr<frame_consumer>&)>([](const safe_ptr<frame_consumer>& x){return x->buffer_depth();})); 
		

		return std::make_pair(*boost::range::min_element(buffer_depths), *boost::range::max_element(buffer_depths));
	}

	bool has_synchronization_clock() const
	{
		return boost::range::count_if(consumers_ | boost::adaptors::map_values, [](const safe_ptr<frame_consumer>& x){return x->has_synchronization_clock();}) > 0;
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
					sync_timer_.tick(1.0/format_desc_.fps);

				if(input_frame->image_size() != format_desc_.size)
				{
					sync_timer_.tick(1.0/format_desc_.fps);
					return;
				}
					
				auto minmax = minmax_buffer_depth();

				frames_.set_capacity(minmax.second - minmax.first + 1);
				frames_.push_back(input_frame);

				if(!frames_.full())
					return;

				std::map<int, boost::unique_future<bool>> send_results;

				// Start invocations
				for (auto it = consumers_.begin(); it != consumers_.end(); ++it)
				{
					auto consumer	= it->second;
					auto frame		= frames_.at(consumer->buffer_depth()-minmax.first);
						
					try
					{
						send_results.insert(std::make_pair(it->first, consumer->send(frame)));
					}
					catch(...)
					{
						CASPAR_LOG_CURRENT_EXCEPTION();
						try
						{
							send_results.insert(std::make_pair(it->first, consumer->send(frame)));
						}
						catch(...)
						{
							CASPAR_LOG_CURRENT_EXCEPTION();
							CASPAR_LOG(error) << "Failed to recover consumer: " << consumer->print() << L". Removing it.";
							consumers_.erase(it);
						}
					}
				}

				// Retrieve results
				for (auto result_it = send_results.begin(); result_it != send_results.end(); ++result_it)
				{
					auto consumer		= consumers_.at(result_it->first);
					auto frame			= frames_.at(consumer->buffer_depth()-minmax.first);
					auto& result_future	= result_it->second;
						
					try
					{
						if(!result_future.get())
						{
							CASPAR_LOG(info) << print() << L" " << consumer->print() << L" Removed.";
							consumers_.erase(result_it->first);
						}
					}
					catch(...)
					{
						CASPAR_LOG_CURRENT_EXCEPTION();
						try
						{
							consumer->initialize(format_desc_, channel_index_);
							if(!consumer->send(frame).get())
							{
								CASPAR_LOG(info) << print() << L" " << consumer->print() << L" Removed.";
								consumers_.erase(result_it->first);
							}
						}
						catch(...)
						{
							CASPAR_LOG_CURRENT_EXCEPTION();
							CASPAR_LOG(error) << "Failed to recover consumer: " << consumer->print() << L". Removing it.";
							consumers_.erase(result_it->first);
						}
					}
				}
						
				graph_->set_value("consume-time", consume_timer_.elapsed()*format_desc_.fps*0.5);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
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
		}, high_priority));
	}

	bool empty()
	{
		return executor_.invoke([this]
		{
			return consumers_.empty();
		});
	}
};

output::output(const safe_ptr<diagnostics::graph>& graph, const video_format_desc& format_desc, int channel_index) : impl_(new implementation(graph, format_desc, channel_index)){}
void output::add(int index, const safe_ptr<frame_consumer>& consumer){impl_->add(index, consumer);}
void output::add(const safe_ptr<frame_consumer>& consumer){impl_->add(consumer);}
void output::remove(int index){impl_->remove(index);}
void output::remove(const safe_ptr<frame_consumer>& consumer){impl_->remove(consumer);}
void output::send(const std::pair<safe_ptr<read_frame>, std::shared_ptr<void>>& frame) {impl_->send(frame); }
void output::set_video_format_desc(const video_format_desc& format_desc){impl_->set_video_format_desc(format_desc);}
boost::unique_future<boost::property_tree::wptree> output::info() const{return impl_->info();}
bool output::empty() const{return impl_->empty();}
}}