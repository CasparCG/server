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

#include "frame_consumer.h"

#include <common/except.h>
#include <common/future.h>
#include <common/os/general_protection_fault.h>

#include <core/video_format.h>
#include <core/frame/frame.h>
#include <core/frame/audio_channel_layout.h>

#include <future>
#include <vector>
#include <map>

namespace caspar { namespace core {

struct frame_consumer_registry::impl
{
	std::vector<consumer_factory_t>								consumer_factories;
	std::map<std::wstring, preconfigured_consumer_factory_t>	preconfigured_consumer_factories;
};

frame_consumer_registry::frame_consumer_registry()
	: impl_(new impl())
{
}

void frame_consumer_registry::register_consumer_factory(const std::wstring& name, const consumer_factory_t& factory)
{
	impl_->consumer_factories.push_back(factory);
}

void frame_consumer_registry::register_preconfigured_consumer_factory(
		const std::wstring& element_name,
		const preconfigured_consumer_factory_t& factory)
{
	impl_->preconfigured_consumer_factories.insert(std::make_pair(element_name, factory));
}

std::atomic<bool>& destroy_consumers_in_separate_thread()
{
	static std::atomic<bool> state;

	return state;
}

void destroy_consumers_synchronously()
{
	destroy_consumers_in_separate_thread() = false;
}

class destroy_consumer_proxy : public frame_consumer
{
	std::shared_ptr<frame_consumer> consumer_;
public:
	destroy_consumer_proxy(spl::shared_ptr<frame_consumer>&& consumer)
		: consumer_(std::move(consumer))
	{
		destroy_consumers_in_separate_thread() = true;
	}

	~destroy_consumer_proxy()
	{
		static std::atomic<int> counter;
		static std::once_flag counter_init_once;
		std::call_once(counter_init_once, []{ counter = 0; });

		if (!destroy_consumers_in_separate_thread())
			return;

		++counter;
		CASPAR_VERIFY(counter < 8);

		auto consumer = new std::shared_ptr<frame_consumer>(std::move(consumer_));
		std::thread([=]
		{
			std::unique_ptr<std::shared_ptr<frame_consumer>> pointer_guard(consumer);
			auto str = (*consumer)->print();

			try
			{
				ensure_gpf_handler_installed_for_thread(u8(L"Destroyer: " + str).c_str());

				if (!consumer->unique())
					CASPAR_LOG(debug) << str << L" Not destroyed on asynchronous destruction thread: " << consumer->use_count();
				else
					CASPAR_LOG(debug) << str << L" Destroying on asynchronous destruction thread.";
			}
			catch(...){}

			pointer_guard.reset();

		}).detach();
	}

	std::future<bool> send(const_frame frame) override																				{return consumer_->send(std::move(frame));}
	void initialize(const video_format_desc& format_desc, const audio_channel_layout& channel_layout, int channel_index) override	{return consumer_->initialize(format_desc, channel_layout, channel_index);}
	std::wstring print() const override																								{return consumer_->print();}
	std::wstring name() const override																								{return consumer_->name();}
	boost::property_tree::wptree info() const override 																				{return consumer_->info();}
	bool has_synchronization_clock() const override																					{return consumer_->has_synchronization_clock();}
	int buffer_depth() const override																								{return consumer_->buffer_depth();}
	int index() const override																										{return consumer_->index();}
	int64_t presentation_frame_age_millis() const override																			{return consumer_->presentation_frame_age_millis();}
	monitor::subject& monitor_output() override																						{return consumer_->monitor_output();}
	const frame_consumer* unwrapped() const override																				{return consumer_->unwrapped();}
};

class print_consumer_proxy : public frame_consumer
{
	std::shared_ptr<frame_consumer> consumer_;
public:
	print_consumer_proxy(spl::shared_ptr<frame_consumer>&& consumer)
		: consumer_(std::move(consumer))
	{
	}

	~print_consumer_proxy()
	{
		auto str = consumer_->print();
		CASPAR_LOG(debug) << str << L" Uninitializing.";
		consumer_.reset();
		CASPAR_LOG(info) << str << L" Uninitialized.";
	}

	std::future<bool> send(const_frame frame) override																				{return consumer_->send(std::move(frame));}
	void initialize(const video_format_desc& format_desc, const audio_channel_layout& channel_layout, int channel_index) override
	{
		consumer_->initialize(format_desc, channel_layout, channel_index);
		CASPAR_LOG(info) << consumer_->print() << L" Initialized.";
	}
	std::wstring print() const override																								{return consumer_->print();}
	std::wstring name() const override																								{return consumer_->name();}
	boost::property_tree::wptree info() const override 																				{return consumer_->info();}
	bool has_synchronization_clock() const override																					{return consumer_->has_synchronization_clock();}
	int buffer_depth() const override																								{return consumer_->buffer_depth();}
	int index() const override																										{return consumer_->index();}
	int64_t presentation_frame_age_millis() const override																			{return consumer_->presentation_frame_age_millis();}
	monitor::subject& monitor_output() override																						{return consumer_->monitor_output();}
	const frame_consumer* unwrapped() const override																				{return consumer_->unwrapped();}
};

class recover_consumer_proxy : public frame_consumer
{
	std::shared_ptr<frame_consumer> consumer_;
	int								channel_index_	= -1;
	video_format_desc				format_desc_;
	audio_channel_layout			channel_layout_	= audio_channel_layout::invalid();
public:
	recover_consumer_proxy(spl::shared_ptr<frame_consumer>&& consumer)
		: consumer_(std::move(consumer))
	{
	}

	std::future<bool> send(const_frame frame) override
	{
		try
		{
			return consumer_->send(frame);
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			try
			{
				consumer_->initialize(format_desc_, channel_layout_, channel_index_);
				return consumer_->send(frame);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				CASPAR_LOG(error) << print() << " Failed to recover consumer.";
				return make_ready_future(false);
			}
		}
	}

	void initialize(const video_format_desc& format_desc, const audio_channel_layout& channel_layout, int channel_index) override
	{
		format_desc_	= format_desc;
		channel_layout_ = channel_layout;
		channel_index_	= channel_index;
		return consumer_->initialize(format_desc, channel_layout, channel_index);
	}

	std::wstring print() const override										{return consumer_->print();}
	std::wstring name() const override										{return consumer_->name();}
	boost::property_tree::wptree info() const override 						{return consumer_->info();}
	bool has_synchronization_clock() const override							{return consumer_->has_synchronization_clock();}
	int buffer_depth() const override										{return consumer_->buffer_depth();}
	int index() const override												{return consumer_->index();}
	int64_t presentation_frame_age_millis() const override					{return consumer_->presentation_frame_age_millis();}
	monitor::subject& monitor_output() override								{return consumer_->monitor_output();}
	const frame_consumer* unwrapped() const override						{return consumer_->unwrapped();}
};

// This class is used to guarantee that audio cadence is correct. This is important for NTSC audio.
class cadence_guard : public frame_consumer
{
	spl::shared_ptr<frame_consumer>		consumer_;
	std::vector<int>					audio_cadence_;
	video_format_desc					format_desc_;
	audio_channel_layout				channel_layout_	= audio_channel_layout::invalid();
	boost::circular_buffer<std::size_t>	sync_buffer_;
public:
	cadence_guard(const spl::shared_ptr<frame_consumer>& consumer)
		: consumer_(consumer)
	{
	}

	void initialize(const video_format_desc& format_desc, const audio_channel_layout& channel_layout, int channel_index) override
	{
		audio_cadence_	= format_desc.audio_cadence;
		sync_buffer_	= boost::circular_buffer<std::size_t>(format_desc.audio_cadence.size());
		format_desc_	= format_desc;
		channel_layout_ = channel_layout;
		consumer_->initialize(format_desc, channel_layout, channel_index);
	}

	std::future<bool> send(const_frame frame) override
	{
		if(audio_cadence_.size() == 1)
			return consumer_->send(frame);

		std::future<bool> result = make_ready_future(true);

		if(boost::range::equal(sync_buffer_, audio_cadence_) && audio_cadence_.front() * channel_layout_.num_channels == static_cast<int>(frame.audio_data().size()))
		{
			// Audio sent so far is in sync, now we can send the next chunk.
			result = consumer_->send(frame);
			boost::range::rotate(audio_cadence_, std::begin(audio_cadence_)+1);
		}
		else
			CASPAR_LOG(trace) << print() << L" Syncing audio.";

		sync_buffer_.push_back(static_cast<int>(frame.audio_data().size() / channel_layout_.num_channels));

		return std::move(result);
	}

	std::wstring print() const override										{return consumer_->print();}
	std::wstring name() const override										{return consumer_->name();}
	boost::property_tree::wptree info() const override 						{return consumer_->info();}
	bool has_synchronization_clock() const override							{return consumer_->has_synchronization_clock();}
	int buffer_depth() const override										{return consumer_->buffer_depth();}
	int index() const override												{return consumer_->index();}
	int64_t presentation_frame_age_millis() const override					{return consumer_->presentation_frame_age_millis();}
	monitor::subject& monitor_output() override								{return consumer_->monitor_output();}
	const frame_consumer* unwrapped() const override						{return consumer_->unwrapped();}
};

spl::shared_ptr<core::frame_consumer> frame_consumer_registry::create_consumer(
		const std::vector<std::wstring>& params, interaction_sink* sink, std::vector<spl::shared_ptr<video_channel>> channels) const
{
	if(params.empty())
		CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info("params cannot be empty"));

	auto consumer = frame_consumer::empty();
	auto& consumer_factories = impl_->consumer_factories;
	std::any_of(consumer_factories.begin(), consumer_factories.end(), [&](const consumer_factory_t& factory) -> bool
		{
			try
			{
				consumer = factory(params, sink, channels);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
			return consumer != frame_consumer::empty();
		});

	if(consumer == frame_consumer::empty())
		CASPAR_THROW_EXCEPTION(file_not_found() << msg_info("No match found for supplied commands. Check syntax."));

	return spl::make_shared<destroy_consumer_proxy>(
			spl::make_shared<print_consumer_proxy>(
			 spl::make_shared<recover_consumer_proxy>(
			  spl::make_shared<cadence_guard>(
			   std::move(consumer)))));
}

spl::shared_ptr<frame_consumer> frame_consumer_registry::create_consumer(
		const std::wstring& element_name,
		const boost::property_tree::wptree& element,
		interaction_sink* sink,
		std::vector<spl::shared_ptr<video_channel>> channels) const
{
	auto& preconfigured_consumer_factories = impl_->preconfigured_consumer_factories;
	auto found = preconfigured_consumer_factories.find(element_name);

	if (found == preconfigured_consumer_factories.end())
		CASPAR_THROW_EXCEPTION(user_error()
			<< msg_info(L"No consumer factory registered for element name " + element_name));

	return spl::make_shared<destroy_consumer_proxy>(
			spl::make_shared<print_consumer_proxy>(
					spl::make_shared<recover_consumer_proxy>(
							spl::make_shared<cadence_guard>(
									found->second(element, sink, channels)))));
}

const spl::shared_ptr<frame_consumer>& frame_consumer::empty()
{
	class empty_frame_consumer : public frame_consumer
	{
	public:
		std::future<bool> send(const_frame) override { return make_ready_future(false); }
		void initialize(const video_format_desc&, const audio_channel_layout&, int) override{}
		std::wstring print() const override {return L"empty";}
		std::wstring name() const override {return L"empty";}
		bool has_synchronization_clock() const override {return false;}
		int buffer_depth() const override {return 0;};
		int index() const override {return -1;}
		int64_t presentation_frame_age_millis() const override {return -1;}
		monitor::subject& monitor_output() override {static monitor::subject monitor_subject(""); return monitor_subject;}
		boost::property_tree::wptree info() const override
		{
			boost::property_tree::wptree info;
			info.add(L"type", L"empty");
			return info;
		}
	};
	static spl::shared_ptr<frame_consumer> consumer = spl::make_shared<empty_frame_consumer>();
	return consumer;
}

}}
