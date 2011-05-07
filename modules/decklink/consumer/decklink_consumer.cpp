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

#include <common/concurrency/executor.h>
#include <common/diagnostics/graph.h>
#include <common/exception/exceptions.h>
#include <common/memory/memcpy.h>
#include <common/memory/memclr.h>
#include <common/utility/timer.h>

#include <tbb/concurrent_queue.h>

#include <boost/circular_buffer.hpp>
#include <boost/timer.hpp>

#include <array>

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

struct decklink_output : public IDeckLinkVideoOutputCallback, public IDeckLinkAudioOutputCallback, boost::noncopyable
{		
	static const size_t BUFFER_SIZE = 4;
	
	struct co_init
	{
		co_init(){CoInitialize(nullptr);}
		~co_init(){CoUninitialize();}
	} co_;
	
	std::exception_ptr exception_;
	const configuration config_;

	std::wstring model_name_;
	tbb::atomic<bool> is_running_;

	std::shared_ptr<diagnostics::graph> graph_;
	boost::timer perf_timer_;

	std::array<std::pair<void*, CComPtr<IDeckLinkMutableVideoFrame>>, BUFFER_SIZE+1> reserved_frames_;
	boost::circular_buffer<std::vector<short>> audio_container_;
	
	CComPtr<IDeckLink>					decklink_;
	CComQIPtr<IDeckLinkOutput>			output_;
	CComQIPtr<IDeckLinkConfiguration>	configuration_;
	CComQIPtr<IDeckLinkKeyer>			keyer_;

	const core::video_format_desc format_desc_;

	BMDTimeScale frame_time_scale_;
	BMDTimeValue frame_duration_;
	unsigned long frames_scheduled_;
	unsigned long audio_scheduled_;
	
	tbb::concurrent_bounded_queue<std::shared_ptr<const core::read_frame>> video_frame_buffer_;
	tbb::concurrent_bounded_queue<std::shared_ptr<const core::read_frame>> audio_frame_buffer_;

public:
	decklink_output(const configuration& config, const core::video_format_desc& format_desc) 
		: model_name_(L"DECKLINK")
		, config_(config)
		, audio_container_(5)
		, frames_scheduled_(0)
		, audio_scheduled_(0)
		, format_desc_(format_desc)
	{
		is_running_ = true;
		CComPtr<IDeckLinkIterator> pDecklinkIterator;
		if(FAILED(pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " No Decklink drivers installed."));
		
		size_t n = 0;
		while(n < config_.device_index && pDecklinkIterator->Next(&decklink_) == S_OK){++n;}	

		if(n != config_.device_index || !decklink_)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Decklink card not found.") << arg_name_info("device_index") << arg_value_info(boost::lexical_cast<std::string>(config_.device_index)));
		
		BSTR pModelName;
		decklink_->GetModelName(&pModelName);
		model_name_ = std::wstring(pModelName);
				
		graph_ = diagnostics::create_graph(narrow(print()));
		graph_->add_guide("tick-time", 0.5);
		graph_->set_color("tick-time", diagnostics::color(0.1f, 0.7f, 0.8f));
		
		output_ = decklink_;
		configuration_ = decklink_;
		keyer_ = decklink_;

		auto display_mode = get_display_mode(output_.p, format_desc_.format);
		if(display_mode == nullptr) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Card does not support requested videoformat."));
		
		display_mode->GetFrameRate(&frame_duration_, &frame_time_scale_);

		BMDDisplayModeSupport displayModeSupport;
		if(FAILED(output_->DoesSupportVideoMode(display_mode->GetDisplayMode(), bmdFormat8BitBGRA, bmdVideoOutputFlagDefault, &displayModeSupport, nullptr)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Card does not support requested videoformat."));
		
		if(config_.embedded_audio)
		{
			if(FAILED(output_->EnableAudioOutput(bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger, 2, bmdAudioOutputStreamTimestamped)))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not enable audio output."));
				
			if(FAILED(output_->SetAudioCallback(this)))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not set audio callback."));

			CASPAR_LOG(info) << print() << L" Enabled embedded-audio.";
		}

		if(config_.latency == normal_latency)
		{
			configuration_->SetFlag(bmdDeckLinkConfigLowLatencyVideoOutput, false);
			CASPAR_LOG(info) << print() << L" Enabled normal-latency mode";
		}
		else if(config_.latency == low_latency)
		{			
			configuration_->SetFlag(bmdDeckLinkConfigLowLatencyVideoOutput, true);
			CASPAR_LOG(info) << print() << L" Enabled low-latency mode";
		}
		else
			CASPAR_LOG(info) << print() << L" Uses driver latency settings.";	
		
		if(config_.keyer == internal_key) 
		{
			if(FAILED(keyer_->Enable(FALSE)))			
				CASPAR_LOG(error) << print() << L" Failed to enable internal keyer.";			
			else if(FAILED(keyer_->SetLevel(255)))			
				CASPAR_LOG(error) << print() << L" Failed to set key-level to max.";
			else
				CASPAR_LOG(info) << print() << L" Enabled internal keyer.";		
		}
		else if(config.keyer == external_key)
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

		if(FAILED(output_->EnableVideoOutput(display_mode->GetDisplayMode(), bmdVideoOutputFlagDefault))) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not enable video output."));
		
		if(FAILED(output_->SetScheduledFrameCompletionCallback(this)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to set playback completion callback."));
					
		for(size_t n = 0; n < reserved_frames_.size(); ++n)
		{
			if(FAILED(output_->CreateVideoFrame(format_desc_.width, format_desc_.height, format_desc_.size/format_desc_.height, bmdFormat8BitBGRA, bmdFrameFlagDefault, &reserved_frames_[n].second)))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to create frame."));

			if(FAILED(reserved_frames_[n].second->GetBytes(&reserved_frames_[n].first)))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to get frame bytes."));
		}

		CASPAR_LOG(info) << print() << L" Buffer-depth: " << BUFFER_SIZE;
		
		for(size_t n = 0; n < BUFFER_SIZE; ++n)
			schedule_next_video(core::read_frame::empty());

		video_frame_buffer_.set_capacity(2);
		audio_frame_buffer_.set_capacity(2);
		
		if(config_.embedded_audio)
			output_->BeginAudioPreroll();
		else
		{
			if(FAILED(output_->StartScheduledPlayback(0, frame_time_scale_, 1.0))) 
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to schedule playback."));
		}
		
		CASPAR_LOG(info) << print() << L" Successfully initialized for " << format_desc_.name;	
	}

	~decklink_output()
	{		
		is_running_ = false;
		video_frame_buffer_.try_push(core::read_frame::empty());
		audio_frame_buffer_.try_push(core::read_frame::empty());

		if(output_ != nullptr) 
		{
			output_->StopScheduledPlayback(0, nullptr, 0);
			if(config_.embedded_audio)
				output_->DisableAudioOutput();
			output_->DisableVideoOutput();
		}
		CASPAR_LOG(info) << print() << L" Shutting down.";	
	}
			
	virtual HRESULT STDMETHODCALLTYPE	QueryInterface (REFIID, LPVOID*)	{return E_NOINTERFACE;}
	virtual ULONG STDMETHODCALLTYPE		AddRef ()							{return 1;}
	virtual ULONG STDMETHODCALLTYPE		Release ()							{return 1;}
	
	virtual HRESULT STDMETHODCALLTYPE ScheduledFrameCompleted (IDeckLinkVideoFrame* /*completedFrame*/, BMDOutputFrameCompletionResult /*result*/)
	{
		if(!is_running_)
			return S_OK;

		std::shared_ptr<const core::read_frame> frame;	
		video_frame_buffer_.pop(frame);		
		schedule_next_video(safe_ptr<const core::read_frame>(frame));

		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ScheduledPlaybackHasStopped (void)
	{
		is_running_ = false;
		CASPAR_LOG(info) << print() << L"Scheduled playback has stopped.";
		return S_OK;
	}
		
	virtual HRESULT STDMETHODCALLTYPE RenderAudioSamples (BOOL preroll)
	{
		if(!is_running_)
			return S_OK;
		
		try
		{
			if(preroll)
			{
				if(FAILED(output_->StartScheduledPlayback(0, frame_time_scale_, 1.0)))
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

		int audio_samples = static_cast<size_t>(48000.0 / format_desc_.fps);

		auto frame_audio_data = frame->audio_data().empty() ? silence.data() : const_cast<short*>(frame->audio_data().begin());

		audio_container_.push_back(std::vector<short>(frame_audio_data, frame_audio_data+audio_samples*2));

		if(FAILED(output_->ScheduleAudioSamples(audio_container_.back().data(), audio_samples, (audio_scheduled_++) * audio_samples, 48000, nullptr)))
			CASPAR_LOG(error) << print() << L" Failed to schedule audio.";
	}
			
	void schedule_next_video(const safe_ptr<const core::read_frame>& frame)
	{
		if(static_cast<size_t>(frame->image_data().size()) == format_desc_.size)
			fast_memcpy(reserved_frames_.front().first, frame->image_data().begin(), frame->image_data().size());
		else
			fast_memclr(reserved_frames_.front().first, format_desc_.size);

		if(FAILED(output_->ScheduleVideoFrame(reserved_frames_.front().second, (frames_scheduled_++) * frame_duration_, frame_duration_, frame_time_scale_)))
			CASPAR_LOG(error) << print() << L" Failed to schedule video.";

		std::rotate(reserved_frames_.begin(), reserved_frames_.begin() + 1, reserved_frames_.end());
		graph_->update_value("tick-time", static_cast<float>(perf_timer_.elapsed()/format_desc_.interval)*0.5f);
		perf_timer_.restart();
	}

	void send(const safe_ptr<const core::read_frame>& frame)
	{
		if(!is_running_)
			return;

		if(exception_ != nullptr)
			std::rethrow_exception(exception_);

		video_frame_buffer_.push(frame);
		if(config_.embedded_audio)
			audio_frame_buffer_.push(frame);
	}

	std::wstring print() const
	{
		return model_name_ + L" [" + boost::lexical_cast<std::wstring>(config_.device_index) + L"]";
	}
};

struct decklink_consumer : public core::frame_consumer
{
	std::unique_ptr<decklink_output> input_;
	configuration config_;

	executor executor_;
public:

	decklink_consumer(const configuration& config)
		: config_(config)
		, executor_(L"DECKLINK[" + boost::lexical_cast<std::wstring>(config.device_index) + L"]", true){}

	~decklink_consumer()
	{
		executor_.invoke([&]
		{
			input_ = nullptr;
		});
	}

	void initialize(const core::video_format_desc& format_desc)
	{
		executor_.invoke([&]
		{
			input_.reset(new decklink_output(config_, format_desc));
		});
	}
	
	void send(const safe_ptr<const core::read_frame>& frame)
	{
		input_->send(frame);
	}
	
	std::wstring print() const
	{
		return input_->print();
	}
};	

safe_ptr<core::frame_consumer> create_decklink_consumer(const std::vector<std::wstring>& params) 
{
	if(params.size() < 1 || params[0] != L"DECKLINK")
		return core::frame_consumer::empty();
	
	configuration config;
		
	if(params.size() > 1)
		config.device_index = lexical_cast_or_default<int>(params[1], config.device_index);
		
	auto it = std::find(params.begin(), params.end(), L"INTERNAL_KEY");
	if(it != params.end())
		config.keyer = internal_key;
	else
	{
		auto it = std::find(params.begin(), params.end(), L"EXTERNAL_KEY");
		if(it != params.end())
			config.keyer = external_key;
	}
		
	config.embedded_audio = std::find(params.begin(), params.end(), L"EMBEDDED_AUDIO") != params.end();

	return make_safe<decklink_consumer>(config);
}

safe_ptr<core::frame_consumer> create_decklink_consumer_ptree(const boost::property_tree::ptree& ptree) 
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

	return make_safe<decklink_consumer>(config);
}

}