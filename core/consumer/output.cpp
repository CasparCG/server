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
#include "../frame/audio_channel_layout.h"

#include <common/assert.h>
#include <common/future.h>
#include <common/executor.h>
#include <common/diagnostics/graph.h>
#include <common/prec_timer.h>
#include <common/memshfl.h>
#include <common/env.h>
#include <common/timer.h>

#include <boost/circular_buffer.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>

#include <functional>

namespace caspar { namespace core {

struct output::impl
{
	spl::shared_ptr<diagnostics::graph>	graph_;
	spl::shared_ptr<monitor::subject>	monitor_subject_			= spl::make_shared<monitor::subject>("/output");
	const int							channel_index_;
	video_format_desc					format_desc_;
	audio_channel_layout				channel_layout_;
	std::map<int, port>					ports_;
	prec_timer							sync_timer_;
	boost::circular_buffer<const_frame>	frames_;
	std::map<int, int64_t>				send_to_consumers_delays_;
	executor							executor_					{ L"output " + boost::lexical_cast<std::wstring>(channel_index_) };
public:
	impl(spl::shared_ptr<diagnostics::graph> graph, const video_format_desc& format_desc, const audio_channel_layout& channel_layout, int channel_index)
		: graph_(std::move(graph))
		, channel_index_(channel_index)
		, format_desc_(format_desc)
		, channel_layout_(channel_layout)
	{
		graph_->set_color("consume-time", diagnostics::color(1.0f, 0.4f, 0.0f, 0.8f));
	}

	void add(int index, spl::shared_ptr<frame_consumer> consumer)
	{
		remove(index);

		consumer->initialize(format_desc_, channel_layout_, channel_index_);

		executor_.begin_invoke([this, index, consumer]
		{
			port p(index, channel_index_, std::move(consumer));
			p.monitor_output().attach_parent(monitor_subject_);
			ports_.insert(std::make_pair(index, std::move(p)));
		});
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
			if (it != ports_.end())
			{
				ports_.erase(it);
				send_to_consumers_delays_.erase(index);
			}
		});
	}

	void remove(const spl::shared_ptr<frame_consumer>& consumer)
	{
		remove(consumer->index());
	}

	void change_channel_format(const core::video_format_desc& format_desc, const core::audio_channel_layout& channel_layout)
	{
		executor_.invoke([&]
		{
			if(format_desc_ == format_desc && channel_layout_ == channel_layout)
				return;

			auto it = ports_.begin();
			while(it != ports_.end())
			{
				try
				{
					it->second.change_channel_format(format_desc, channel_layout);
					++it;
				}
				catch(...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
					send_to_consumers_delays_.erase(it->first);
					ports_.erase(it++);
				}
			}

			format_desc_ = format_desc;
			channel_layout_ = channel_layout;
			frames_.clear();
		});
	}

	std::pair<int, int> minmax_buffer_depth() const
	{
		boost::optional<std::pair<int, int>> minmax;
		for (auto& p : ports_) {
			auto depth = p.second.buffer_depth();
			if (!minmax) {
				*minmax = std::pair<int, int>(depth, depth);
			} else {
				minmax->first = std::min(minmax->first, depth);
				minmax->second = std::max(minmax->second, depth);
			}
		}

		return minmax.value_or(std::pair<int, int>(0, 0));
	}

	bool has_synchronization_clock() const
	{
		bool has_synchronization_clock = false;
		for (auto& p : ports_) {
			has_synchronization_clock |= p.second.has_synchronization_clock();
		}
		return has_synchronization_clock;
	}

	std::future<void> operator()(const_frame input_frame, const core::video_format_desc& format_desc, const core::audio_channel_layout& channel_layout)
	{
		spl::shared_ptr<caspar::timer> frame_timer;

		change_channel_format(format_desc, channel_layout);

		auto pending_send_results = executor_.invoke([=]() -> std::shared_ptr<std::map<int, std::future<bool>>>
		{
			if (input_frame.size() != format_desc_.size)
			{
				CASPAR_LOG(warning) << print() << L" Invalid input frame dimension.";
				return nullptr;
			}

			auto minmax = minmax_buffer_depth();

			frames_.set_capacity(std::max(2, minmax.second - minmax.first) + 1); // std::max(2, x) since we want to guarantee some pipeline depth for asycnhronous mixer read-back.
			frames_.push_back(input_frame);

			if (!frames_.full())
				return nullptr;

			spl::shared_ptr<std::map<int, std::future<bool>>> send_results;

			// Start invocations
			for (auto it = ports_.begin(); it != ports_.end();)
			{
				auto& port = it->second;
				auto depth = port.buffer_depth();
				auto& frame = depth < 0 ? frames_.back() : frames_.at(depth - minmax.first);

				send_to_consumers_delays_[it->first] = frame.get_age_millis();

				try
				{
					send_results->insert(std::make_pair(it->first, port.send(frame)));
					++it;
				}
				catch (...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
					try
					{
						send_results->insert(std::make_pair(it->first, port.send(frame)));
						++it;
					}
					catch (...)
					{
						CASPAR_LOG_CURRENT_EXCEPTION();
						CASPAR_LOG(error) << "Failed to recover consumer: " << port.print() << L". Removing it.";
						send_to_consumers_delays_.erase(it->first);
						it = ports_.erase(it);
					}
				}
			}

			return send_results;
		});

		if (!pending_send_results)
			return make_ready_future();

		return executor_.begin_invoke([=]()
		{
			// Retrieve results
			for (auto it = pending_send_results->begin(); it != pending_send_results->end(); ++it)
			{
				try
				{
					if (!it->second.get())
					{
						send_to_consumers_delays_.erase(it->first);
						ports_.erase(it->first);
					}
				}
				catch (...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
					send_to_consumers_delays_.erase(it->first);
					ports_.erase(it->first);
				}
			}

			if (!has_synchronization_clock())
				sync_timer_.tick(1.0 / format_desc_.fps);

			auto consume_time = frame_timer->elapsed();
			graph_->set_value("consume-time", consume_time * format_desc.fps * 0.5);
			*monitor_subject_
				<< monitor::message("/consume_time") % consume_time
				<< monitor::message("/profiler/time") % consume_time % (1.0 / format_desc.fps);
		});
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
		}));
	}

	std::future<boost::property_tree::wptree> delay_info()
	{
		return std::move(executor_.begin_invoke([&]() -> boost::property_tree::wptree
		{
			boost::property_tree::wptree info;

			for (auto& port : ports_)
			{
				auto total_age =
					port.second.presentation_frame_age_millis();
				auto sendoff_age = send_to_consumers_delays_[port.first];
				auto presentation_time = total_age - sendoff_age;

				boost::property_tree::wptree child;
				child.add(L"name", port.second.print());
				child.add(L"age-at-arrival", sendoff_age);
				child.add(L"presentation-time", presentation_time);
				child.add(L"age-at-presentation", total_age);

				info.add_child(L"consumer", child);
			}
			return info;
		}));
	}

	std::vector<spl::shared_ptr<const frame_consumer>> get_consumers()
	{
		return executor_.invoke([=]
		{
			std::vector<spl::shared_ptr<const frame_consumer>> consumers;

			for (auto& port : ports_)
				consumers.push_back(port.second.consumer());

			return consumers;
		});
	}
};

output::output(spl::shared_ptr<diagnostics::graph> graph, const video_format_desc& format_desc, const core::audio_channel_layout& channel_layout, int channel_index) : impl_(new impl(std::move(graph), format_desc, channel_layout, channel_index)){}
void output::add(int index, const spl::shared_ptr<frame_consumer>& consumer){impl_->add(index, consumer);}
void output::add(const spl::shared_ptr<frame_consumer>& consumer){impl_->add(consumer);}
void output::remove(int index){impl_->remove(index);}
void output::remove(const spl::shared_ptr<frame_consumer>& consumer){impl_->remove(consumer);}
std::future<boost::property_tree::wptree> output::info() const{return impl_->info();}
std::future<boost::property_tree::wptree> output::delay_info() const{ return impl_->delay_info(); }
std::vector<spl::shared_ptr<const frame_consumer>> output::get_consumers() const { return impl_->get_consumers(); }
std::future<void> output::operator()(const_frame frame, const video_format_desc& format_desc, const core::audio_channel_layout& channel_layout){ return (*impl_)(std::move(frame), format_desc, channel_layout); }
monitor::subject& output::monitor_output() {return *impl_->monitor_subject_;}
}}
