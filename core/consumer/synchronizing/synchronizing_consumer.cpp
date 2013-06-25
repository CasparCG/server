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
* Author: Helge Norberg, helge.norberg@svt.se
*/

#include "../../StdAfx.h"

#include "synchronizing_consumer.h"

#include <common/log/log.h>
#include <common/diagnostics/graph.h>
#include <common/concurrency/future_util.h>

#include <core/video_format.h>

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/min_element.hpp>
#include <boost/range/algorithm/max_element.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/count_if.hpp>
#include <boost/range/numeric.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/thread/future.hpp>

#include <functional>
#include <vector>
#include <queue>
#include <utility>

#include <tbb/atomic.h>

namespace caspar { namespace core {

using namespace boost::adaptors;

class delegating_frame_consumer : public frame_consumer
{
	safe_ptr<frame_consumer> consumer_;
public:
	delegating_frame_consumer(const safe_ptr<frame_consumer>& consumer)
		: consumer_(consumer)
	{
	}

	frame_consumer& get_delegate()
	{
		return *consumer_;
	}

	const frame_consumer& get_delegate() const
	{
		return *consumer_;
	}
	
	virtual void initialize(
			const video_format_desc& format_desc, int channel_index) override
	{
		get_delegate().initialize(format_desc, channel_index);
	}

	virtual int64_t presentation_frame_age_millis() const
	{
		return get_delegate().presentation_frame_age_millis();
	}

	virtual boost::unique_future<bool> send(
			const safe_ptr<read_frame>& frame) override
	{		
		return get_delegate().send(frame);
	}

	virtual std::wstring print() const override
	{
		return get_delegate().print();
	}

	virtual boost::property_tree::wptree info() const override
	{
		return get_delegate().info();
	}

	virtual bool has_synchronization_clock() const override
	{
		return get_delegate().has_synchronization_clock();
	}

	virtual size_t buffer_depth() const override
	{
		return get_delegate().buffer_depth();
	}

	virtual int index() const override
	{
		return get_delegate().index();
	}
};

const std::vector<int>& diag_colors()
{
	static std::vector<int> colors = boost::assign::list_of<int>
			(diagnostics::color(0.0f, 0.6f, 0.9f))
			(diagnostics::color(0.6f, 0.3f, 0.3f))
			(diagnostics::color(0.3f, 0.6f, 0.3f))
			(diagnostics::color(0.4f, 0.3f, 0.8f))
			(diagnostics::color(0.9f, 0.9f, 0.5f))
			(diagnostics::color(0.2f, 0.9f, 0.9f));

	return colors;
}

class buffering_consumer_adapter : public delegating_frame_consumer
{
	std::queue<safe_ptr<read_frame>>	buffer_;
	tbb::atomic<size_t>					buffered_;
	tbb::atomic<int64_t>				duplicate_next_;
public:
	buffering_consumer_adapter(const safe_ptr<frame_consumer>& consumer)
		: delegating_frame_consumer(consumer)
	{
		buffered_ = 0;
		duplicate_next_ = 0;
	}

	boost::unique_future<bool> consume_one()
	{
		if (!buffer_.empty())
		{
			buffer_.pop();
			--buffered_;
		}

		return get_delegate().send(buffer_.front());
	}

	virtual boost::unique_future<bool> send(
			const safe_ptr<read_frame>& frame) override
	{
		if (duplicate_next_)
		{
			--duplicate_next_;
		}
		else if (!buffer_.empty())
		{
			buffer_.pop();
			--buffered_;
		}

		buffer_.push(frame);
		++buffered_;

		return get_delegate().send(buffer_.front());
	}

	void duplicate_next(int64_t to_duplicate)
	{
		duplicate_next_ += to_duplicate;
	}

	size_t num_buffered() const
	{
		return buffered_ - 1;
	}

	virtual std::wstring print() const override
	{
		return L"buffering[" + get_delegate().print() + L"]";
	}

	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;

		info.add(L"type", L"buffering-consumer-adapter");
		info.add_child(L"consumer", get_delegate().info());
		info.add(L"buffered-frames", num_buffered());

		return info;
	}
};

static const uint64_t MAX_BUFFERED_OUT_OF_MEMORY_GUARD = 5;

struct synchronizing_consumer::implementation
{
private:
	std::vector<safe_ptr<buffering_consumer_adapter>>	consumers_;
	size_t												buffer_depth_;
	bool												has_synchronization_clock_;
	std::vector<boost::unique_future<bool>>				results_;
	boost::promise<bool>								promise_;
	video_format_desc									format_desc_;
	safe_ptr<diagnostics::graph>						graph_;
	int64_t												grace_period_;
	tbb::atomic<int64_t>								current_diff_;
public:
	implementation(const std::vector<safe_ptr<frame_consumer>>& consumers)
		: grace_period_(0)
	{
		BOOST_FOREACH(auto& consumer, consumers)
			consumers_.push_back(make_safe<buffering_consumer_adapter>(consumer));

		current_diff_ = 0;
		auto buffer_depths = consumers | transformed(std::mem_fn(&frame_consumer::buffer_depth));
		std::vector<size_t> depths(buffer_depths.begin(), buffer_depths.end());
		buffer_depth_ = *boost::max_element(depths);
		has_synchronization_clock_ = boost::count_if(consumers, std::mem_fn(&frame_consumer::has_synchronization_clock)) > 0;

		diagnostics::register_graph(graph_);
	}

	boost::unique_future<bool> send(const safe_ptr<read_frame>& frame)
	{
		results_.clear();

		BOOST_FOREACH(auto& consumer, consumers_)
			results_.push_back(consumer->send(frame));

		promise_ = boost::promise<bool>();
		promise_.set_wait_callback(std::function<void(boost::promise<bool>&)>([this](boost::promise<bool>& promise)
		{
			BOOST_FOREACH(auto& result, results_)
			{
				result.get();
			}

			auto frame_ages = consumers_ | transformed(std::mem_fn(&frame_consumer::presentation_frame_age_millis));
			std::vector<int64_t> ages(frame_ages.begin(), frame_ages.end());
			auto max_age_iter = boost::max_element(ages);
			auto min_age_iter = boost::min_element(ages);
			int64_t min_age = *min_age_iter;
			
			if (min_age == 0)
			{
				// One of the consumers have yet no measurement, wait until next 
				// frame until we make any assumptions.
				promise.set_value(true);
				return;
			}

			int64_t max_age = *max_age_iter;
			int64_t age_diff = max_age - min_age;

			current_diff_ = age_diff;

			for (unsigned i = 0; i < ages.size(); ++i)
				graph_->set_value(
						narrow(consumers_[i]->print()),
						static_cast<double>(ages[i]) / *max_age_iter);

			bool grace_period_over = grace_period_ == 1;

			if (grace_period_)
				--grace_period_;

			if (grace_period_ == 0)
			{
				int64_t frame_duration = static_cast<int64_t>(1000 / format_desc_.fps);

				if (age_diff >= frame_duration)
				{
					CASPAR_LOG(info) << print() << L" Consumers not in sync. min: " << min_age << L" max: " << max_age;

					auto index = min_age_iter - ages.begin();
					auto to_duplicate = age_diff / frame_duration;
					auto& consumer = *consumers_.at(index);

					auto currently_buffered = consumer.num_buffered();

					if (currently_buffered + to_duplicate > MAX_BUFFERED_OUT_OF_MEMORY_GUARD)
					{
						CASPAR_LOG(info) << print() << L" Protecting from out of memory. Duplicating less frames than calculated";

						to_duplicate = MAX_BUFFERED_OUT_OF_MEMORY_GUARD - currently_buffered;
					}

					consumer.duplicate_next(to_duplicate);

					grace_period_ = 10 + to_duplicate + buffer_depth_;
				}
				else if (grace_period_over)
				{
					CASPAR_LOG(info) << print() << L" Consumers resynced. min: " << min_age << L" max: " << max_age;
				}
			}

			blocking_consume_unnecessarily_buffered();

			promise.set_value(true);
		}));

		return promise_.get_future();
	}

	void blocking_consume_unnecessarily_buffered()
	{
		auto buffered = consumers_ | transformed(std::mem_fn(&buffering_consumer_adapter::num_buffered));
		std::vector<size_t> num_buffered(buffered.begin(), buffered.end());
		auto min_buffered = *boost::min_element(num_buffered);

		if (min_buffered)
			CASPAR_LOG(info) << print() << L" " << min_buffered 
					<< L" frames unnecessarily buffered. Consuming and letting channel pause during that time.";

		while (min_buffered)
		{
			std::vector<boost::unique_future<bool>> results;

			BOOST_FOREACH(auto& consumer, consumers_)
				results.push_back(consumer->consume_one());

			BOOST_FOREACH(auto& result, results)
				result.get();

			--min_buffered;
		}
	}

	void initialize(const video_format_desc& format_desc, int channel_index)
	{
		for (size_t i = 0; i < consumers_.size(); ++i)
		{
			auto& consumer = consumers_.at(i);
			consumer->initialize(format_desc, channel_index); 
			graph_->set_color(
					narrow(consumer->print()),
					diag_colors().at(i % diag_colors().size()));
		}

		graph_->set_text(print());
		format_desc_ = format_desc;
	}

	int64_t presentation_frame_age_millis() const
	{
		int64_t result = 0;

		BOOST_FOREACH(auto& consumer, consumers_)
			result = std::max(result, consumer->presentation_frame_age_millis());

		return result;
	}

	std::wstring print() const
	{
		return L"synchronized[" + boost::algorithm::join(consumers_ | transformed(std::mem_fn(&frame_consumer::print)), L"|") +  L"]";
	}

	boost::property_tree::wptree info() const
	{
		boost::property_tree::wptree info;

		info.add(L"type", L"synchronized-consumer");

		BOOST_FOREACH(auto& consumer, consumers_)
			info.add_child(L"consumer", consumer->info());

		info.add(L"age-diff", current_diff_);

		return info;
	}

	bool has_synchronization_clock() const
	{
		return has_synchronization_clock_;
	}

	size_t buffer_depth() const
	{
		return buffer_depth_;
	}

	int index() const
	{
		return boost::accumulate(consumers_ | transformed(std::mem_fn(&frame_consumer::index)), 10000);
	}
};

synchronizing_consumer::synchronizing_consumer(const std::vector<safe_ptr<frame_consumer>>& consumers)
	: impl_(new implementation(consumers))
{
}

boost::unique_future<bool> synchronizing_consumer::send(const safe_ptr<read_frame>& frame)
{
	return impl_->send(frame);
}

void synchronizing_consumer::initialize(const video_format_desc& format_desc, int channel_index)
{
	impl_->initialize(format_desc, channel_index);
}

int64_t synchronizing_consumer::presentation_frame_age_millis() const
{
	return impl_->presentation_frame_age_millis();
}

std::wstring synchronizing_consumer::print() const
{
	return impl_->print();
}

boost::property_tree::wptree synchronizing_consumer::info() const
{
	return impl_->info();
}

bool synchronizing_consumer::has_synchronization_clock() const
{
	return impl_->has_synchronization_clock();
}

size_t synchronizing_consumer::buffer_depth() const
{
	return impl_->buffer_depth();
}

int synchronizing_consumer::index() const
{
	return impl_->index();
}

}}
