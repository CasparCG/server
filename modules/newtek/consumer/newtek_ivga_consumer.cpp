/*
* Copyright 2013 NewTek
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
* Author: Robert Nagy, ronag@live.com
*/

#include "../StdAfx.h"

#include "newtek_ivga_consumer.h"

#include <core/consumer/frame_consumer.h>
#include <core/video_format.h>
#include <core/frame/frame.h>
#include <core/frame/audio_channel_layout.h>
#include <core/mixer/audio/audio_util.h>
#include <core/monitor/monitor.h>
#include <core/help/help_sink.h>
#include <core/help/help_repository.h>

#include <common/assert.h>
#include <common/executor.h>
#include <common/diagnostics/graph.h>
#include <common/timer.h>
#include <common/param.h>

#include <boost/algorithm/string.hpp>
#include <boost/timer.hpp>
#include <boost/property_tree/ptree.hpp>

#include <tbb/atomic.h>

#include "../util/air_send.h"

namespace caspar { namespace newtek {

struct newtek_ivga_consumer : public core::frame_consumer
{
	core::monitor::subject				monitor_subject_;
	std::shared_ptr<void>				air_send_;
	core::video_format_desc				format_desc_;
	core::audio_channel_layout			channel_layout_		= core::audio_channel_layout::invalid();
	executor							executor_;
	tbb::atomic<bool>					connected_;
	spl::shared_ptr<diagnostics::graph>	graph_;
	timer								tick_timer_;
	timer								frame_timer_;

public:

	newtek_ivga_consumer()
		: executor_(print())
	{
		if (!airsend::is_available())
			CASPAR_THROW_EXCEPTION(not_supported() << msg_info(airsend::dll_name() + L" not available"));

		connected_ = false;

		graph_->set_text(print());
		graph_->set_color("frame-time", diagnostics::color(0.5f, 1.0f, 0.2f));
		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
		graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
		diagnostics::register_graph(graph_);
	}

	~newtek_ivga_consumer()
	{
	}

	// frame_consumer

	virtual void initialize(
			const core::video_format_desc& format_desc,
			const core::audio_channel_layout& channel_layout,
			int channel_index) override
	{
		air_send_.reset(
			airsend::create(
				format_desc.width,
				format_desc.height,
				format_desc.time_scale,
				format_desc.duration,
				format_desc.field_mode == core::field_mode::progressive,
				static_cast<float>(format_desc.square_width) / static_cast<float>(format_desc.square_height),
				true,
				channel_layout.num_channels,
				format_desc.audio_sample_rate),
				airsend::destroy);

		CASPAR_VERIFY(air_send_);

		format_desc_	= format_desc;
		channel_layout_	= channel_layout;
	}

	std::future<bool> schedule_send(core::const_frame frame)
	{
		return executor_.begin_invoke([=]() -> bool
		{
			graph_->set_value("tick-time", tick_timer_.elapsed() * format_desc_.fps * 0.5);
			tick_timer_.restart();
			frame_timer_.restart();

			// AUDIO

			auto audio_buffer = core::audio_32_to_16(frame.audio_data());

			airsend::add_audio(air_send_.get(), audio_buffer.data(), static_cast<int>(audio_buffer.size()) / channel_layout_.num_channels);

			// VIDEO

			connected_ = airsend::add_frame_bgra(air_send_.get(), frame.image_data().begin());

			graph_->set_text(print());
			graph_->set_value("frame-time", frame_timer_.elapsed() * format_desc_.fps * 0.5);

			return true;
		});
	}

	virtual std::future<bool> send(core::const_frame frame) override
	{
		CASPAR_VERIFY(format_desc_.height * format_desc_.width * 4 == frame.image_data().size());

		if (executor_.size() > 0 || executor_.is_currently_in_task())
		{
			graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");

			return make_ready_future(true);
		}

		schedule_send(std::move(frame));

		return make_ready_future(true);
	}

	virtual core::monitor::subject& monitor_output() override
	{
		return monitor_subject_;
	}

	virtual std::wstring print() const override
	{
		return connected_ ?
				L"newtek-ivga[connected]" : L"newtek-ivga[not connected]";
	}

	virtual std::wstring name() const override
	{
		return L"newtek-ivga";
	}

	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"newtek-ivga-consumer");
		info.add(L"connected", connected_ ? L"true" : L"false");
		return info;
	}

	virtual int buffer_depth() const override
	{
		return -1;
	}

	virtual int index() const override
	{
		return 900;
	}

	virtual int64_t presentation_frame_age_millis() const override
	{
		return 0;
	}

	virtual bool has_synchronization_clock() const override
	{
		return false;
	}
};

void describe_ivga_consumer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"A consumer for streaming a channel to a NewTek TriCaster via iVGA/AirSend protocol.");
	sink.syntax(L"NEWTEK_IVGA");
	sink.para()->text(L"A consumer for streaming a channel to a NewTek TriCaster via iVGA/AirSend protocol.");
	sink.para()->text(L"Examples:");
	sink.example(L">> ADD 1 NEWTEK_IVGA");
}

spl::shared_ptr<core::frame_consumer> create_ivga_consumer(const std::vector<std::wstring>& params, core::interaction_sink*, std::vector<spl::shared_ptr<core::video_channel>> channels, spl::shared_ptr<core::consumer_delayed_responder> responder)
{
	if (params.size() < 1 || !boost::iequals(params.at(0), L"NEWTEK_IVGA"))
		return core::frame_consumer::empty();

	return spl::make_shared<newtek_ivga_consumer>();
}

spl::shared_ptr<core::frame_consumer> create_preconfigured_ivga_consumer(const boost::property_tree::wptree& ptree, core::interaction_sink*, std::vector<spl::shared_ptr<core::video_channel>> channels)
{
	return spl::make_shared<newtek_ivga_consumer>();
}

}}
