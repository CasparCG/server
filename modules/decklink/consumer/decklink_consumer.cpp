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

#include "decklink_consumer.h"

#include "../util/util.h"
#include "../decklink.h"

#include "../decklink_api.h"

#include <core/frame/frame.h>
#include <core/mixer/audio/audio_mixer.h>
#include <core/consumer/frame_consumer.h>
#include <core/diagnostics/call_context.h>

#include <common/executor.h>
#include <common/diagnostics/graph.h>
#include <common/except.h>
#include <common/memshfl.h>
#include <common/semaphore.h>
#include <common/array.h>
#include <common/future.h>
#include <common/timer.h>
#include <common/param.h>

#include <tbb/concurrent_queue.h>
#include <tbb/scalable_allocator.h>

#include <common/assert.h>
#include <boost/lexical_cast.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/property_tree/ptree.hpp>

#include <mutex>
#include <future>

namespace caspar { namespace decklink {

struct configuration
{
	enum class keyer_t
	{
		internal_keyer,
		external_keyer,
		external_separate_device_keyer,
		default_keyer					= external_keyer
	};

	enum class latency_t
	{
		low_latency,
		normal_latency,
		default_latency	= normal_latency
	};

	int							device_index		= 1;
	int							key_device_idx		= 0;
	bool						embedded_audio		= false;
	keyer_t						keyer				= keyer_t::default_keyer;
	latency_t					latency				= latency_t::default_latency;
	bool						key_only			= false;
	int							base_buffer_depth	= 3;

	int buffer_depth() const
	{
		return base_buffer_depth + (latency == latency_t::low_latency ? 0 : 1);
	}

	int key_device_index() const
	{
		return key_device_idx == 0 ? device_index + 1 : key_device_idx;
	}
};

template <typename Configuration>
void set_latency(
		const com_iface_ptr<Configuration>& config,
		configuration::latency_t latency,
		const std::wstring& print)
{
	if (latency == configuration::latency_t::low_latency)
	{
		config->SetFlag(bmdDeckLinkConfigLowLatencyVideoOutput, true);
		CASPAR_LOG(info) << print << L" Enabled low-latency mode.";
	}
	else if (latency == configuration::latency_t::normal_latency)
	{
		config->SetFlag(bmdDeckLinkConfigLowLatencyVideoOutput, false);
		CASPAR_LOG(info) << print << L" Disabled low-latency mode.";
	}
}

void set_keyer(
		const com_iface_ptr<IDeckLinkAttributes>& attributes,
		const com_iface_ptr<IDeckLinkKeyer>& decklink_keyer,
		configuration::keyer_t keyer,
		const std::wstring& print)
{
	if (keyer == configuration::keyer_t::internal_keyer)
	{
		BOOL value = true;
		if (SUCCEEDED(attributes->GetFlag(BMDDeckLinkSupportsInternalKeying, &value)) && !value)
			CASPAR_LOG(error) << print << L" Failed to enable internal keyer.";
		else if (FAILED(decklink_keyer->Enable(FALSE)))
			CASPAR_LOG(error) << print << L" Failed to enable internal keyer.";
		else if (FAILED(decklink_keyer->SetLevel(255)))
			CASPAR_LOG(error) << print << L" Failed to set key-level to max.";
		else
			CASPAR_LOG(info) << print << L" Enabled internal keyer.";
	}
	else if (keyer == configuration::keyer_t::external_keyer)
	{
		BOOL value = true;
		if (SUCCEEDED(attributes->GetFlag(BMDDeckLinkSupportsExternalKeying, &value)) && !value)
			CASPAR_LOG(error) << print << L" Failed to enable external keyer.";
		else if (FAILED(decklink_keyer->Enable(TRUE)))
			CASPAR_LOG(error) << print << L" Failed to enable external keyer.";
		else if (FAILED(decklink_keyer->SetLevel(255)))
			CASPAR_LOG(error) << print << L" Failed to set key-level to max.";
		else
			CASPAR_LOG(info) << print << L" Enabled external keyer.";
	}
}

class decklink_frame : public IDeckLinkVideoFrame
{
	std::atomic<int>								ref_count_;
	core::const_frame								frame_;
	const core::video_format_desc					format_desc_;

	const bool										key_only_;
	void*											data_;
public:
	decklink_frame(core::const_frame frame, const core::video_format_desc& format_desc, bool key_only)
		: frame_(frame)
		, format_desc_(format_desc)
		, key_only_(key_only)
		, data_(scalable_aligned_malloc(format_desc_.size, 64))
	{
		ref_count_ = 0;
	}

	~decklink_frame()
	{
		scalable_aligned_free(data_);
	}

	// IUnknown

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID*)
	{
		return E_NOINTERFACE;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef()
	{
		return ++ref_count_;
	}

	virtual ULONG STDMETHODCALLTYPE Release()
	{
		if(--ref_count_ == 0)
		{
			delete this;

			return 0;
		}

		return ref_count_;
	}

	// IDecklinkVideoFrame

	virtual long STDMETHODCALLTYPE GetWidth()                   {return static_cast<long>(format_desc_.width);}
	virtual long STDMETHODCALLTYPE GetHeight()                  {return static_cast<long>(format_desc_.height);}
	virtual long STDMETHODCALLTYPE GetRowBytes()                {return static_cast<long>(format_desc_.width*4);}
	virtual BMDPixelFormat STDMETHODCALLTYPE GetPixelFormat()   {return bmdFormat8BitBGRA;}
	virtual BMDFrameFlags STDMETHODCALLTYPE GetFlags()			{return bmdFrameFlagDefault;}

	virtual HRESULT STDMETHODCALLTYPE GetBytes(void** buffer)
	{
		try {
			if (static_cast<int>(frame_.image_data().size()) != format_desc_.size) {
				std::memset(data_, 0, format_desc_.size);
			} else if(key_only_) {
				aligned_memshfl(data_, frame_.image_data().begin(), format_desc_.size, 0x0F0F0F0F, 0x0B0B0B0B, 0x07070707, 0x03030303);
			} else {
				std::memcpy(data_, frame_.image_data().begin(), format_desc_.size);
			}
			*buffer = data_;
		} catch(...) {
			CASPAR_LOG_CURRENT_EXCEPTION();
			return E_FAIL;
		}

		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetTimecode(BMDTimecodeFormat format, IDeckLinkTimecode** timecode) {return S_FALSE;}
	virtual HRESULT STDMETHODCALLTYPE GetAncillaryData(IDeckLinkVideoFrameAncillary** ancillary)		  {return S_FALSE;}

	// decklink_frame

	const core::audio_buffer& audio_data()
	{
		return frame_.audio_data();
	}

	int64_t get_age_millis() const
	{
		return frame_.get_age_millis();
	}
};

template <typename Configuration>
struct key_video_context : public IDeckLinkVideoOutputCallback, boost::noncopyable
{
	const configuration					config_;
	com_ptr<IDeckLink>					decklink_					= get_device(config_.key_device_index());
	com_iface_ptr<IDeckLinkOutput>		output_						= iface_cast<IDeckLinkOutput>(decklink_);
	com_iface_ptr<IDeckLinkKeyer>		keyer_						= iface_cast<IDeckLinkKeyer>(decklink_, true);
	com_iface_ptr<IDeckLinkAttributes>	attributes_					= iface_cast<IDeckLinkAttributes>(decklink_);
	com_iface_ptr<Configuration>		configuration_				= iface_cast<Configuration>(decklink_);
	std::atomic<int64_t>				current_presentation_delay_;
	std::atomic<int64_t>				scheduled_frames_completed_;

	key_video_context(const configuration& config, const std::wstring& print)
		: config_(config)
	{
		current_presentation_delay_ = 0;
		scheduled_frames_completed_ = 0;

		set_latency(configuration_, config.latency, print);
		set_keyer(attributes_, keyer_, config.keyer, print);

		if (FAILED(output_->SetScheduledFrameCompletionCallback(this)))
			CASPAR_THROW_EXCEPTION(caspar_exception()
					<< msg_info(print + L" Failed to set key playback completion callback.")
					<< boost::errinfo_api_function("SetScheduledFrameCompletionCallback"));
	}

	template<typename Print>
	void enable_video(BMDDisplayMode display_mode, const Print& print)
	{
		if (FAILED(output_->EnableVideoOutput(display_mode, bmdVideoOutputFlagDefault)))
			CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Could not enable key video output."));

		if (FAILED(output_->SetScheduledFrameCompletionCallback(this)))
			CASPAR_THROW_EXCEPTION(caspar_exception()
					<< msg_info(print() + L" Failed to set key playback completion callback.")
					<< boost::errinfo_api_function("SetScheduledFrameCompletionCallback"));
	}

	virtual ~key_video_context()
	{
		if (output_)
		{
			output_->StopScheduledPlayback(0, nullptr, 0);
			output_->DisableVideoOutput();
		}
	}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID*)	{return E_NOINTERFACE;}
	virtual ULONG STDMETHODCALLTYPE AddRef()							{return 1;}
	virtual ULONG STDMETHODCALLTYPE Release()							{return 1;}

	virtual HRESULT STDMETHODCALLTYPE ScheduledPlaybackHasStopped()
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ScheduledFrameCompleted(
			IDeckLinkVideoFrame* completed_frame,
			BMDOutputFrameCompletionResult result)
	{
		auto dframe = reinterpret_cast<decklink_frame*>(completed_frame);
		current_presentation_delay_ = dframe->get_age_millis();
		++scheduled_frames_completed_;

		// Let the fill callback keep the pace, so no scheduling here.

		return S_OK;
	}
};

template <typename Configuration>
struct decklink_consumer : public IDeckLinkVideoOutputCallback, boost::noncopyable
{
	const int											channel_index_;
	const configuration									config_;

	com_ptr<IDeckLink>									decklink_				= get_device(config_.device_index);
	com_iface_ptr<IDeckLinkOutput>						output_					= iface_cast<IDeckLinkOutput>(decklink_);
	com_iface_ptr<Configuration>						configuration_			= iface_cast<Configuration>(decklink_);
	com_iface_ptr<IDeckLinkKeyer>						keyer_					= iface_cast<IDeckLinkKeyer>(decklink_, true);
	com_iface_ptr<IDeckLinkAttributes>					attributes_				= iface_cast<IDeckLinkAttributes>(decklink_);

	std::mutex   										exception_mutex_;
	std::exception_ptr									exception_;

	std::atomic<bool>									is_running_;

	const std::wstring									model_name_				= get_model_name(decklink_);
	const core::video_format_desc						format_desc_;
	const int											buffer_size_			= config_.buffer_depth(); // Minimum buffer-size 3.

	long long											video_scheduled_		= 0;
	long long											audio_scheduled_		= 0;

	int													preroll_count_			= 0;

	boost::circular_buffer<std::vector<int32_t>>		audio_container_		{ static_cast<unsigned long>(buffer_size_ + 1) };

	tbb::concurrent_bounded_queue<core::const_frame>	frame_buffer_;
	caspar::semaphore									ready_for_new_frames_	{ 0 };

	spl::shared_ptr<diagnostics::graph>					graph_;
	caspar::timer										tick_timer_;
	reference_signal_detector							reference_signal_detector_	{ output_ };
	std::atomic<int64_t>								current_presentation_delay_;
	std::atomic<int64_t>								scheduled_frames_completed_;
	std::unique_ptr<key_video_context<Configuration>>	key_context_;

public:
	decklink_consumer(
			const configuration& config,
			const core::video_format_desc& format_desc,
			int channel_index)
		: channel_index_(channel_index)
		, config_(config)
		, format_desc_(format_desc)
	{
		is_running_ = true;
		current_presentation_delay_ = 0;
		scheduled_frames_completed_ = 0;

		frame_buffer_.set_capacity(1);

		if (config.keyer == configuration::keyer_t::external_separate_device_keyer)
			key_context_.reset(new key_video_context<Configuration>(config, print()));

		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
		graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
		graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
		graph_->set_color("flushed-frame", diagnostics::color(0.4f, 0.3f, 0.8f));
		graph_->set_color("buffered-audio", diagnostics::color(0.9f, 0.9f, 0.5f));
		graph_->set_color("buffered-video", diagnostics::color(0.2f, 0.9f, 0.9f));

		if (key_context_)
		{
			graph_->set_color("key-offset", diagnostics::color(1.0f, 0.0f, 0.0f));
		}

		graph_->set_text(print());
		diagnostics::register_graph(graph_);

		enable_video(get_display_mode(output_, format_desc_.format, bmdFormat8BitBGRA, bmdVideoOutputFlagDefault));

		if(config.embedded_audio)
			enable_audio();

		set_latency(configuration_, config.latency, print());
		set_keyer(attributes_, keyer_, config.keyer, print());

		if(config.embedded_audio)
			output_->BeginAudioPreroll();

		for (int n = 0; n < buffer_size_; ++n)
		{
			if (config.embedded_audio)
				schedule_next_audio(core::mutable_audio_buffer(format_desc_.audio_cadence[n % format_desc_.audio_cadence.size()] * format_desc_.audio_channels, 0));

			schedule_next_video(core::const_frame::empty());
		}

		if (config.embedded_audio)
		{
			// Preroll one extra frame worth of audio
			schedule_next_audio(core::mutable_audio_buffer(format_desc_.audio_cadence[buffer_size_ % format_desc_.audio_cadence.size()] * format_desc_.audio_channels, 0));
			output_->EndAudioPreroll();
		}

		start_playback();
	}

	~decklink_consumer()
	{
		is_running_ = false;
		frame_buffer_.try_push(core::const_frame::empty());

		if(output_ != nullptr)
		{
			output_->StopScheduledPlayback(0, nullptr, 0);
			if(config_.embedded_audio)
				output_->DisableAudioOutput();
			output_->DisableVideoOutput();
		}
	}

	void enable_audio()
	{
		if(FAILED(output_->EnableAudioOutput(bmdAudioSampleRate48kHz, bmdAudioSampleType32bitInteger, format_desc_.audio_channels, bmdAudioOutputStreamTimestamped)))
				CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Could not enable audio output."));

		CASPAR_LOG(info) << print() << L" Enabled embedded-audio.";
	}

	void enable_video(BMDDisplayMode display_mode)
	{
		if(FAILED(output_->EnableVideoOutput(display_mode, bmdVideoOutputFlagDefault)))
			CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Could not enable fill video output."));

		if(FAILED(output_->SetScheduledFrameCompletionCallback(this)))
			CASPAR_THROW_EXCEPTION(caspar_exception()
									<< msg_info(print() + L" Failed to set fill playback completion callback.")
									<< boost::errinfo_api_function("SetScheduledFrameCompletionCallback"));

		if (key_context_)
			key_context_->enable_video(display_mode, [this]() { return print(); });
	}

	void start_playback()
	{
		if(FAILED(output_->StartScheduledPlayback(0, format_desc_.time_scale, 1.0)))
			CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to schedule fill playback."));

		if (key_context_ && FAILED(key_context_->output_->StartScheduledPlayback(0, format_desc_.time_scale, 1.0)))
			CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to schedule key playback."));
	}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, LPVOID*)	{return E_NOINTERFACE;}
	virtual ULONG STDMETHODCALLTYPE AddRef()					{return 1;}
	virtual ULONG STDMETHODCALLTYPE Release()				{return 1;}

	virtual HRESULT STDMETHODCALLTYPE ScheduledPlaybackHasStopped()
	{
		is_running_ = false;
		CASPAR_LOG(info) << print() << L" Scheduled playback has stopped.";
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ScheduledFrameCompleted(IDeckLinkVideoFrame* completed_frame, BMDOutputFrameCompletionResult result)
	{
		if(!is_running_)
			return E_FAIL;

		try
		{
			auto tick_time = tick_timer_.elapsed()*format_desc_.fps * 0.5;
			graph_->set_value("tick-time", tick_time);
			tick_timer_.restart();

			reference_signal_detector_.detect_change([this]() { return print(); });

			auto dframe = reinterpret_cast<decklink_frame*>(completed_frame);
			current_presentation_delay_ = dframe->get_age_millis();
			++scheduled_frames_completed_;

			if (key_context_)
				graph_->set_value(
						"key-offset",
						static_cast<double>(
								scheduled_frames_completed_
								- key_context_->scheduled_frames_completed_)
						* 0.1 + 0.5);

			if(result == bmdOutputFrameDisplayedLate)
			{
				graph_->set_tag(diagnostics::tag_severity::WARNING, "late-frame");
				video_scheduled_ += format_desc_.duration;
				audio_scheduled_ += dframe->audio_data().size() / format_desc_.audio_channels;
			}
			else if(result == bmdOutputFrameDropped)
				graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
			else if(result == bmdOutputFrameFlushed)
				graph_->set_tag(diagnostics::tag_severity::WARNING, "flushed-frame");

			UINT32 buffered;
			output_->GetBufferedVideoFrameCount(&buffered);
			graph_->set_value("buffered-video", static_cast<double>(buffered) / (config_.buffer_depth()));

			if (config_.embedded_audio)
			{
				output_->GetBufferedAudioSampleFrameCount(&buffered);
				graph_->set_value("buffered-audio", static_cast<double>(buffered) / (format_desc_.audio_cadence[0] * config_.buffer_depth()));
			}

			auto frame = core::const_frame::empty();

			if (!frame_buffer_.try_pop(frame)) {
				graph_->set_tag(diagnostics::tag_severity::WARNING, "underflow");
				frame_buffer_.pop(frame);
			}

			ready_for_new_frames_.release();

			if (!is_running_)
				return E_FAIL;

			if (config_.embedded_audio)
				schedule_next_audio(frame.audio_data());

			schedule_next_video(frame);
		}
		catch(...)
		{
            std::lock_guard<std::mutex> lock(exception_mutex_);
			exception_ = std::current_exception();
			return E_FAIL;
		}

		return S_OK;
	}

	template<typename T>
	void schedule_next_audio(const T& audio_data)
	{
		auto sample_frame_count = static_cast<int>(audio_data.size() / format_desc_.audio_channels);

		audio_container_.push_back(std::vector<int32_t>(audio_data.begin(), audio_data.end()));

		if(FAILED(output_->ScheduleAudioSamples(audio_container_.back().data(), sample_frame_count, audio_scheduled_, format_desc_.audio_sample_rate, nullptr)))
			CASPAR_LOG(error) << print() << L" Failed to schedule audio.";

		audio_scheduled_ += sample_frame_count;
	}

	void schedule_next_video(core::const_frame frame)
	{
		if (key_context_)
		{
			auto key_frame = wrap_raw<com_ptr, IDeckLinkVideoFrame>(new decklink_frame(frame, format_desc_, true));
			if (FAILED(key_context_->output_->ScheduleVideoFrame(get_raw(key_frame), video_scheduled_, format_desc_.duration, format_desc_.time_scale)))
				CASPAR_LOG(error) << print() << L" Failed to schedule key video.";
		}

		auto fill_frame = wrap_raw<com_ptr, IDeckLinkVideoFrame>(new decklink_frame(frame, format_desc_, config_.key_only));
		if (FAILED(output_->ScheduleVideoFrame(get_raw(fill_frame), video_scheduled_, format_desc_.duration, format_desc_.time_scale)))
			CASPAR_LOG(error) << print() << L" Failed to schedule fill video.";

		video_scheduled_ += format_desc_.duration;
	}

	std::future<bool> send(core::const_frame frame)
	{
        {
            std::lock_guard<std::mutex> lock(exception_mutex_);
            if (exception_ != nullptr)
                std::rethrow_exception(exception_);
        }

		if(!is_running_)
			CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Is not running."));

		frame_buffer_.push(frame);

		auto send_completion = spl::make_shared<std::promise<bool>>();

		ready_for_new_frames_.acquire(1, [send_completion]
		{
			send_completion->set_value(true);
		});

		return send_completion->get_future();
	}

	std::wstring print() const
	{
		if (config_.keyer == configuration::keyer_t::external_separate_device_keyer)
			return model_name_ + L" [" + boost::lexical_cast<std::wstring>(channel_index_)+L"-" +
				boost::lexical_cast<std::wstring>(config_.device_index) +
				L"&&" +
				boost::lexical_cast<std::wstring>(config_.key_device_index()) +
				L"|" +
				format_desc_.name + L"]";
		else
			return model_name_ + L" [" + boost::lexical_cast<std::wstring>(channel_index_)+L"-" +
				boost::lexical_cast<std::wstring>(config_.device_index) + L"|" + format_desc_.name + L"]";
	}
};

template <typename Configuration>
struct decklink_consumer_proxy : public core::frame_consumer
{
	core::monitor::subject								monitor_subject_;
	const configuration									config_;
	std::unique_ptr<decklink_consumer<Configuration>>	consumer_;
	core::video_format_desc								format_desc_;
	executor											executor_;
public:

	decklink_consumer_proxy(const configuration& config)
		: config_(config)
		, executor_(L"decklink_consumer[" + boost::lexical_cast<std::wstring>(config.device_index) + L"]")
	{
		auto ctx = core::diagnostics::call_context::for_thread();
		executor_.begin_invoke([=]
		{
			core::diagnostics::call_context::for_thread() = ctx;
			com_initialize();
		});
	}

	~decklink_consumer_proxy()
	{
		executor_.invoke([=]
		{
			consumer_.reset();
			com_uninitialize();
		});
	}

	// frame_consumer

	void initialize(const core::video_format_desc& format_desc, int channel_index) override
	{
		format_desc_ = format_desc;
		executor_.invoke([=]
		{
			consumer_.reset();
			consumer_.reset(new decklink_consumer<Configuration>(config_, format_desc, channel_index));
		});
	}

	std::future<bool> send(core::const_frame frame) override
	{
		return consumer_->send(frame);
	}

	std::wstring print() const override
	{
		return consumer_ ? consumer_->print() : L"[decklink_consumer]";
	}

	std::wstring name() const override
	{
		return L"decklink";
	}

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"decklink");
		info.add(L"key-only", config_.key_only);
		info.add(L"device", config_.device_index);

		if (config_.keyer == configuration::keyer_t::external_separate_device_keyer)
		{
			info.add(L"key-device", config_.key_device_index());
		}

		info.add(L"low-latency", config_.latency == configuration::latency_t::low_latency);
		info.add(L"embedded-audio", config_.embedded_audio);
		info.add(L"presentation-frame-age", presentation_frame_age_millis());
		//info.add(L"internal-key", config_.internal_key);
		return info;
	}

	int buffer_depth() const override
	{
		return config_.buffer_depth() + 2;
	}

	int index() const override
	{
		return 300 + config_.device_index;
	}

	int64_t presentation_frame_age_millis() const override
	{
		return consumer_ ? static_cast<int64_t>(consumer_->current_presentation_delay_) : 0;
	}

	core::monitor::subject& monitor_output()
	{
		return monitor_subject_;
	}
};

spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>& params, core::interaction_sink*, std::vector<spl::shared_ptr<core::video_channel>> channels)
{
	if (params.size() < 1 || !boost::iequals(params.at(0), L"DECKLINK"))
		return core::frame_consumer::empty();

	configuration config;

	if (params.size() > 1)
		config.device_index = boost::lexical_cast<int>(params.at(1));

	if (contains_param(L"INTERNAL_KEY", params))
		config.keyer = configuration::keyer_t::internal_keyer;
	else if (contains_param(L"EXTERNAL_KEY", params))
		config.keyer = configuration::keyer_t::external_keyer;
	else if (contains_param(L"EXTERNAL_SEPARATE_DEVICE_KEY", params))
		config.keyer = configuration::keyer_t::external_separate_device_keyer;
	else
		config.keyer = configuration::keyer_t::default_keyer;

	if (contains_param(L"LOW_LATENCY", params))
		config.latency = configuration::latency_t::low_latency;

	config.embedded_audio	= contains_param(L"EMBEDDED_AUDIO", params);
	config.key_only			= contains_param(L"KEY_ONLY", params);

	return spl::make_shared<decklink_consumer_proxy<IDeckLinkConfiguration>>(config);
}

spl::shared_ptr<core::frame_consumer> create_preconfigured_consumer(const boost::property_tree::wptree& ptree, core::interaction_sink*, std::vector<spl::shared_ptr<core::video_channel>> channels)
{
	configuration config;

	auto keyer = ptree.get(L"keyer", L"default");
	if(keyer == L"external")
		config.keyer = configuration::keyer_t::external_keyer;
	else if(keyer == L"internal")
		config.keyer = configuration::keyer_t::internal_keyer;
	else if (keyer == L"external_separate_device")
		config.keyer = configuration::keyer_t::external_separate_device_keyer;

	auto latency = ptree.get(L"latency", L"default");
	if(latency == L"low")
		config.latency = configuration::latency_t::low_latency;
	else if(latency == L"normal")
		config.latency = configuration::latency_t::normal_latency;

	config.key_only				= ptree.get(L"key-only",		config.key_only);
	config.device_index			= ptree.get(L"device",			config.device_index);
	config.key_device_idx		= ptree.get(L"key-device",		config.key_device_idx);
	config.embedded_audio		= ptree.get(L"embedded-audio",	config.embedded_audio);
	config.base_buffer_depth	= ptree.get(L"buffer-depth",	config.base_buffer_depth);

	return spl::make_shared<decklink_consumer_proxy<IDeckLinkConfiguration>>(config);
}

}}

/*
##############################################################################
Pre-rolling

Mail: 2011-05-09

Yoshan
BMD Developer Support
developer@blackmagic-design.com

-----------------------------------------------------------------------------

Thanks for your inquiry. The minimum number of frames that you can preroll
for scheduled playback is three frames for video and four frames for audio.
As you mentioned if you preroll less frames then playback will not start or
playback will be very sporadic. From our experience with Media Express, we
recommended that at least seven frames are prerolled for smooth playback.

Regarding the bmdDeckLinkConfigLowLatencyVideoOutput flag:
There can be around 3 frames worth of latency on scheduled output.
When the bmdDeckLinkConfigLowLatencyVideoOutput flag is used this latency is
reduced  or removed for scheduled playback. If the DisplayVideoFrameSync()
method is used, the bmdDeckLinkConfigLowLatencyVideoOutput setting will
guarantee that the provided frame will be output as soon the previous
frame output has been completed.
################################################################################
*/

/*
##############################################################################
Async DMA Transfer without redundant copying

Mail: 2011-05-10

Yoshan
BMD Developer Support
developer@blackmagic-design.com

-----------------------------------------------------------------------------

Thanks for your inquiry. You could try subclassing IDeckLinkMutableVideoFrame
and providing a pointer to your video buffer when GetBytes() is called.
This may help to keep copying to a minimum. Please ensure that the pixel
format is in bmdFormat10BitYUV, otherwise the DeckLink API / driver will
have to colourspace convert which may result in additional copying.
################################################################################
*/
