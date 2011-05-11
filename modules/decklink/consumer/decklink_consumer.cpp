/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include "../StdAfx.h"
 
#include "decklink_consumer.h"

#include "../util/util.h"

#include "../interop/DeckLinkAPI_h.h"

#include <core/video_format.h>

#include <core/consumer/frame/read_frame.h>

#include <common/concurrency/com_context.h>
#include <common/diagnostics/graph.h>
#include <common/exception/exceptions.h>
#include <common/memory/memcpy.h>
#include <common/memory/memclr.h>

#include <tbb/concurrent_queue.h>

#include <boost/circular_buffer.hpp>
#include <boost/timer.hpp>

#include <vector>

#pragma warning(push)
#pragma warning(disable : 4996)

	#include <atlbase.h>

	#include <atlcom.h>
	#include <atlhost.h>

#pragma warning(push)

namespace caspar { 
	
enum key
{
	external_key,
	internal_key,
	default_key
};

enum latency
{
	low_latency,
	normal_latency,
	default_latency
};

struct configuration
{
	size_t device_index;
	bool embedded_audio;
	key keyer;
	latency latency;
	
	configuration()
		: device_index(1)
		, embedded_audio(false)
		, keyer(default_key)
		, latency(default_latency){}
};

class decklink_frame_adapter : public IDeckLinkVideoFrame
{
	safe_ptr<const core::read_frame> frame_;
	core::video_format_desc format_desc_;
public:
	decklink_frame_adapter(const safe_ptr<const core::read_frame>& frame, const core::video_format_desc& format_desc)
		: frame_(frame)
		, format_desc_(format_desc){}
	
	STDMETHOD (QueryInterface(REFIID, LPVOID*))	{return E_NOINTERFACE;}
	STDMETHOD_(ULONG, AddRef())					{return 1;}
	STDMETHOD_(ULONG, Release())				{return 1;}

	STDMETHOD_(long, GetWidth())				{return format_desc_.width;}        
    STDMETHOD_(long, GetHeight())				{return format_desc_.height;}        
    STDMETHOD_(long, GetRowBytes())				{return format_desc_.width*4;}        
	STDMETHOD_(BMDPixelFormat, GetPixelFormat()){return bmdFormat8BitBGRA;}        
    STDMETHOD_(BMDFrameFlags, GetFlags())		{return bmdFrameFlagDefault;}
        
    STDMETHOD(GetBytes(void** buffer))
	{
		static std::vector<unsigned char> zeros(1920*1080*4, 0);
		*buffer = const_cast<unsigned char*>(frame_->image_data().begin());
		if(static_cast<size_t>(frame_->image_data().size()) != format_desc_.size)
			*buffer = zeros.data();
		return S_OK;
	}
        
    STDMETHOD(GetTimecode(BMDTimecodeFormat format, IDeckLinkTimecode** timecode)){return S_FALSE;}        
    STDMETHOD(GetAncillaryData(IDeckLinkVideoFrameAncillary** ancillary))		  {return S_FALSE;}
};

struct decklink_consumer : public IDeckLinkVideoOutputCallback, public IDeckLinkAudioOutputCallback, boost::noncopyable
{		
	const configuration config_;

	CComPtr<IDeckLink>					decklink_;
	CComQIPtr<IDeckLinkOutput>			output_;
	CComQIPtr<IDeckLinkConfiguration>	configuration_;
	CComQIPtr<IDeckLinkKeyer>			keyer_;

	std::exception_ptr exception_;

	tbb::atomic<bool> is_running_;
		
	const std::wstring model_name_;
	const core::video_format_desc format_desc_;
	const size_t buffer_size_;

	unsigned long frames_scheduled_;
	unsigned long audio_scheduled_;
		
	std::list<decklink_frame_adapter> frame_container_; // Must be std::list in order to guarantee that pointers are always valid.
	boost::circular_buffer<std::vector<short>> audio_container_;

	tbb::concurrent_bounded_queue<std::shared_ptr<const core::read_frame>> video_frame_buffer_;
	tbb::concurrent_bounded_queue<std::shared_ptr<const core::read_frame>> audio_frame_buffer_;
	
	std::shared_ptr<diagnostics::graph> graph_;
	boost::timer tick_timer_;


public:
	decklink_consumer(const configuration& config, const core::video_format_desc& format_desc) 
		: config_(config)
		, decklink_(get_device(config.device_index))
		, output_(decklink_)
		, configuration_(decklink_)
		, keyer_(decklink_)
		, model_name_(get_model_name(decklink_))
		, format_desc_(format_desc)
		, buffer_size_(config.embedded_audio ? 5 : 4) // Minimum buffer-size (3 + 1 tolerance).
		, frames_scheduled_(0)
		, audio_scheduled_(0)
		, audio_container_(buffer_size_+1)
	{
		is_running_ = true;
				
		video_frame_buffer_.set_capacity(1);
		audio_frame_buffer_.set_capacity(1);

		graph_ = diagnostics::create_graph(narrow(print()));
		graph_->add_guide("tick-time", 0.5);
		graph_->set_color("tick-time", diagnostics::color(0.1f, 0.7f, 0.8f));
		graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
		graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
		graph_->set_color("flushed-frame", diagnostics::color(0.3f, 0.3f, 0.6f));
		
		enable_video(get_display_mode(output_, format_desc_.format, bmdFormat8BitBGRA, bmdVideoOutputFlagDefault));
				
		if(config.embedded_audio)
			enable_audio();

		set_latency(config.latency);				
		set_keyer(config.keyer);
								
		for(size_t n = 0; n < buffer_size_; ++n)
			schedule_next_video(core::read_frame::empty());
		
		if(config.embedded_audio)
			output_->BeginAudioPreroll();
		else
		{
			if(FAILED(output_->StartScheduledPlayback(0, format_desc_.time_scale, 1.0))) 
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to schedule playback."));
		}
		
		CASPAR_LOG(info) << print() << L" Buffer depth: " << buffer_size_;		
		CASPAR_LOG(info) << print() << L" Successfully initialized for " << format_desc_.name;	
	}

	~decklink_consumer()
	{		
		is_running_ = false;
		video_frame_buffer_.clear();
		audio_frame_buffer_.clear();
		video_frame_buffer_.try_push(core::read_frame::empty());
		audio_frame_buffer_.try_push(core::read_frame::empty());

		if(output_ != nullptr) 
		{
			output_->StopScheduledPlayback(0, nullptr, 0);
			if(config_.embedded_audio)
				output_->DisableAudioOutput();
			output_->DisableVideoOutput();
		}
	}
			
	void set_latency(latency latency)
	{		
		if(latency == normal_latency)
		{
			configuration_->SetFlag(bmdDeckLinkConfigLowLatencyVideoOutput, false);
			CASPAR_LOG(info) << print() << L" Enabled normal-latency mode";
		}
		else if(latency == low_latency)
		{			
			configuration_->SetFlag(bmdDeckLinkConfigLowLatencyVideoOutput, true);
			CASPAR_LOG(info) << print() << L" Enabled low-latency mode";
		}
		else
			CASPAR_LOG(info) << print() << L" Uses driver latency settings.";	
	}

	void set_keyer(key keyer)
	{
		if(keyer == internal_key) 
		{
			if(FAILED(keyer_->Enable(FALSE)))			
				CASPAR_LOG(error) << print() << L" Failed to enable internal keyer.";			
			else if(FAILED(keyer_->SetLevel(255)))			
				CASPAR_LOG(error) << print() << L" Failed to set key-level to max.";
			else
				CASPAR_LOG(info) << print() << L" Enabled internal keyer.";		
		}
		else if(keyer == external_key)
		{
			if(FAILED(keyer_->Enable(TRUE)))			
				CASPAR_LOG(error) << print() << L" Failed to enable external keyer.";	
			else if(FAILED(keyer_->SetLevel(255)))			
				CASPAR_LOG(error) << print() << L" Failed to set key-level to max.";
			else
				CASPAR_LOG(info) << print() << L" Enabled external keyer.";			
		}
		else
			CASPAR_LOG(info) << print() << L" Uses driver keyer settings.";	
	}
	
	void enable_audio()
	{
		if(FAILED(output_->EnableAudioOutput(bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger, 2, bmdAudioOutputStreamTimestamped)))
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
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to set playback completion callback."));
	}
	
	STDMETHOD (QueryInterface(REFIID, LPVOID*))	{return E_NOINTERFACE;}
	STDMETHOD_(ULONG, AddRef())					{return 1;}
	STDMETHOD_(ULONG, Release())				{return 1;}
	
	STDMETHOD(ScheduledFrameCompleted(IDeckLinkVideoFrame* completed_frame, BMDOutputFrameCompletionResult result))
	{
		if(!is_running_)
			return E_FAIL;
		
		try
		{
			if(result == bmdOutputFrameDisplayedLate)
				graph_->add_tag("late-frame");
			else if(result == bmdOutputFrameDropped)
				graph_->add_tag("dropped-frame");
			else if(result == bmdOutputFrameFlushed)
				graph_->add_tag("flushed-frame");

			frame_container_.erase(std::remove_if(frame_container_.begin(), frame_container_.end(), [&](const decklink_frame_adapter& frame)
			{
				return &frame == completed_frame;
			}), frame_container_.end());

			std::shared_ptr<const core::read_frame> frame;	
			video_frame_buffer_.pop(frame);		
			schedule_next_video(safe_ptr<const core::read_frame>(frame));			
		}
		catch(...)
		{
			exception_ = std::current_exception();
			return E_FAIL;
		}

		return S_OK;
	}

	STDMETHOD(ScheduledPlaybackHasStopped())
	{
		is_running_ = false;
		CASPAR_LOG(info) << print() << L" Scheduled playback has stopped.";
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
				if(FAILED(output_->StartScheduledPlayback(0, format_desc_.time_scale, 1.0)))
					BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to schedule playback."));
			}

			std::shared_ptr<const core::read_frame> frame;
			audio_frame_buffer_.pop(frame);
			schedule_next_audio(safe_ptr<const core::read_frame>(frame));		
		}
		catch(...)
		{
			exception_ = std::current_exception();
			return E_FAIL;
		}

		return S_OK;
	}

	void schedule_next_audio(const safe_ptr<const core::read_frame>& frame)
	{
		static std::vector<short> silence(48000, 0);

		int audio_samples = static_cast<size_t>(48000.0 / format_desc_.fps)*2; // Audio samples per channel

		const short* frame_audio_data = frame->audio_data().size() == audio_samples ? frame->audio_data().begin() : silence.data();
		audio_container_.push_back(std::vector<short>(frame_audio_data, frame_audio_data+audio_samples));

		if(FAILED(output_->ScheduleAudioSamples(audio_container_.back().data(), audio_samples/2, (audio_scheduled_++) * audio_samples/2, 48000, nullptr)))
			CASPAR_LOG(error) << print() << L" Failed to schedule audio.";
	}
			
	void schedule_next_video(const safe_ptr<const core::read_frame>& frame)
	{
		frame_container_.push_back(decklink_frame_adapter(frame, format_desc_));
		if(FAILED(output_->ScheduleVideoFrame(&frame_container_.back(), (frames_scheduled_++) * format_desc_.duration, format_desc_.duration, format_desc_.time_scale)))
			CASPAR_LOG(error) << print() << L" Failed to schedule video.";

		graph_->update_value("tick-time", tick_timer_.elapsed()*format_desc_.fps*0.5);
		tick_timer_.restart();
	}

	void send(const safe_ptr<const core::read_frame>& frame)
	{
		if(exception_ != nullptr)
			std::rethrow_exception(exception_);

		if(!is_running_)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Is not running."));
		
		if(config_.embedded_audio)
			audio_frame_buffer_.push(frame);	
		video_frame_buffer_.push(frame);	
	}
	
	std::wstring print() const
	{
		return model_name_ + L" [" + boost::lexical_cast<std::wstring>(config_.device_index) + L"]";
	}
};

struct decklink_consumer_proxy : public core::frame_consumer
{
	const configuration config_;

	com_context<decklink_consumer> context_;
public:

	decklink_consumer_proxy(const configuration& config)
		: config_(config)
		, context_(L"decklink_consumer[" + boost::lexical_cast<std::wstring>(config.device_index) + L"]"){}
	
	virtual void initialize(const core::video_format_desc& format_desc)
	{
		context_.reset([&]{return new decklink_consumer(config_, format_desc);});
	}
	
	virtual void send(const safe_ptr<const core::read_frame>& frame)
	{
		context_->send(frame);
	}
	
	virtual std::wstring print() const
	{
		return context_->print();
	}
};	

safe_ptr<core::frame_consumer> create_decklink_consumer(const std::vector<std::wstring>& params) 
{
	if(params.size() < 1 || params[0] != L"DECKLINK")
		return core::frame_consumer::empty();
	
	configuration config;
		
	if(params.size() > 1)
		config.device_index = lexical_cast_or_default<int>(params[1], config.device_index);
		
	if(std::find(params.begin(), params.end(), L"INTERNAL_KEY") != params.end())
		config.keyer = internal_key;
	else if(std::find(params.begin(), params.end(), L"EXTERNAL_KEY") != params.end())
		config.keyer = external_key;
	
	if(std::find(params.begin(), params.end(), L"LOW_LATENCY") != params.end())
		config.latency = low_latency;
	else if(std::find(params.begin(), params.end(), L"NORMAL_LATENCY") != params.end())
		config.latency = normal_latency;
		
	config.embedded_audio = std::find(params.begin(), params.end(), L"EMBEDDED_AUDIO") != params.end();

	return make_safe<decklink_consumer_proxy>(config);
}

safe_ptr<core::frame_consumer> create_decklink_consumer(const boost::property_tree::ptree& ptree) 
{
	configuration config;

	auto key_str = ptree.get("key", "default");
	if(key_str == "internal")
		config.keyer = internal_key;
	else if(key_str == "external")
		config.keyer = external_key;

	auto latency_str = ptree.get("latency", "default");
	if(latency_str == "normal")
		config.latency = normal_latency;
	else if(latency_str == "low")
		config.latency = low_latency;

	config.device_index = ptree.get("device", 0);
	config.embedded_audio  = ptree.get("embedded-audio", false);

	return make_safe<decklink_consumer_proxy>(config);
}

}

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