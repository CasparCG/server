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

#include <core/mixer/read_frame.h>

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

#include <agents.h>
#include <concrt_extras.h>

namespace caspar { namespace decklink { 
	
struct configuration
{
	size_t	device_index;
	bool	embedded_audio;
	bool	internal_key;
	bool	low_latency;
	bool	key_only;
	
	configuration()
		: device_index(1)
		, embedded_audio(false)
		, internal_key(false)
		, low_latency(false)
		, key_only(false){}

	size_t preroll_count() const
	{
		size_t count = 0;
		count += low_latency ? 2 : 3;
		count += embedded_audio ? 1 : 0;
		return count;
	}
};

class decklink_frame : public IDeckLinkVideoFrame
{
	tbb::atomic<int>											ref_count_;
	std::shared_ptr<core::read_frame>							frame_;
	const core::video_format_desc								format_desc_;

	bool														key_only_;
	std::vector<uint8_t, tbb::cache_aligned_allocator<uint8_t>> key_data_;
public:
	decklink_frame(const safe_ptr<core::read_frame>& frame, const core::video_format_desc& format_desc, bool key_only)
		: frame_(frame)
		, format_desc_(format_desc)
		, key_only_(key_only)
	{
		ref_count_ = 0;
	}
	
	STDMETHOD (QueryInterface(REFIID, LPVOID*))		{return E_NOINTERFACE;}
	STDMETHOD_(ULONG,			AddRef())			
	{
		return ++ref_count_;
	}
	STDMETHOD_(ULONG,			Release())			
	{
		--ref_count_;
		if(ref_count_ == 0)
			delete this;
		return ref_count_;
	}

	STDMETHOD_(long,			GetWidth())			{return format_desc_.width;}        
    STDMETHOD_(long,			GetHeight())		{return format_desc_.height;}        
    STDMETHOD_(long,			GetRowBytes())		{return format_desc_.width*4;}        
	STDMETHOD_(BMDPixelFormat,	GetPixelFormat())	{return bmdFormat8BitBGRA;}        
    STDMETHOD_(BMDFrameFlags,	GetFlags())			{return bmdFrameFlagDefault;}
        
    STDMETHOD(GetBytes(void** buffer))
	{
		static std::vector<uint8_t, tbb::cache_aligned_allocator<uint8_t>> zeros(1920*1080*4, 0);
		if(static_cast<size_t>(frame_->image_data().size()) != format_desc_.size)
		{
			*buffer = zeros.data();
			return S_OK;
		}

		if(!key_only_)
			*buffer = const_cast<uint8_t*>(frame_->image_data().begin());
		else
		{
			if(key_data_.empty())
			{
				key_data_.resize(frame_->image_data().size());
				fast_memshfl(key_data_.data(), frame_->image_data().begin(), frame_->image_data().size(), 0x0F0F0F0F, 0x0B0B0B0B, 0x07070707, 0x03030303);
				frame_.reset();
			}
			*buffer = key_data_.data();
		}

		return S_OK;
	}
        
    STDMETHOD(GetTimecode(BMDTimecodeFormat format, IDeckLinkTimecode** timecode)){return S_FALSE;}        
    STDMETHOD(GetAncillaryData(IDeckLinkVideoFrameAncillary** ancillary))		  {return S_FALSE;}
};

struct decklink_consumer : public IDeckLinkVideoOutputCallback, public IDeckLinkAudioOutputCallback, boost::noncopyable
{		
	const configuration					config_;

	CComPtr<IDeckLink>					decklink_;
	CComQIPtr<IDeckLinkOutput>			output_;
	CComQIPtr<IDeckLinkConfiguration>	configuration_;
	CComQIPtr<IDeckLinkKeyer>			keyer_;

	Concurrency::overwrite_buffer<std::exception_ptr>	exception_;

	tbb::atomic<bool>					is_running_;
		
	const std::wstring					model_name_;
	const core::video_format_desc		format_desc_;
	const size_t						buffer_size_;

	long long							frames_scheduled_;
	long long							audio_scheduled_;

	size_t								preroll_count_;
		
	boost::circular_buffer<std::vector<int32_t>>	audio_container_;

	tbb::concurrent_bounded_queue<std::shared_ptr<core::read_frame>> video_frame_buffer_;
	tbb::concurrent_bounded_queue<std::shared_ptr<core::read_frame>> audio_frame_buffer_;
	
	safe_ptr<diagnostics::graph> graph_;
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
		, buffer_size_(config.preroll_count())
		, frames_scheduled_(0)
		, audio_scheduled_(0)
		, preroll_count_(0)
		, audio_container_(buffer_size_+1)
	{
		is_running_ = true;
				
		video_frame_buffer_.set_capacity(1);
		audio_frame_buffer_.set_capacity(1);

		graph_->add_guide("tick-time", 0.5);
		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));	
		graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
		graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
		graph_->set_color("flushed-frame", diagnostics::color(0.4f, 0.3f, 0.8f));
		graph_->set_text(print());
		diagnostics::register_graph(graph_);
		
		enable_video(get_display_mode(output_, format_desc_.format, bmdFormat8BitBGRA, bmdVideoOutputFlagDefault));
				
		if(config.embedded_audio)
			enable_audio();

		set_latency(config.low_latency);				
		set_keyer(config.internal_key);
				
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
			
	const core::video_format_desc& get_video_format_desc() const
	{
		return format_desc_;
	}

	void set_latency(bool low_latency)
	{		
		if(!low_latency)
		{
			configuration_->SetFlag(bmdDeckLinkConfigLowLatencyVideoOutput, false);
			CASPAR_LOG(info) << print() << L" Enabled normal-latency mode";
		}
		else
		{			
			configuration_->SetFlag(bmdDeckLinkConfigLowLatencyVideoOutput, true);
			CASPAR_LOG(info) << print() << L" Enabled low-latency mode";
		}
	}

	void set_keyer(bool internal_key)
	{
		if(internal_key) 
		{
			if(FAILED(keyer_->Enable(FALSE)))			
				CASPAR_LOG(error) << print() << L" Failed to enable internal keyer.";			
			else if(FAILED(keyer_->SetLevel(255)))			
				CASPAR_LOG(error) << print() << L" Failed to set key-level to max.";
			else
				CASPAR_LOG(info) << print() << L" Enabled internal keyer.";		
		}
		else
		{
			if(FAILED(keyer_->Enable(TRUE)))			
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
				graph_->add_tag("late-frame");
				++frames_scheduled_;
				++audio_scheduled_;
			}
			else if(result == bmdOutputFrameDropped)
				graph_->add_tag("dropped-frame");
			else if(result == bmdOutputFrameFlushed)
				graph_->add_tag("flushed-frame");

			std::shared_ptr<core::read_frame> frame;	
			video_frame_buffer_.pop(frame);					
			schedule_next_video(make_safe_ptr(frame));	
		}
		catch(...)
		{
			Concurrency::send(exception_, std::current_exception());
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
					schedule_next_audio(make_safe<core::read_frame>());	
			}
			else
			{
				std::shared_ptr<core::read_frame> frame;
				audio_frame_buffer_.pop(frame);
				schedule_next_audio(make_safe_ptr(frame));	
			}
		}
		catch(...)
		{
			Concurrency::send(exception_, std::current_exception());
			return E_FAIL;
		}

		return S_OK;
	}

	void schedule_next_audio(const safe_ptr<core::read_frame>& frame)
	{
		const int sample_frame_count = frame->audio_data().size()/format_desc_.audio_channels;

		audio_container_.push_back(std::vector<int32_t>(frame->audio_data().begin(), frame->audio_data().end()));

		if(FAILED(output_->ScheduleAudioSamples(audio_container_.back().data(), sample_frame_count, (audio_scheduled_++) * sample_frame_count, format_desc_.audio_sample_rate, nullptr)))
			CASPAR_LOG(error) << print() << L" Failed to schedule audio.";
	}
			
	void schedule_next_video(const safe_ptr<core::read_frame>& frame)
	{
		CComPtr<IDeckLinkVideoFrame> frame2(new decklink_frame(frame, format_desc_, config_.key_only));
		if(FAILED(output_->ScheduleVideoFrame(frame2, (frames_scheduled_++) * format_desc_.duration, format_desc_.duration, format_desc_.time_scale)))
			CASPAR_LOG(error) << print() << L" Failed to schedule video.";

		graph_->update_value("tick-time", tick_timer_.elapsed()*format_desc_.fps*0.5);
		tick_timer_.restart();
	}

	void send(const safe_ptr<core::read_frame>& frame)
	{
		if(exception_.has_value())
			std::rethrow_exception(exception_.value());

		if(!is_running_)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Is not running."));
		
		Concurrency::scoped_oversubcription_token oversubscribe;		
		if(config_.embedded_audio)
			audio_frame_buffer_.push(frame);	
		video_frame_buffer_.push(frame);	
	}
	
	std::wstring print() const
	{
		return model_name_ + L" [" + boost::lexical_cast<std::wstring>(config_.device_index) + L"|" +  format_desc_.name + L"]";
	}
};

struct decklink_consumer_proxy : public core::frame_consumer
{
	const configuration					config_;
	std::unique_ptr<decklink_consumer>	context_;
	core::video_format_desc				format_desc_;
public:

	decklink_consumer_proxy(const configuration& config)
		: config_(config)
	{
	}

	~decklink_consumer_proxy()
	{
		auto str = print();
		context_.reset();
		CASPAR_LOG(info) << str << L" Successfully Uninitialized.";	
	}
	
	virtual void initialize(const core::video_format_desc& format_desc)
	{
		Concurrency::scoped_oversubcription_token oversubscribe;
		format_desc_ = format_desc;
		struct co_init
		{
			co_init(){CoInitialize(nullptr);}
			~co_init(){CoUninitialize();}
		} init;		
		context_ = nullptr;
		context_.reset(new decklink_consumer(config_, format_desc_));		
				
		CASPAR_LOG(info) << print() << L" Successfully Initialized.";	
	}
	
	virtual bool send(const safe_ptr<core::read_frame>& frame)
	{
		context_->send(frame);
		return true;
	}

	virtual size_t buffer_depth() const
	{
		return config_.preroll_count();
	}
	
	virtual std::wstring print() const
	{
		return context_ ? context_->print() : L"decklink_consumer";
	}
			
	virtual const core::video_format_desc& get_video_format_desc() const
	{
		return format_desc_;
	}
};	

safe_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>& params) 
{
	if(params.size() < 1 || params[0] != L"DECKLINK")
		return core::frame_consumer::empty();
	
	configuration config;
		
	if(params.size() > 1)
		config.device_index = lexical_cast_or_default<int>(params[1], config.device_index);
	
	config.internal_key		= std::find(params.begin(), params.end(), L"INTERNAL_KEY")	 != params.end();
	config.low_latency		= std::find(params.begin(), params.end(), L"LOW_LATENCY")	 != params.end();
	config.embedded_audio	= std::find(params.begin(), params.end(), L"EMBEDDED_AUDIO") != params.end();
	config.key_only			= std::find(params.begin(), params.end(), L"KEY_ONLY")		 != params.end();

	return make_safe<decklink_consumer_proxy>(config);
}

safe_ptr<core::frame_consumer> create_consumer(const boost::property_tree::ptree& ptree) 
{
	configuration config;

	config.internal_key		= ptree.get("internal-key",	  config.internal_key);
	config.low_latency		= ptree.get("low-latency",	  config.low_latency);
	config.key_only			= ptree.get("key-only",		  config.key_only);
	config.device_index		= ptree.get("device",		  config.device_index);
	config.embedded_audio	= ptree.get("embedded-audio", config.embedded_audio);

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