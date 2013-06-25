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
 
#include "decklink_consumer.h"

#include "../util/util.h"

#include "../interop/DeckLinkAPI_h.h"

#include <map>

#include <common/concurrency/com_context.h>
#include <common/concurrency/executor.h>
#include <common/diagnostics/graph.h>
#include <common/exception/exceptions.h>
#include <common/memory/memcpy.h>
#include <common/memory/memclr.h>
#include <common/utility/timer.h>

#include <core/parameters/parameters.h>
#include <core/consumer/frame_consumer.h>
#include <core/mixer/read_frame.h>
#include <core/mixer/audio/audio_util.h>

#include <tbb/cache_aligned_allocator.h>

#include <boost/timer.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/circular_buffer.hpp>

namespace caspar { namespace decklink { 

// second is the index in the array to consume the next time
typedef std::pair<std::vector<int32_t>, size_t> audio_buffer;

struct blocking_decklink_consumer : boost::noncopyable
{		
	const int								channel_index_;
	const configuration						config_;

	CComPtr<IDeckLink>						decklink_;
	CComQIPtr<IDeckLinkOutput>				output_;

	const std::wstring						model_name_;
	const core::video_format_desc			format_desc_;
	std::shared_ptr<core::read_frame>		previous_frame_;
	boost::circular_buffer<audio_buffer>	audio_samples_;
	size_t									buffered_audio_samples_;
	BMDTimeValue							last_reference_clock_value_;

	safe_ptr<diagnostics::graph>			graph_;
	boost::timer							frame_timer_;
	boost::timer							tick_timer_;
	boost::timer							sync_timer_;	
	reference_signal_detector				reference_signal_detector_;

	tbb::atomic<int64_t>					current_presentation_delay_;
	executor								executor_;

public:
	blocking_decklink_consumer(const configuration& config, const core::video_format_desc& format_desc, int channel_index) 
		: channel_index_(channel_index)
		, config_(config)
		, decklink_(get_device(config.device_index))
		, output_(decklink_)
		, model_name_(get_model_name(decklink_))
		, format_desc_(format_desc)
		, buffered_audio_samples_(0)
		, last_reference_clock_value_(-1)
		, reference_signal_detector_(output_)
		, executor_(L"blocking_decklink_consumer")
	{
		audio_samples_.set_capacity(2);
		current_presentation_delay_ = 0;
		executor_.set_capacity(1);

		graph_->set_color("sync-time", diagnostics::color(1.0f, 0.0f, 0.0f));
		graph_->set_color("frame-time", diagnostics::color(0.5f, 1.0f, 0.2f));
		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
		graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));

		if (config_.embedded_audio)
			graph_->set_color(
					"buffered-audio", diagnostics::color(0.9f, 0.9f, 0.5f));

		graph_->set_text(print());
		diagnostics::register_graph(graph_);
		
		enable_video(get_display_mode(output_, format_desc_.format, bmdFormat8BitBGRA, bmdVideoOutputFlagDefault));
				
		if(config.embedded_audio)
			enable_audio();

		set_latency(
				CComQIPtr<IDeckLinkConfiguration>(decklink_),
				configuration::low_latency,
				print());
		set_keyer(
				CComQIPtr<IDeckLinkAttributes>(decklink_),
				CComQIPtr<IDeckLinkKeyer>(decklink_),
				config.keyer,
				print());
	}

	~blocking_decklink_consumer()
	{		
		if(output_ != nullptr) 
		{
			if(config_.embedded_audio)
				output_->DisableAudioOutput();
			output_->DisableVideoOutput();
		}
	}
	
	void enable_audio()
	{
		if(FAILED(output_->EnableAudioOutput(bmdAudioSampleRate48kHz, bmdAudioSampleType32bitInteger, config_.num_out_channels(), bmdAudioOutputStreamContinuous)))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not enable audio output."));
				
		CASPAR_LOG(info) << print() << L" Enabled embedded-audio.";
	}

	void enable_video(BMDDisplayMode display_mode)
	{
		if(FAILED(output_->EnableVideoOutput(display_mode, bmdVideoOutputFlagDefault))) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not enable video output."));
	}

	void queue_audio_samples(const safe_ptr<core::read_frame>& frame)
	{
		auto view = frame->multichannel_view();
		const int sample_frame_count = view.num_samples();

		if (audio_samples_.full())
		{
			CASPAR_LOG(warning) << print() << L" Too much audio buffered. Discarding samples.";
			audio_samples_.clear();
			buffered_audio_samples_ = 0;
		}

		if (core::needs_rearranging(
				view, config_.audio_layout, config_.num_out_channels()))
		{
			std::vector<int32_t> resulting_audio_data;
			resulting_audio_data.resize(
					sample_frame_count * config_.num_out_channels());

			auto dest_view = core::make_multichannel_view<int32_t>(
					resulting_audio_data.begin(), 
					resulting_audio_data.end(),
					config_.audio_layout,
					config_.num_out_channels());

			core::rearrange_or_rearrange_and_mix(
					view, dest_view, core::default_mix_config_repository());

			if (config_.audio_layout.num_channels == 1) // mono
				boost::copy(                            // duplicate L to R
						dest_view.channel(0),
						dest_view.channel(1).begin());

			audio_samples_.push_back(
					std::make_pair(std::move(resulting_audio_data), 0));
		}
		else
		{
			audio_samples_.push_back(std::make_pair(
					std::vector<int32_t>(
							frame->audio_data().begin(),
							frame->audio_data().end()),
					0));
		}

		buffered_audio_samples_ += sample_frame_count;
		graph_->set_value("buffered-audio",
				static_cast<double>(buffered_audio_samples_)
						/ format_desc_.audio_cadence[0] * 0.5);
	}

	bool try_consume_audio(std::pair<std::vector<int32_t>, size_t>& buffer)
	{
		size_t to_offer = (buffer.first.size() - buffer.second)
				/ config_.num_out_channels();
		auto begin = buffer.first.data() + buffer.second;
		unsigned long samples_written;

		if (FAILED(output_->WriteAudioSamplesSync(
				begin,
				to_offer,
				&samples_written)))
			CASPAR_LOG(error) << print() << L" Failed to write audio samples.";

		buffered_audio_samples_ -= samples_written;

		if (samples_written == to_offer)
			return true; // depleted buffer

		size_t consumed = samples_written * config_.num_out_channels();
		buffer.second += consumed;


		return false;
	}

	void write_audio_samples()
	{
		while (!audio_samples_.empty())
		{
			auto buffer = audio_samples_.front();

			if (try_consume_audio(buffer))
				audio_samples_.pop_front();
			else
				break;
		}
	}

	void wait_for_frame_to_be_displayed()
	{
		sync_timer_.restart();
		BMDTimeScale time_scale = 1000;
		BMDTimeValue hardware_time;
		BMDTimeValue time_in_frame;
		BMDTimeValue ticks_per_frame;

		output_->GetHardwareReferenceClock(
				time_scale, &hardware_time, &time_in_frame, &ticks_per_frame);

		auto reference_clock_value = hardware_time - time_in_frame;
		auto frame_duration = static_cast<int>(1000 / format_desc_.fps);
		auto actual_duration = last_reference_clock_value_ == -1 
				? frame_duration
				: reference_clock_value - last_reference_clock_value_;

		if (std::abs(frame_duration - actual_duration) > 1)
			graph_->set_tag("late-frame");
		else
		{
			auto to_wait = ticks_per_frame - time_in_frame;
			high_prec_timer timer;
			timer.tick_millis(0);
			timer.tick_millis(static_cast<DWORD>(to_wait));
		}


		last_reference_clock_value_ = reference_clock_value;
		graph_->set_value("sync-time",
				sync_timer_.elapsed() * format_desc_.fps * 0.5);
	}

	void write_video_frame(
			const safe_ptr<core::read_frame>& frame,
			std::vector<uint8_t, tbb::cache_aligned_allocator<uint8_t>>&& key)
	{
		CComPtr<IDeckLinkVideoFrame> frame2;

		if (config_.key_only)
			frame2 = CComPtr<IDeckLinkVideoFrame>(
					new decklink_frame(frame, format_desc_, std::move(key)));
		else
			frame2 = CComPtr<IDeckLinkVideoFrame>(
					new decklink_frame(frame, format_desc_, config_.key_only));

		if (FAILED(output_->DisplayVideoFrameSync(frame2)))
			CASPAR_LOG(error) << print() << L" Failed to display video frame.";

		reference_signal_detector_.detect_change([this]() { return print(); });
	}

	boost::unique_future<bool> send(const safe_ptr<core::read_frame>& frame)
	{
		return executor_.begin_invoke([=]() -> bool
		{
			frame_timer_.restart();
			std::vector<uint8_t, tbb::cache_aligned_allocator<uint8_t>> key;

			if (config_.key_only)
				key = std::move(extract_key(frame));

			if (config_.embedded_audio)
			{
				write_audio_samples();
				queue_audio_samples(frame);
			}

			double frame_time = frame_timer_.elapsed();

			wait_for_frame_to_be_displayed();

			if (previous_frame_)
			{
				// According to SDK when DisplayVideoFrameSync is used in
				// combination with low latency, the next frame displayed should
				// be the submitted frame but it appears to be an additional 2
				// frame delay before it is sent on SDI.
				int adjustment = static_cast<int>(2000 / format_desc_.fps);

				current_presentation_delay_ =
						previous_frame_->get_age_millis() + adjustment;
			}

			previous_frame_ = frame;

			graph_->set_value(
					"tick-time",
					tick_timer_.elapsed() * format_desc_.fps * 0.5);
			tick_timer_.restart();

			frame_timer_.restart();
			write_video_frame(frame, std::move(key));

			if (config_.embedded_audio)
				write_audio_samples();

			frame_time += frame_timer_.elapsed();
			graph_->set_value(
					"frame-time", frame_time * format_desc_.fps * 0.5);

			return true;
		});
	}
	
	std::wstring print() const
	{
		return model_name_ + L" [" + boost::lexical_cast<std::wstring>(channel_index_) + L"-" +
			boost::lexical_cast<std::wstring>(config_.device_index) + L"|" +  format_desc_.name + L"]";
	}
};

struct blocking_decklink_consumer_proxy : public core::frame_consumer
{
	const configuration						config_;
	com_context<blocking_decklink_consumer>	context_;
	std::vector<size_t>						audio_cadence_;
	core::video_format_desc					format_desc_;
public:

	blocking_decklink_consumer_proxy(const configuration& config)
		: config_(config)
		, context_(L"blocking_decklink_consumer[" + boost::lexical_cast<std::wstring>(config.device_index) + L"]")
	{
	}

	~blocking_decklink_consumer_proxy()
	{
		if(context_)
		{
			auto str = print();
			context_.reset();
			CASPAR_LOG(info) << str << L" Successfully Uninitialized.";	
		}
	}

	// frame_consumer
	
	virtual void initialize(const core::video_format_desc& format_desc, int channel_index) override
	{
		context_.reset([&]{return new blocking_decklink_consumer(config_, format_desc, channel_index);});		
		audio_cadence_ = format_desc.audio_cadence;		
		format_desc_ = format_desc;

		CASPAR_LOG(info) << print() << L" Successfully Initialized.";	
	}
	
	virtual boost::unique_future<bool> send(const safe_ptr<core::read_frame>& frame) override
	{
		CASPAR_VERIFY(audio_cadence_.front() * frame->num_channels() == static_cast<size_t>(frame->audio_data().size()));
		boost::range::rotate(audio_cadence_, std::begin(audio_cadence_)+1);

		return context_->send(frame);
	}
	
	virtual std::wstring print() const override
	{
		return context_ ? context_->print() : L"[blocking_decklink_consumer]";
	}		

	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"decklink-consumer");
		info.add(L"key-only", config_.key_only);
		info.add(L"device", config_.device_index);
		info.add(L"embedded-audio", config_.embedded_audio);
		info.add(L"presentation-frame-age", presentation_frame_age_millis());
		//info.add(L"internal-key", config_.internal_key);
		return info;
	}

	virtual size_t buffer_depth() const override
	{
		// Should be 1 according to SDK when DisplayVideoFrameSync is used in
		// combination with low latency, but it does not seem so.
		return 3; 
	}

	virtual bool has_synchronization_clock() const override
	{
		return true;
	}

	virtual int index() const override
	{
		return 350 + config_.device_index;
	}

	virtual int64_t presentation_frame_age_millis() const
	{
		return context_ ? context_->current_presentation_delay_ : 0;
	}
};	

safe_ptr<core::frame_consumer> create_blocking_consumer(const core::parameters& params) 
{
	if(params.size() < 1 || params[0] != L"BLOCKING_DECKLINK")
		return core::frame_consumer::empty();
	
	configuration config;
		
	if(params.size() > 1)
		config.device_index = lexical_cast_or_default<int>(params[1], config.device_index);
	
	if(std::find(params.begin(), params.end(), L"INTERNAL_KEY")			!= params.end())
		config.keyer = configuration::internal_keyer;
	else if(std::find(params.begin(), params.end(), L"EXTERNAL_KEY")	!= params.end())
		config.keyer = configuration::external_keyer;
	else
		config.keyer = configuration::default_keyer;

	config.embedded_audio	= std::find(params.begin(), params.end(), L"EMBEDDED_AUDIO") != params.end();
	config.key_only			= std::find(params.begin(), params.end(), L"KEY_ONLY")		 != params.end();
	config.audio_layout		= core::default_channel_layout_repository().get_by_name(
			params.get(L"CHANNEL_LAYOUT", L"STEREO"));

	return make_safe<blocking_decklink_consumer_proxy>(config);
}

safe_ptr<core::frame_consumer> create_blocking_consumer(const boost::property_tree::wptree& ptree) 
{
	configuration config;

	auto keyer = ptree.get(L"keyer", L"external");
	if(keyer == L"external")
		config.keyer = configuration::external_keyer;
	else if(keyer == L"internal")
		config.keyer = configuration::internal_keyer;

	config.key_only				= ptree.get(L"key-only",		config.key_only);
	config.device_index			= ptree.get(L"device",			config.device_index);
	config.embedded_audio		= ptree.get(L"embedded-audio",	config.embedded_audio);
	config.audio_layout =
		core::default_channel_layout_repository().get_by_name(
				boost::to_upper_copy(ptree.get(L"channel-layout", L"STEREO")));

	return make_safe<blocking_decklink_consumer_proxy>(config);
}

}}
