/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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
	monitor::subject								monitor_subject_;
	boost::timer									consume_timer_;

	video_format_desc								format_desc_;
	channel_layout									audio_channel_layout_;

	std::map<int, safe_ptr<frame_consumer>>			consumers_;
	
	high_prec_timer									sync_timer_;

	boost::circular_buffer<safe_ptr<read_frame>>	frames_;
	std::map<int, int64_t>							send_to_consumers_delays_;

	executor										executor_;
		
public:
	implementation(
			const safe_ptr<diagnostics::graph>& graph,
			const video_format_desc& format_desc,
			const channel_layout& audio_channel_layout,
			int channel_index) 
		: channel_index_(channel_index)
		, graph_(graph)
		, monitor_subject_("/output")
		, format_desc_(format_desc)
		, audio_channel_layout_(audio_channel_layout)
		, executor_(L"output")
	{
		graph_->set_color("consume-time", diagnostics::color(1.0f, 0.4f, 0.0f, 0.8));
	}

	void add(int index, safe_ptr<frame_consumer> consumer)
	{		
		remove(index);

		consumer = create_consumer_cadence_guard(consumer);
		consumer->initialize(format_desc_, audio_channel_layout_, channel_index_);

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
				send_to_consumers_delays_.erase(it->first);
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
					it->second->initialize(format_desc, audio_channel_layout_, channel_index_);
					++it;
				}
				catch(...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
					CASPAR_LOG(info) << print() << L" " << it->second->print() << L" Removed.";
					send_to_consumers_delays_.erase(it->first);
					consumers_.erase(it++);
				}
			}
			
			format_desc_ = format_desc;
			frames_.clear();
		});
	}
	
	std::map<int, size_t> buffer_depths_snapshot() const
	{
		std::map<int, size_t> result;

		BOOST_FOREACH(auto& consumer, consumers_)
			result.insert(std::make_pair(
					consumer.first,
					consumer.second->buffer_depth()));

		return std::move(result);
	}

	std::pair<size_t, size_t> minmax_buffer_depth(
			const std::map<int, size_t>& buffer_depths) const
	{		
		if(consumers_.empty())
			return std::make_pair(0, 0);
		
		auto depths = buffer_depths | boost::adaptors::map_values; 
		
		return std::make_pair(
				*boost::range::min_element(depths),
				*boost::range::max_element(depths));
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
				
				auto buffer_depths = buffer_depths_snapshot();
				auto minmax = minmax_buffer_depth(buffer_depths);

				frames_.set_capacity(minmax.second - minmax.first + 1);
				frames_.push_back(input_frame);

				if(!frames_.full())
					return;

				std::map<int, boost::unique_future<bool>> send_results;

				// Start invocations
				for (auto it = consumers_.begin(); it != consumers_.end();)
				{
					auto consumer	= it->second;
					auto frame		= frames_.at(buffer_depths[it->first]-minmax.first);

					send_to_consumers_delays_[it->first] = frame->get_age_millis();
						
					try
					{
						send_results.insert(std::make_pair(it->first, consumer->send(frame)));
						++it;
					}
					catch(...)
					{
						CASPAR_LOG_CURRENT_EXCEPTION();
						try
						{
							send_results.insert(std::make_pair(it->first, consumer->send(frame)));
							++it;
						}
						catch(...)
						{
							CASPAR_LOG_CURRENT_EXCEPTION();
							CASPAR_LOG(error) << "Failed to recover consumer: " << consumer->print() << L". Removing it.";
							send_to_consumers_delays_.erase(it->first);
							it = consumers_.erase(it);
						}
					}
				}

				// Retrieve results
				for (auto result_it = send_results.begin(); result_it != send_results.end(); ++result_it)
				{
					auto consumer		= consumers_.at(result_it->first);
					auto frame			= frames_.at(buffer_depths[result_it->first]-minmax.first);
					auto& result_future	= result_it->second;
						
					try
					{
						if (!result_future.timed_wait(boost::posix_time::seconds(2)))
						{
							BOOST_THROW_EXCEPTION(timed_out() << msg_info(narrow(print()) + " " + narrow(consumer->print()) + " Timed out during send"));
						}

						if (!result_future.get())
						{
							CASPAR_LOG(info) << print() << L" " << consumer->print() << L" Removed.";
							send_to_consumers_delays_.erase(result_it->first);
							consumers_.erase(result_it->first);
						}
					}
					catch (...)
					{
						CASPAR_LOG_CURRENT_EXCEPTION();
						try
						{
							consumer->initialize(format_desc_, audio_channel_layout_, channel_index_);
							auto retry_future = consumer->send(frame);

							if (!retry_future.timed_wait(boost::posix_time::seconds(2)))
							{
								BOOST_THROW_EXCEPTION(timed_out() << msg_info(narrow(print()) + " " + narrow(consumer->print()) + " Timed out during retry"));
							}

							if (!retry_future.get())
							{
								CASPAR_LOG(info) << print() << L" " << consumer->print() << L" Removed.";
								send_to_consumers_delays_.erase(result_it->first);
								consumers_.erase(result_it->first);
							}
						}
						catch (...)
						{
							CASPAR_LOG_CURRENT_EXCEPTION();
							CASPAR_LOG(error) << "Failed to recover consumer: " << consumer->print() << L". Removing it.";
							send_to_consumers_delays_.erase(result_it->first);
							consumers_.erase(result_it->first);
						}
					}
				}
						
				graph_->set_value("consume-time", consume_timer_.elapsed()*format_desc_.fps*0.5);
				monitor_subject_ << monitor::message("/consume_time") % (consume_timer_.elapsed());
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

	boost::unique_future<boost::property_tree::wptree> delay_info()
	{
		return std::move(executor_.begin_invoke([&]() -> boost::property_tree::wptree
		{			
			boost::property_tree::wptree info;
			BOOST_FOREACH(auto& consumer, consumers_)
			{
				auto total_age =
						consumer.second->presentation_frame_age_millis();
				auto sendoff_age = send_to_consumers_delays_[consumer.first];
				auto presentation_time = total_age - sendoff_age;

				boost::property_tree::wptree child;
				child.add(L"name", consumer.second->print());
				child.add(L"age-at-arrival", sendoff_age);
				child.add(L"presentation-time", presentation_time);
				child.add(L"age-at-presentation", total_age);

				info.add_child(L"consumer", child);
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

	monitor::subject& monitor_output()
	{ 
		return monitor_subject_;
	}
};

output::output(const safe_ptr<diagnostics::graph>& graph, const video_format_desc& format_desc, const channel_layout& audio_channel_layout, int channel_index) : impl_(new implementation(graph, format_desc, audio_channel_layout, channel_index)){}
void output::add(int index, const safe_ptr<frame_consumer>& consumer){impl_->add(index, consumer);}
void output::add(const safe_ptr<frame_consumer>& consumer){impl_->add(consumer);}
void output::remove(int index){impl_->remove(index);}
void output::remove(const safe_ptr<frame_consumer>& consumer){impl_->remove(consumer);}
void output::send(const std::pair<safe_ptr<read_frame>, std::shared_ptr<void>>& frame) {impl_->send(frame); }
void output::set_video_format_desc(const video_format_desc& format_desc){impl_->set_video_format_desc(format_desc);}
boost::unique_future<boost::property_tree::wptree> output::info() const{return impl_->info();}
boost::unique_future<boost::property_tree::wptree> output::delay_info() const{return impl_->delay_info();}
bool output::empty() const{return impl_->empty();}
monitor::subject& output::monitor_output() { return impl_->monitor_output(); }
}}