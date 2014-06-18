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
#include <core/parameters/parameters.h>
#include <core/video_format.h>
#include <core/mixer/read_frame.h>

#include <common/utility/assert.h>
#include <common/concurrency/executor.h>
#include <common/diagnostics/graph.h>

#include <boost/algorithm/string.hpp>
#include <boost/timer.hpp>

#include <tbb/atomic.h>

#include "../util/air_send.h"

namespace caspar { namespace newtek {

struct newtek_ivga_consumer : public core::frame_consumer
{
	std::shared_ptr<void>			air_send_;
	core::video_format_desc			format_desc_;
	core::channel_layout			channel_layout_;
	executor						executor_;
	tbb::atomic<bool>				connected_;

	safe_ptr<diagnostics::graph>	graph_;
	boost::timer					tick_timer_;
	boost::timer					frame_timer_;

public:

	newtek_ivga_consumer(core::channel_layout channel_layout)
		: executor_(print())
		, channel_layout_(channel_layout)
	{
		if (!airsend::is_available())
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(airsend::dll_name()) + " not available"));

		connected_ = false;

		graph_->set_text(print());
		graph_->set_color("frame-time", diagnostics::color(0.5f, 1.0f, 0.2f));
		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
		diagnostics::register_graph(graph_);
	}
	
	~newtek_ivga_consumer()
	{
	}

	// frame_consumer
	
	virtual void initialize(
			const core::video_format_desc& format_desc,
			const core::channel_layout& audio_channel_layout,
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
				channel_layout_.num_channels,
				format_desc.audio_sample_rate),
				airsend::destroy);

		CASPAR_VERIFY(air_send_);

		format_desc_ = format_desc;

		CASPAR_LOG(info) << print() << L" Successfully Initialized.";	
	}

	virtual boost::unique_future<bool> send(const safe_ptr<core::read_frame>& frame) override
	{
		CASPAR_VERIFY(format_desc_.height * format_desc_.width * 4 == static_cast<unsigned>(frame->image_data().size()));

		return executor_.begin_invoke([=]() -> bool
		{			
			graph_->set_value("tick-time", tick_timer_.elapsed() * format_desc_.fps * 0.5);
			tick_timer_.restart();
			frame_timer_.restart();

			// AUDIO

			auto audio_buffer = core::audio_32_to_16(
					core::get_rearranged_and_mixed(
							frame->multichannel_view(),
							channel_layout_,
							channel_layout_.num_channels));

			airsend::add_audio(air_send_.get(), audio_buffer.data(), audio_buffer.size() / channel_layout_.num_channels);

			// VIDEO

			connected_ = airsend::add_frame_bgra(air_send_.get(), frame->image_data().begin());

			graph_->set_text(print());
			graph_->set_value("frame-time", frame_timer_.elapsed() * format_desc_.fps * 0.5);
			
			return true;
		});
	}
		
	virtual std::wstring print() const override
	{
		return connected_ ?
				L"newtek-ivga[connected]" : L"newtek-ivga[not connected]";
	}

	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"newtek-ivga-consumer");
		info.add(L"connected", connected_ ? L"true" : L"false");
		return info;
	}

	virtual size_t buffer_depth() const override
	{
		return 0;
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
		return connected_;
	}
};	

safe_ptr<core::frame_consumer> create_ivga_consumer(const core::parameters& params)
{
	if(params.size() < 1 || params[0] != L"NEWTEK_IVGA")
		return core::frame_consumer::empty();
		
	const auto channel_layout = core::default_channel_layout_repository()
		.get_by_name(
			params.get(L"CHANNEL_LAYOUT", L"STEREO"));

	return make_safe<newtek_ivga_consumer>(channel_layout);
}

safe_ptr<core::frame_consumer> create_ivga_consumer(const boost::property_tree::wptree& ptree) 
{	
	const auto channel_layout =
		core::default_channel_layout_repository()
			.get_by_name(
				boost::to_upper_copy(ptree.get(L"channel-layout", L"STEREO")));

	return make_safe<newtek_ivga_consumer>(channel_layout);
}

}}