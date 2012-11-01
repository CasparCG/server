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

#include "../interop/DeckLinkAPI_h.h"

#include <core/mixer/read_frame.h>

#include <common/concurrency/com_context.h>
#include <common/concurrency/future_util.h>
#include <common/diagnostics/graph.h>
#include <common/exception/exceptions.h>
#include <common/memory/memcpy.h>
#include <common/memory/memclr.h>
#include <common/memory/memshfl.h>

#include <core/consumer/frame_consumer.h>

#include <tbb/concurrent_queue.h>
#include <tbb/cache_aligned_allocator.h>

#include <boost/circular_buffer.hpp>
#include <boost/timer.hpp>
#include <boost/property_tree/ptree.hpp>

namespace caspar { namespace decklink { 
	
struct configuration
{
	enum keyer_t
	{
		internal_keyer,
		external_keyer,
		default_keyer
	};

	enum latency_t
	{
		low_latency,
		normal_latency,
		default_latency
	};

	size_t		device_index;
	bool		embedded_audio;
	keyer_t		keyer;
	latency_t	latency;
	bool		key_only;
	size_t		base_buffer_depth;
	
	configuration()
		: device_index(1)
		, embedded_audio(false)
		, keyer(default_keyer)
		, latency(default_latency)
		, key_only(false)
		, base_buffer_depth(3)
	{
	}
	
	size_t buffer_depth() const
	{
		return base_buffer_depth + (latency == low_latency ? 0 : 1) + (embedded_audio ? 1 : 0);
	}
};

class decklink_frame : public IDeckLinkVideoFrame
{
	tbb::atomic<int>											ref_count_;
	std::shared_ptr<core::read_frame>							frame_;
	const core::video_format_desc								format_desc_;

	const bool													key_only_;
	std::vector<uint8_t, tbb::cache_aligned_allocator<uint8_t>> data_;
public:
	decklink_frame(const safe_ptr<core::read_frame>& frame, const core::video_format_desc& format_desc, bool key_only)
		: frame_(frame)
		, format_desc_(format_desc)
		, key_only_(key_only)
	{
		ref_count_ = 0;
	}
	
	// IUnknown

	STDMETHOD (QueryInterface(REFIID, LPVOID*))		
	{
		return E_NOINTERFACE;
	}
	
	STDMETHOD_(ULONG,			AddRef())			
	{
		return ++ref_count_;
	}

	STDMETHOD_(ULONG,			Release())			
	{
		if(--ref_count_ == 0)
			delete this;
		return ref_count_;
	}

	// IDecklinkVideoFrame

	STDMETHOD_(long,			GetWidth())			{return format_desc_.width;}        
    STDMETHOD_(long,			GetHeight())		{return format_desc_.height;}        
    STDMETHOD_(long,			GetRowBytes())		{return format_desc_.width*4;}        
	STDMETHOD_(BMDPixelFormat,	GetPixelFormat())	{return bmdFormat8BitBGRA;}        
    STDMETHOD_(BMDFrameFlags,	GetFlags())			{return bmdFrameFlagDefault;}
        
    STDMETHOD(GetBytes(void** buffer))
	{
		try
		{
			if(static_cast<size_t>(frame_->image_data().size()) != format_desc_.size)
			{
				data_.resize(format_desc_.size, 0);
				*buffer = data_.data();
			}
			else if(key_only_)
			{
				if(data_.empty())
				{
					data_.resize(frame_->image_data().size());
					fast_memshfl(data_.data(), frame_->image_data().begin(), frame_->image_data().size(), 0x0F0F0F0F, 0x0B0B0B0B, 0x07070707, 0x03030303);
				}
				*buffer = data_.data();
			}
			else
				*buffer = const_cast<uint8_t*>(frame_->image_data().begin());
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			return E_FAIL;
		}

		return S_OK;
	}
        
    STDMETHOD(GetTimecode(BMDTimecodeFormat format, IDeckLinkTimecode** timecode)){return S_FALSE;}        
    STDMETHOD(GetAncillaryData(IDeckLinkVideoFrameAncillary** ancillary))		  {return S_FALSE;}

	// decklink_frame	

	const boost::iterator_range<const int32_t*> audio_data()
	{
		return frame_->audio_data();
	}
};

struct decklink_consumer : public IDeckLinkVideoOutputCallback, public IDeckLinkAudioOutputCallback, boost::noncopyable
{		
	const int							channel_index_;
	const configuration					config_;

	CComPtr<IDeckLink>					decklink_;
	CComQIPtr<IDeckLinkOutput>			output_;
	CComQIPtr<IDeckLinkConfiguration>	configuration_;
	CComQIPtr<IDeckLinkKeyer>			keyer_;
	CComQIPtr<IDeckLinkAttributes>		attributes_;

	tbb::spin_mutex						exception_mutex_;
	std::exception_ptr					exception_;

	tbb::atomic<bool>					is_running_;
		
	const std::wstring					model_name_;
	const core::video_format_desc		format_desc_;
	const size_t						buffer_size_;

	long long							video_scheduled_;
	long long							audio_scheduled_;

	size_t								preroll_count_;
		
	boost::circular_buffer<std::vector<int32_t>>	audio_container_;

	tbb::concurrent_bounded_queue<std::shared_ptr<core::read_frame>> video_frame_buffer_;
	tbb::concurrent_bounded_queue<std::shared_ptr<core::read_frame>> audio_frame_buffer_;
	
	safe_ptr<diagnostics::graph> graph_;
	boost::timer tick_timer_;
	BMDReferenceStatus last_reference_status_;
	retry_task<bool> send_completion_;

public:
	decklink_consumer(const configuration& config, const core::video_format_desc& format_desc, int channel_index) 
		: channel_index_(channel_index)
		, config_(config)
		, decklink_(get_device(config.device_index))
		, output_(decklink_)
		, configuration_(decklink_)
		, keyer_(decklink_)
		, attributes_(decklink_)
		, model_name_(get_model_name(decklink_))
		, format_desc_(format_desc)
		, buffer_size_(config.buffer_depth()) // Minimum buffer-size 3.
		, video_scheduled_(0)
		, audio_scheduled_(0)
		, preroll_count_(0)
		, audio_container_(buffer_size_+1)
		, last_reference_status_(static_cast<BMDReferenceStatus>(-1))
	{
		is_running_ = true;
				
		video_frame_buffer_.set_capacity(1);
		audio_frame_buffer_.set_capacity(1);

		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));	
		graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
		graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
		graph_->set_color("flushed-frame", diagnostics::color(0.4f, 0.3f, 0.8f));
		graph_->set_color("buffered-audio", diagnostics::color(0.9f, 0.9f, 0.5f));
		graph_->set_color("buffered-video", diagnostics::color(0.2f, 0.9f, 0.9f));
		graph_->set_text(print());
		diagnostics::register_graph(graph_);
		
		enable_video(get_display_mode(output_, format_desc_.format, bmdFormat8BitBGRA, bmdVideoOutputFlagDefault));
				
		if(config.embedded_audio)
			enable_audio();

		set_latency(config.latency);				
		set_keyer(config.keyer);
				
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
			
	void set_latency(configuration::latency_t latency)
	{		
		if(latency == configuration::low_latency)
		{
			configuration_->SetFlag(bmdDeckLinkConfigLowLatencyVideoOutput, true);
			CASPAR_LOG(info) << print() << L" Enabled low-latency mode.";
		}
		else if(latency == configuration::normal_latency)
		{			
			configuration_->SetFlag(bmdDeckLinkConfigLowLatencyVideoOutput, false);
			CASPAR_LOG(info) << print() << L" Disabled low-latency mode.";
		}
	}

	void set_keyer(configuration::keyer_t keyer)
	{
		if(keyer == configuration::internal_keyer) 
		{
			BOOL value = true;
			if(SUCCEEDED(attributes_->GetFlag(BMDDeckLinkSupportsInternalKeying, &value)) && !value)
				CASPAR_LOG(error) << print() << L" Failed to enable internal keyer.";	
			else if(FAILED(keyer_->Enable(FALSE)))			
				CASPAR_LOG(error) << print() << L" Failed to enable internal keyer.";			
			else if(FAILED(keyer_->SetLevel(255)))			
				CASPAR_LOG(error) << print() << L" Failed to set key-level to max.";
			else
				CASPAR_LOG(info) << print() << L" Enabled internal keyer.";		
		}
		else if(keyer == configuration::external_keyer)
		{
			BOOL value = true;
			if(SUCCEEDED(attributes_->GetFlag(BMDDeckLinkSupportsExternalKeying, &value)) && !value)
				CASPAR_LOG(error) << print() << L" Failed to enable external keyer.";	
			else if(FAILED(keyer_->Enable(TRUE)))			
				CASPAR_LOG(error) << print() << L" Failed to enable external keyer.";	
			else if(FAILED(keyer_->SetLevel(255)))			
				CASPAR_LOG(error) << print() << L" Failed to set key-level to max.";
			else
				CASPAR_LOG(info) << print() << L" Enabled external keyer.";			
		}
	}
	
	void enable_audio()
	{
		if(FAILED(output_->EnableAudioOutput(bmdAudioSampleRate48kHz, bmdAudioSampleType32bitInteger, 2, bmdAudioOutputStreamTimestamped)))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not enable audio output."));
				
		if(FAILED(output_->SetAudioCallback(this)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not set audio callback."));

		CASPAR_LOG(info) << print() << L" Enabled embedded-audio.";
	}

	void enable_video(BMDDisplayMode display_mode)
	{
		if(FAILED(output_->EnableVideoOutput(display_mode, bmdVideoOutputFlagDefault))) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not enable video output."));
		
		if(FAILED(output_->SetScheduledFrameCompletionCallback(this)))
			BOOST_THROW_EXCEPTION(caspar_exception() 
									<< msg_info(narrow(print()) + " Failed to set playback completion callback.")
									<< boost::errinfo_api_function("SetScheduledFrameCompletionCallback"));
	}

	void start_playback()
	{
		if(FAILED(output_->StartScheduledPlayback(0, format_desc_.time_scale, 1.0))) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to schedule playback."));
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
		if(!is_running_)
			return E_FAIL;
		
		try
		{
			if(result == bmdOutputFrameDisplayedLate)
			{
				graph_->set_tag("late-frame");
				video_scheduled_ += format_desc_.duration;
				audio_scheduled_ += reinterpret_cast<decklink_frame*>(completed_frame)->audio_data().size()/format_desc_.audio_channels;
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
					schedule_next_audio(core::audio_buffer(format_desc_.audio_cadence[preroll % format_desc_.audio_cadence.size()], 0));	
			}
			else
			{
				std::shared_ptr<core::read_frame> frame;
				audio_frame_buffer_.pop(frame);
				send_completion_.try_completion();
				schedule_next_audio(frame->audio_data());
			}

			unsigned long buffered;
			output_->GetBufferedAudioSampleFrameCount(&buffered);
			graph_->set_value("buffered-audio", static_cast<double>(buffered)/(format_desc_.audio_cadence[0]*2));
		}
		catch(...)
		{
			tbb::spin_mutex::scoped_lock lock(exception_mutex_);
			exception_ = std::current_exception();
			return E_FAIL;
		}

		return S_OK;
	}

	template<typename T>
	void schedule_next_audio(const T& audio_data)
	{
		const int sample_frame_count = audio_data.size()/format_desc_.audio_channels;

		audio_container_.push_back(std::vector<int32_t>(audio_data.begin(), audio_data.end()));

		if(FAILED(output_->ScheduleAudioSamples(audio_container_.back().data(), sample_frame_count, audio_scheduled_, format_desc_.audio_sample_rate, nullptr)))
			CASPAR_LOG(error) << print() << L" Failed to schedule audio.";

		audio_scheduled_ += sample_frame_count;
	}
			
	void schedule_next_video(const safe_ptr<core::read_frame>& frame)
	{
		CComPtr<IDeckLinkVideoFrame> frame2(new decklink_frame(frame, format_desc_, config_.key_only));
		if(FAILED(output_->ScheduleVideoFrame(frame2, video_scheduled_, format_desc_.duration, format_desc_.time_scale)))
			CASPAR_LOG(error) << print() << L" Failed to schedule video.";

		video_scheduled_ += format_desc_.duration;

		graph_->set_value("tick-time", tick_timer_.elapsed()*format_desc_.fps*0.5);
		tick_timer_.restart();

		detect_reference_signal_change();
	}

	void detect_reference_signal_change()
	{
		BMDReferenceStatus reference_status;

		if (output_->GetReferenceStatus(&reference_status) != S_OK)
		{
			CASPAR_LOG(error) << print() << L" Reference signal: failed while querying status";
		}
		else if (reference_status != last_reference_status_)
		{
			last_reference_status_ = reference_status;

			if (reference_status == 0)
				CASPAR_LOG(info) << print() << L" Reference signal: not detected.";
			else if (reference_status & bmdReferenceNotSupportedByHardware)
				CASPAR_LOG(info) << print() << L" Reference signal: not supported by hardware.";
			else if (reference_status & bmdReferenceLocked)
				CASPAR_LOG(info) << print() << L" Reference signal: locked.";
			else
				CASPAR_LOG(info) << print() << L" Reference signal: Unhandled enum bitfield: " << reference_status;
		}
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
		return model_name_ + L" [" + boost::lexical_cast<std::wstring>(channel_index_) + L"-" +
			boost::lexical_cast<std::wstring>(config_.device_index) + L"|" +  format_desc_.name + L"]";
	}
};

struct decklink_consumer_proxy : public core::frame_consumer
{
	const configuration				config_;
	com_context<decklink_consumer>	context_;
	std::vector<size_t>				audio_cadence_;
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
	
	virtual void initialize(const core::video_format_desc& format_desc, int channel_index) override
	{
		context_.reset([&]{return new decklink_consumer(config_, format_desc, channel_index);});		
		audio_cadence_ = format_desc.audio_cadence;		

		CASPAR_LOG(info) << print() << L" Successfully Initialized.";	
	}
	
	virtual boost::unique_future<bool> send(const safe_ptr<core::read_frame>& frame) override
	{
		CASPAR_VERIFY(audio_cadence_.front() == static_cast<size_t>(frame->audio_data().size()));
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
		info.add(L"low-latency", config_.low_latency);
		info.add(L"embedded-audio", config_.embedded_audio);
		info.add(L"low-latency", config_.low_latency);
		//info.add(L"internal-key", config_.internal_key);
		return info;
	}

	virtual size_t buffer_depth() const override
	{
		return config_.buffer_depth();
	}

	virtual int index() const override
	{
		return 300 + config_.device_index;
	}
};	

safe_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>& params) 
{
	if(params.size() < 1 || params[0] != L"DECKLINK")
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

	if(std::find(params.begin(), params.end(), L"LOW_LATENCY")	 != params.end())
		config.latency = configuration::low_latency;

	config.embedded_audio	= std::find(params.begin(), params.end(), L"EMBEDDED_AUDIO") != params.end();
	config.key_only			= std::find(params.begin(), params.end(), L"KEY_ONLY")		 != params.end();

	return make_safe<decklink_consumer_proxy>(config);
}

safe_ptr<core::frame_consumer> create_consumer(const boost::property_tree::wptree& ptree) 
{
	configuration config;

	auto keyer = ptree.get(L"keyer", L"external");
	if(keyer == L"external")
		config.keyer = configuration::external_keyer;
	else if(keyer == L"internal")
		config.keyer = configuration::internal_keyer;

	auto latency = ptree.get(L"latency", L"normal");
	if(latency == L"low")
		config.latency = configuration::low_latency;
	else if(latency == L"normal")
		config.latency = configuration::normal_latency;

	config.key_only				= ptree.get(L"key-only",		config.key_only);
	config.device_index			= ptree.get(L"device",			config.device_index);
	config.embedded_audio		= ptree.get(L"embedded-audio",	config.embedded_audio);
	config.base_buffer_depth	= ptree.get(L"buffer-depth",	config.base_buffer_depth);

	return make_safe<decklink_consumer_proxy>(config);
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