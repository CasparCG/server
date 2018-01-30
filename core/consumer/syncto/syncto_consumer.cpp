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

#include "syncto_consumer.h"

#include "../frame_consumer.h"
#include "../../frame/frame.h"
#include "../../module_dependencies.h"
#include "../../monitor/monitor.h"
#include "../../video_channel.h"
#include "../output.h"

#include <common/semaphore.h>

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>

#include <future>

namespace caspar { namespace core { namespace syncto {

void verify_cyclic_reference(int self_channel_index, const spl::shared_ptr<video_channel>& other_channel);

class syncto_consumer : public frame_consumer
{
	monitor::subject				monitor_subject_;
	spl::shared_ptr<video_channel>	other_channel_;
	semaphore						frames_to_render_	{ 0 };
	std::shared_ptr<void>			tick_subscription_;
	int								self_channel_index_	= -1;
public:
	syncto_consumer(spl::shared_ptr<video_channel> other_channel)
		: other_channel_(std::move(other_channel))
	{
	}

	void initialize(const video_format_desc& format_desc, int channel_index) override
	{
		verify_cyclic_reference(channel_index, other_channel_);

		self_channel_index_	= channel_index;
		tick_subscription_	= other_channel_->add_tick_listener([=]
		{
			frames_to_render_.release();
		});
	}

	std::future<bool> send(const_frame frame) override
	{
		auto task = spl::make_shared<std::packaged_task<bool ()>>([=] { return true; });

		frames_to_render_.acquire(1, [task]
		{
			(*task)();
		});

		return task->get_future();
	}

	monitor::subject& monitor_output() override
	{
		return monitor_subject_;
	}

	std::wstring print() const override
	{
		if (self_channel_index_ != -1)
			return L"sync[" + boost::lexical_cast<std::wstring>(self_channel_index_) + L"]to[" + boost::lexical_cast<std::wstring>(other_channel_->index()) + L"]";
		else
			return L"syncto[" + boost::lexical_cast<std::wstring>(other_channel_->index()) + L"]";
	}

	std::wstring name() const override
	{
		return L"syncto";
	}

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"syncto-consumer");
		info.add(L"channel-to-sync-to", other_channel_->index());
		return info;
	}

	bool has_synchronization_clock() const override
	{
		return true;
	}

	int buffer_depth() const override
	{
		return -1;
	}

	int index() const override
	{
		return 70000;
	}

	int64_t presentation_frame_age_millis() const override
	{
		return 0;
	}

	spl::shared_ptr<video_channel> other_channel() const
	{
		return other_channel_;
	}
};

void verify_cyclic_reference(int self_channel_index, const spl::shared_ptr<video_channel>& other_channel)
{
	if (self_channel_index == other_channel->index())
		CASPAR_THROW_EXCEPTION(user_error() << msg_info(
				L"Cannot create syncto consumer where source channel and destination channel is the same or indirectly related"));

	for (auto& consumer : other_channel->output().get_consumers())
	{
		auto raw_consumer	= consumer->unwrapped();
		auto syncto			= dynamic_cast<const syncto_consumer*>(raw_consumer);

		if (syncto)
			verify_cyclic_reference(self_channel_index, syncto->other_channel());
	}
}

spl::shared_ptr<core::frame_consumer> create_consumer(
		const std::vector<std::wstring>& params,
		core::interaction_sink*,
		std::vector<spl::shared_ptr<video_channel>> channels)
{
	if (params.size() < 1 || !boost::iequals(params.at(0), L"SYNCTO"))
		return core::frame_consumer::empty();

	auto channel_id	= boost::lexical_cast<int>(params.at(1));
	auto channel	= channels.at(channel_id - 1);

	return spl::make_shared<syncto_consumer>(channel);
}

spl::shared_ptr<core::frame_consumer> create_preconfigured_consumer(
		const boost::property_tree::wptree& ptree,
		core::interaction_sink*,
		std::vector<spl::shared_ptr<video_channel>> channels)
{
	auto channel_id = ptree.get<int>(L"channel-id");

	return spl::make_shared<syncto_consumer>(channels.at(channel_id - 1));
}

void init(module_dependencies dependencies)
{
	dependencies.consumer_registry->register_consumer_factory(L"syncto", &create_consumer);
	dependencies.consumer_registry->register_preconfigured_consumer_factory(L"syncto", &create_preconfigured_consumer);
}

}}}
