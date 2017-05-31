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
#include "../util/decklink_allocator.h"
#include "../decklink.h"

#include "../interop/DeckLinkAPI_h.h"

#include <core/mixer/read_frame.h>

#include <common/concurrency/com_context.h>
#include <common/concurrency/future_util.h>
#include <common/diagnostics/graph.h>
#include <common/exception/exceptions.h>
#include <common/exception/win32_exception.h>
#include <common/utility/assert.h>
#include <common/utility/software_version.h>

#include <core/parameters/parameters.h>
#include <core/consumer/frame_consumer.h>
#include <core/mixer/audio/audio_util.h>

#include <tbb/concurrent_queue.h>
#include <tbb/cache_aligned_allocator.h>

#include <boost/circular_buffer.hpp>
#include <boost/timer.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>

namespace caspar { namespace decklink { 

template<typename Output>
struct key_video_context
		: public IDeckLinkVideoOutputCallback, boost::noncopyable
{
	CComPtr<IDeckLink>										decklink_;
	CComQIPtr<Output>										output_;
	CComQIPtr<IDeckLinkKeyer>								keyer_;
	CComQIPtr<IDeckLinkAttributes>							attributes_;
	CComQIPtr<IDeckLinkConfiguration>						configuration_;
	const std::unique_ptr<thread_safe_decklink_allocator>&	allocator_;
	tbb::atomic<int64_t>									current_presentation_delay_;
	tbb::atomic<int64_t>									scheduled_frames_completed_;

	key_video_context(
			const configuration& config,
			const std::wstring& print,
			const std::unique_ptr<thread_safe_decklink_allocator>& allocator)
		: decklink_(get_device(config.key_device_index()))
		, output_(decklink_)
		, keyer_(decklink_)
		, attributes_(decklink_)
		, configuration_(decklink_)
		, allocator_(allocator)
	{
		current_presentation_delay_ = 0;
		scheduled_frames_completed_ = 0;

		set_latency(configuration_, config.latency, print);
		set_keyer(attributes_, keyer_, config.keyer, print);
		
		configuration_->SetFlag(bmdDeckLinkConfigUse1080pNotPsF, TRUE);

		if(FAILED(output_->SetScheduledFrameCompletionCallback(this)))
			BOOST_THROW_EXCEPTION(caspar_exception() 
									<< msg_info(narrow(print) + " Failed to set key playback completion callback.")
									<< boost::errinfo_api_function("SetScheduledFrameCompletionCallback"));
	}

	template<typename Print>
	void enable_video(BMDDisplayMode display_mode, const Print& print)
	{
		if (allocator_)
		{
			if (FAILED(output_->SetVideoOutputFrameMemoryAllocator(allocator_.get())))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not set key custom memory allocator."));
		}

		if(FAILED(output_->EnableVideoOutput(display_mode, bmdVideoOutputFlagDefault))) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not enable key video output."));
		
		if(FAILED(output_->SetScheduledFrameCompletionCallback(this)))
			BOOST_THROW_EXCEPTION(caspar_exception() 
									<< msg_info(narrow(print()) + " Failed to set key playback completion callback.")
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
	
	STDMETHOD (QueryInterface(REFIID, LPVOID*))	{return E_NOINTERFACE;}
	STDMETHOD_(ULONG, AddRef())					{return 1;}
	STDMETHOD_(ULONG, Release())				{return 1;}

	STDMETHOD(ScheduledPlaybackHasStopped())
	{
		return S_OK;
	}

	STDMETHOD(ScheduledFrameCompleted(
			IDeckLinkVideoFrame* completed_frame,
			BMDOutputFrameCompletionResult result))
	{
		auto dframe = reinterpret_cast<decklink_frame*>(completed_frame);
		current_presentation_delay_ = dframe->get_age_millis();
		++scheduled_frames_completed_;

		// Let the fill callback keep the pace, so no scheduling here.

		return S_OK;
	}
};

template<typename Output>
struct decklink_consumer : public IDeckLinkVideoOutputCallback, public IDeckLinkAudioOutputCallback, boost::noncopyable
{
	const int										channel_index_;
	const configuration								config_;

	std::unique_ptr<thread_safe_decklink_allocator>	allocator_;
	CComPtr<IDeckLink>								decklink_;
	CComQIPtr<Output>								output_;
	CComQIPtr<IDeckLinkKeyer>						keyer_;
	CComQIPtr<IDeckLinkAttributes>					attributes_;
	CComQIPtr<IDeckLinkConfiguration>				configuration_;

	tbb::spin_mutex									exception_mutex_;
	std::exception_ptr								exception_;

	tbb::atomic<bool>								is_running_;
		
	const std::wstring								model_name_;
	const core::video_format_desc					format_desc_;
	const size_t									buffer_size_;

	long long										video_scheduled_;
	long long										audio_scheduled_;

	size_t											preroll_count_;
		
	boost::circular_buffer<std::vector<int32_t, tbb::cache_aligned_allocator<int32_t>>>	audio_container_;

	tbb::concurrent_bounded_queue<std::shared_ptr<core::read_frame>> video_frame_buffer_;
	tbb::concurrent_bounded_queue<std::shared_ptr<core::read_frame>> audio_frame_buffer_;
	
	safe_ptr<diagnostics::graph> graph_;
	boost::timer tick_timer_;
	retry_task<bool> send_completion_;
	reference_signal_detector<Output> reference_signal_detector_;

	tbb::atomic<int64_t>						current_presentation_delay_;
	tbb::atomic<int64_t>						scheduled_frames_completed_;
	std::unique_ptr<key_video_context<Output>>	key_context_;

public:
	decklink_consumer(
			const configuration& config,
			const core::video_format_desc& format_desc,
			int channel_index) 
		: channel_index_(channel_index)
		, config_(config)
		, decklink_(get_device(config.device_index))
		, output_(decklink_)
		, keyer_(decklink_)
		, attributes_(decklink_)
		, configuration_(decklink_)
		, model_name_(get_model_name(decklink_))
		, format_desc_(format_desc)
		, buffer_size_(config.buffer_depth()) // Minimum buffer-size 3.
		, video_scheduled_(0)
		, audio_scheduled_(0)
		, preroll_count_(0)
		, audio_container_(buffer_size_+1)
		, reference_signal_detector_(output_)
	{
		is_running_ = true;
		current_presentation_delay_ = 0;
		scheduled_frames_completed_ = 0;
				
		video_frame_buffer_.set_capacity(1);

		if (format_desc.fps > 50.0)
			// Blackmagic calls RenderAudioSamples() 50 times per second
			// regardless of video mode so we sometimes need to give them
			// samples from 2 frames in order to keep up
			audio_frame_buffer_.set_capacity(2); 
		else
			audio_frame_buffer_.set_capacity(1);

		if (config.keyer == configuration::external_separate_device_keyer)
			key_context_.reset(new key_video_context<Output>(config, print(), allocator_));

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
		
		for(size_t n = 0; n < buffer_size_; ++n)
			schedule_next_video(make_safe<core::read_frame>());

		if(!config.embedded_audio)
			start_playback();
	}

	~decklink_consumer()
	{		
		is_running_ = false;
		video_frame_buffer_.try_push(std::make_shared<core::read_frame>());
		audio_frame_buffer_.try_push(std::make_shared<core::read_frame>());

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
		if(FAILED(output_->EnableAudioOutput(bmdAudioSampleRate48kHz, bmdAudioSampleType32bitInteger, config_.num_out_channels(), bmdAudioOutputStreamTimestamped)))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not enable audio output."));
				
		if(FAILED(output_->SetAudioCallback(this)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not set audio callback."));

		CASPAR_LOG(info) << print() << L" Enabled embedded-audio.";
	}

	void enable_video(BMDDisplayMode display_mode)
	{
		if (config_.custom_allocator)
		{
			allocator_.reset(new thread_safe_decklink_allocator(print()));

			if (FAILED(output_->SetVideoOutputFrameMemoryAllocator(allocator_.get())))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not set fill custom memory allocator."));

			CASPAR_LOG(info) << print() << L" Using custom allocator.";
		}

		if(FAILED(output_->EnableVideoOutput(display_mode, bmdVideoOutputFlagDefault))) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not enable fill video output."));
		
		if(FAILED(output_->SetScheduledFrameCompletionCallback(this)))
			BOOST_THROW_EXCEPTION(caspar_exception() 
									<< msg_info(narrow(print()) + " Failed to set fill playback completion callback.")
									<< boost::errinfo_api_function("SetScheduledFrameCompletionCallback"));

		if (key_context_)
			key_context_->enable_video(display_mode, [this]() { return print(); });
	}

	void start_playback()
	{
		if(FAILED(output_->StartScheduledPlayback(0, format_desc_.time_scale, 1.0))) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to schedule fill playback."));

		if(key_context_ && FAILED(key_context_->output_->StartScheduledPlayback(0, format_desc_.time_scale, 1.0))) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to schedule key playback."));
	}
	
	STDMETHOD (QueryInterface(REFIID, LPVOID*))	{return E_NOINTERFACE;}
	STDMETHOD_(ULONG, AddRef())					{return 1;}
	STDMETHOD_(ULONG, Release())				{return 1;}
	
	STDMETHOD(ScheduledPlaybackHasStopped())
	{
		is_running_ = false;
		CASPAR_LOG(info) << print() << L" Scheduled playback has stopped.";
		return S_OK;
	}

	STDMETHOD(ScheduledFrameCompleted(IDeckLinkVideoFrame* completed_frame, BMDOutputFrameCompletionResult result))
	{
		win32_exception::ensure_handler_installed_for_thread("decklink-ScheduledFrameCompleted");
		if(!is_running_)
			return E_FAIL;
		
		try
		{
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
				graph_->set_tag("late-frame");
				video_scheduled_ += format_desc_.duration;
				audio_scheduled_ += dframe->audio_data().size()/config_.num_out_channels();
				//++video_scheduled_;
				//audio_scheduled_ += format_desc_.audio_cadence[0];
				//++audio_scheduled_;
			}
			else if(result == bmdOutputFrameDropped)
				graph_->set_tag("dropped-frame");
			else if(result == bmdOutputFrameFlushed)
				graph_->set_tag("flushed-frame");

			std::shared_ptr<core::read_frame> frame;	
			video_frame_buffer_.pop(frame);
			send_completion_.try_completion();
			schedule_next_video(make_safe_ptr(frame));	
			
			unsigned long buffered;
			output_->GetBufferedVideoFrameCount(&buffered);
			graph_->set_value("buffered-video", static_cast<double>(buffered)/format_desc_.fps);
		}
		catch(...)
		{
			tbb::spin_mutex::scoped_lock lock(exception_mutex_);
			exception_ = std::current_exception();
			return E_FAIL;
		}

		return S_OK;
	}
		
	STDMETHOD(RenderAudioSamples(BOOL preroll))
	{
		win32_exception::ensure_handler_installed_for_thread("decklink-RenderAudioSamples");

		if(!is_running_)
			return E_FAIL;
		
		try
		{	
			if(preroll)
			{
				if(++preroll_count_ >= buffer_size_)
				{
					output_->EndAudioPreroll();
					start_playback();				
				}
				else
				{
					core::audio_buffer silent_audio(format_desc_.audio_cadence[preroll_count_ % format_desc_.audio_cadence.size()] * config_.num_out_channels(), 0);
					auto view = core::make_multichannel_view<int32_t>(silent_audio.begin(), silent_audio.end(), config_.audio_layout, config_.num_out_channels());
					schedule_next_audio(view);
				}
			}
			else
			{
				std::shared_ptr<core::read_frame> frame;

				while (audio_frame_buffer_.try_pop(frame))
				{
					send_completion_.try_completion();
					schedule_next_audio(frame->multichannel_view());
				}
			}

			unsigned long buffered;
			output_->GetBufferedAudioSampleFrameCount(&buffered);
			graph_->set_value("buffered-audio", static_cast<double>(buffered)/(format_desc_.audio_cadence[0] * config_.num_out_channels() * 2));
		}
		catch(...)
		{
			tbb::spin_mutex::scoped_lock lock(exception_mutex_);
			exception_ = std::current_exception();
			return E_FAIL;
		}

		return S_OK;
	}

	template<typename View>
	void schedule_next_audio(const View& view)
	{
		const int sample_frame_count = view.num_samples();

		audio_container_.push_back(core::get_rearranged_and_mixed(
				view,
				config_.audio_layout,
				config_.num_out_channels()));

		if(FAILED(output_->ScheduleAudioSamples(
				audio_container_.back().data(),
				sample_frame_count,
				audio_scheduled_,
				format_desc_.audio_sample_rate,
				nullptr)))
			CASPAR_LOG(error) << print() << L" Failed to schedule audio.";

		audio_scheduled_ += sample_frame_count;
	}
			
	void schedule_next_video(const safe_ptr<core::read_frame>& frame)
	{
		if (key_context_)
		{
			CComPtr<IDeckLinkVideoFrame> key_frame(new decklink_frame(frame, format_desc_, true));
			if(FAILED(key_context_->output_->ScheduleVideoFrame(key_frame, video_scheduled_, format_desc_.duration, format_desc_.time_scale)))
				CASPAR_LOG(error) << print() << L" Failed to schedule key video.";
		}

		CComPtr<IDeckLinkVideoFrame> fill_frame(new decklink_frame(frame, format_desc_, config_.key_only));
		if(FAILED(output_->ScheduleVideoFrame(fill_frame, video_scheduled_, format_desc_.duration, format_desc_.time_scale)))
			CASPAR_LOG(error) << print() << L" Failed to schedule fill video.";

		video_scheduled_ += format_desc_.duration;

		graph_->set_value("tick-time", tick_timer_.elapsed()*format_desc_.fps*0.5);
		tick_timer_.restart();

		reference_signal_detector_.detect_change([this]() { return print(); });
	}

	boost::unique_future<bool> send(const safe_ptr<core::read_frame>& frame)
	{
		{
			tbb::spin_mutex::scoped_lock lock(exception_mutex_);
			if(exception_ != nullptr)
				std::rethrow_exception(exception_);
		}

		if(!is_running_)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Is not running."));

		bool audio_ready = !config_.embedded_audio;
		bool video_ready = false;

		auto enqueue_task = [audio_ready, video_ready, frame, this]() mutable -> boost::optional<bool>
		{
			if (!audio_ready)
				audio_ready = audio_frame_buffer_.try_push(frame);

			if (!video_ready)
				video_ready = video_frame_buffer_.try_push(frame);

			if (audio_ready && video_ready)
				return true;
			else
				return boost::optional<bool>();
		};
		
		if (enqueue_task())
			return wrap_as_future(true);

		send_completion_.set_task(enqueue_task);

		return send_completion_.get_future();
	}
	
	std::wstring print() const
	{
		if (config_.keyer == configuration::external_separate_device_keyer)
			return model_name_ + L" [" + boost::lexical_cast<std::wstring>(channel_index_) + L"-" +
				boost::lexical_cast<std::wstring>(config_.device_index) +
				L"&&" +
				boost::lexical_cast<std::wstring>(config_.key_device_index()) +
				L"|" +
				format_desc_.name + L"]";
		else
			return model_name_ + L" [" + boost::lexical_cast<std::wstring>(channel_index_) + L"-" +
				boost::lexical_cast<std::wstring>(config_.device_index) + L"|" +  format_desc_.name + L"]";
	}
};

template<typename Output>
struct decklink_consumer_proxy : public core::frame_consumer
{
	const configuration						config_;
	com_context<decklink_consumer<Output>>	context_;
	std::vector<size_t>						audio_cadence_;
	core::video_format_desc					format_desc_;
public:

	decklink_consumer_proxy(const configuration& config)
		: config_(config)
		, context_(L"decklink_consumer[" + boost::lexical_cast<std::wstring>(config.device_index) + L"]")
	{
	}

	~decklink_consumer_proxy()
	{
		if(context_)
		{
			auto str = print();
			context_.reset();
			CASPAR_LOG(info) << str << L" Successfully Uninitialized.";	
		}
	}

	// frame_consumer
	
	virtual void initialize(
			const core::video_format_desc& format_desc,
			const core::channel_layout& audio_channel_layout,
			int channel_index) override
	{
		context_.reset([&]{return new decklink_consumer<Output>(config_, format_desc, channel_index);});

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
		return context_ ? context_->print() : L"[decklink_consumer]";
	}		

	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"decklink-consumer");
		info.add(L"key-only", config_.key_only);
		info.add(L"device", config_.device_index);

		if (config_.keyer == configuration::external_separate_device_keyer)
		{
			info.add(L"key-device", config_.key_device_index());
		}

		info.add(L"low-latency", config_.latency == configuration::low_latency);
		info.add(L"embedded-audio", config_.embedded_audio);
		info.add(L"presentation-frame-age", presentation_frame_age_millis());
		//info.add(L"internal-key", config_.internal_key);
		return info;
	}

	virtual int buffer_depth() const override
	{
		return config_.buffer_depth();
	}

	virtual int index() const override
	{
		return 300 + config_.device_index;
	}

	virtual int64_t presentation_frame_age_millis() const
	{
		return context_ ? context_->current_presentation_delay_ : 0;
	}
};

const software_version<3>& get_driver_version()
{
	static software_version<3> version(narrow(get_version()));

	return version;
}

const software_version<3> get_new_output_api_version()
{
	static software_version<3> NEW_OUTPUT_API("10.0");

	return NEW_OUTPUT_API;
}

safe_ptr<core::frame_consumer> create_consumer(const core::parameters& params) 
{
	if(params.size() < 1 || params[0] != L"DECKLINK")
		return core::frame_consumer::empty();
	
	configuration config;
		
	if(params.size() > 1)
		config.device_index = lexical_cast_or_default<int>(params[1], config.device_index);
	
	if(std::find(params.begin(), params.end(), L"INTERNAL_KEY") != params.end())
		config.keyer = configuration::internal_keyer;
	else if(std::find(params.begin(), params.end(), L"EXTERNAL_KEY") != params.end())
		config.keyer = configuration::external_keyer;
	else if(std::find(params.begin(), params.end(), L"EXTERNAL_SEPARATE_DEVICE_KEY") != params.end())
		config.keyer = configuration::external_separate_device_keyer;
	else
		config.keyer = configuration::default_keyer;

	if(std::find(params.begin(), params.end(), L"LOW_LATENCY")	 != params.end())
		config.latency = configuration::low_latency;

	config.embedded_audio	= std::find(params.begin(), params.end(), L"EMBEDDED_AUDIO") != params.end();
	config.key_only			= std::find(params.begin(), params.end(), L"KEY_ONLY")		 != params.end();
	config.audio_layout		= core::default_channel_layout_repository().get_by_name(
			params.get(L"CHANNEL_LAYOUT", L"STEREO"));

	bool old_output_api = get_driver_version() < get_new_output_api_version();

	if (old_output_api)
		return make_safe<decklink_consumer_proxy<IDeckLinkOutput_v9_9>>(config);
	else
		return make_safe<decklink_consumer_proxy<IDeckLinkOutput>>(config);
}

safe_ptr<core::frame_consumer> create_consumer(const boost::property_tree::wptree& ptree) 
{
	configuration config;

	auto keyer = ptree.get(L"keyer", L"external");
	if(keyer == L"external")
		config.keyer = configuration::external_keyer;
	else if(keyer == L"internal")
		config.keyer = configuration::internal_keyer;
	else if(keyer == L"external_separate_device")
		config.keyer = configuration::external_separate_device_keyer;

	auto latency = ptree.get(L"latency", L"normal");
	if(latency == L"low")
		config.latency = configuration::low_latency;
	else if(latency == L"normal")
		config.latency = configuration::normal_latency;

	config.key_only				= ptree.get(L"key-only",			config.key_only);
	config.device_index			= ptree.get(L"device",				config.device_index);
	config.key_device_idx		= ptree.get(L"key-device",			config.key_device_idx);
	config.embedded_audio		= ptree.get(L"embedded-audio",		config.embedded_audio);
	config.base_buffer_depth	= ptree.get(L"buffer-depth",		config.base_buffer_depth);
	config.custom_allocator		= ptree.get(L"custom-allocator",	config.custom_allocator);
	config.audio_layout =
		core::default_channel_layout_repository().get_by_name(
				boost::to_upper_copy(ptree.get(L"channel-layout", L"STEREO")));

	bool old_output_api = get_driver_version() < get_new_output_api_version();

	if (old_output_api)
		return make_safe<decklink_consumer_proxy<IDeckLinkOutput_v9_9>>(config);
	else
		return make_safe<decklink_consumer_proxy<IDeckLinkOutput>>(config);
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