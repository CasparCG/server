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

#include "../stdafx.h"

#include "decklink_producer.h"

#include "../interop/DeckLinkAPI_h.h"
#include "../util/util.h"

#include "../../ffmpeg/producer/filter/filter.h"
#include "../../ffmpeg/producer/util/util.h"
#include "../../ffmpeg/producer/muxer/frame_muxer.h"
#include "../../ffmpeg/producer/muxer/display_mode.h"

#include <common/executor.h>
#include <common/diagnostics/graph.h>
#include <common/except.h>
#include <common/log.h>
#include <common/param.h>

#include <core/frame/frame.h>
#include <core/frame/draw_frame.h>
#include <core/frame/frame_transform.h>
#include <core/frame/frame_factory.h>
#include <core/monitor/monitor.h>
#include <core/mixer/audio/audio_mixer.h>

#include <tbb/concurrent_queue.h>

#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/timer.hpp>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavcodec/avcodec.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

#pragma warning(push)
#pragma warning(disable : 4996)

	#include <atlbase.h>

	#include <atlcom.h>
	#include <atlhost.h>

#pragma warning(push)

#include <functional>

namespace caspar { namespace decklink {
		
class decklink_producer : boost::noncopyable, public IDeckLinkInputCallback
{	
	monitor::basic_subject							event_subject_;
	spl::shared_ptr<diagnostics::graph>				graph_;
	boost::timer									tick_timer_;

	CComPtr<IDeckLink>								decklink_;
	CComQIPtr<IDeckLinkInput>						input_;
	CComQIPtr<IDeckLinkAttributes >					attributes_;
	
	const std::wstring								model_name_;
	const int										device_index_;
	const std::wstring								filter_;
	
	std::vector<int>								audio_cadence_;
	boost::circular_buffer<size_t>					sync_buffer_;
	ffmpeg::frame_muxer								muxer_;
			
	spl::shared_ptr<core::frame_factory>			frame_factory_;
	core::video_format_desc							in_format_desc_;
	core::video_format_desc							out_format_desc_;

	tbb::concurrent_bounded_queue<core::draw_frame>	frame_buffer_;

	std::exception_ptr								exception_;		

public:
	decklink_producer(const core::video_format_desc& in_format_desc, 
					  int device_index, 
					  const spl::shared_ptr<core::frame_factory>& frame_factory, 
					  const core::video_format_desc& out_format_desc, 
					  const std::wstring& filter)
		: decklink_(get_device(device_index))
		, input_(decklink_)
		, attributes_(decklink_)
		, model_name_(get_model_name(decklink_))
		, device_index_(device_index)
		, filter_(filter)
		, in_format_desc_(in_format_desc)
		, out_format_desc_(out_format_desc)
		, muxer_(in_format_desc.fps, frame_factory, out_format_desc, filter)
		, audio_cadence_(out_format_desc.audio_cadence)
		, sync_buffer_(out_format_desc.audio_cadence.size())
		, frame_factory_(frame_factory)
	{	
		frame_buffer_.set_capacity(2);
		
		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));	
		graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
		graph_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));
		graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
		graph_->set_color("output-buffer", diagnostics::color(0.0f, 1.0f, 0.0f));
		graph_->set_text(print());
		diagnostics::register_graph(graph_);
		
		auto display_mode = get_display_mode(input_, in_format_desc.format, bmdFormat8BitYUV, bmdVideoInputFlagDefault);
		
		// NOTE: bmdFormat8BitARGB is currently not supported by any decklink card. (2011-05-08)
		if(FAILED(input_->EnableVideoInput(display_mode, bmdFormat8BitYUV, 0))) 
			CASPAR_THROW_EXCEPTION(caspar_exception() 
									<< msg_info(print() + L" Could not enable video input.")
									<< boost::errinfo_api_function("EnableVideoInput"));

		if(FAILED(input_->EnableAudioInput(bmdAudioSampleRate48kHz, bmdAudioSampleType32bitInteger, static_cast<int>(in_format_desc.audio_channels)))) 
			CASPAR_THROW_EXCEPTION(caspar_exception() 
									<< msg_info(print() + L" Could not enable audio input.")
									<< boost::errinfo_api_function("EnableAudioInput"));
			
		if (FAILED(input_->SetCallback(this)) != S_OK)
			CASPAR_THROW_EXCEPTION(caspar_exception() 
									<< msg_info(print() + L" Failed to set input callback.")
									<< boost::errinfo_api_function("SetCallback"));
			
		if(FAILED(input_->StartStreams()))
			CASPAR_THROW_EXCEPTION(caspar_exception() 
									<< msg_info(print() + L" Failed to start input stream.")
									<< boost::errinfo_api_function("StartStreams"));

		CASPAR_LOG(info) << print() << L" Initialized";
	}

	~decklink_producer()
	{
		if(input_ != nullptr) 
		{
			input_->StopStreams();
			input_->DisableVideoInput();
		}
	}

	virtual HRESULT STDMETHODCALLTYPE	QueryInterface (REFIID, LPVOID*)	{return E_NOINTERFACE;}
	virtual ULONG STDMETHODCALLTYPE		AddRef ()							{return 1;}
	virtual ULONG STDMETHODCALLTYPE		Release ()							{return 1;}
		
	virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents /*notificationEvents*/, IDeckLinkDisplayMode* newDisplayMode, BMDDetectedVideoInputFormatFlags /*detectedSignalFlags*/)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame* video, IDeckLinkAudioInputPacket* audio)
	{	
		if(!video)
			return S_OK;

		try
		{
			graph_->set_value("tick-time", tick_timer_.elapsed()*out_format_desc_.fps*0.5);
			tick_timer_.restart();

			boost::timer frame_timer;	
			
			// Video

			void* video_bytes = nullptr;
			if(FAILED(video->GetBytes(&video_bytes)) || !video_bytes)
				return S_OK;
			
			auto video_frame = ffmpeg::create_frame();
						
			video_frame->data[0]			= reinterpret_cast<uint8_t*>(video_bytes);
			video_frame->linesize[0]		= video->GetRowBytes();			
			video_frame->format				= PIX_FMT_UYVY422;
			video_frame->width				= video->GetWidth();
			video_frame->height				= video->GetHeight();
			video_frame->interlaced_frame	= in_format_desc_.field_mode != core::field_mode::progressive;
			video_frame->top_field_first	= in_format_desc_.field_mode == core::field_mode::upper ? 1 : 0;
				
			event_subject_	<< monitor::event("file/name")				% model_name_
							<< monitor::event("file/path")				% device_index_
							<< monitor::event("file/video/width")		% video->GetWidth()
							<< monitor::event("file/video/height")		% video->GetHeight()
							<< monitor::event("file/video/field")		% u8(!video_frame->interlaced_frame ? "progressive" : (video_frame->top_field_first ? "upper" : "lower"))
							<< monitor::event("file/audio/sample-rate")	% 48000
							<< monitor::event("file/audio/channels")	% 2
							<< monitor::event("file/audio/format")		% u8(av_get_sample_fmt_name(AV_SAMPLE_FMT_S32))
							<< monitor::event("file/fps")				% in_format_desc_.fps;

			// Audio

			std::shared_ptr<core::audio_buffer> audio_buffer;
			
			void* audio_bytes = nullptr;
			if(FAILED(audio->GetBytes(&audio_bytes)) || !audio_bytes)
				return S_OK;
			
			auto audio_frame = ffmpeg::create_frame();

			audio_frame->data[0]		= reinterpret_cast<uint8_t*>(audio_bytes);
			audio_frame->linesize[0]	= audio->GetSampleFrameCount()*out_format_desc_.audio_channels*sizeof(int32_t);
			audio_frame->nb_samples		= audio->GetSampleFrameCount();
			audio_frame->format			= AV_SAMPLE_FMT_S32;
						
			// Note: Uses 1 step rotated cadence for 1001 modes (1602, 1602, 1601, 1602, 1601)
			// This cadence fills the audio mixer most optimally.

			sync_buffer_.push_back(audio->GetSampleFrameCount()*out_format_desc_.audio_channels);		
			if(!boost::range::equal(sync_buffer_, audio_cadence_))
			{
				CASPAR_LOG(trace) << print() << L" Syncing audio.";
				return S_OK;
			}
			boost::range::rotate(audio_cadence_, std::begin(audio_cadence_)+1);
			
			// PUSH

			muxer_.push_video(video_frame);	
			muxer_.push_audio(audio_frame);											
			
			// POLL

			auto frame = core::draw_frame::late();
			if(!muxer_.empty())
			{
				frame = std::move(muxer_.front());
				muxer_.pop();

				if(!frame_buffer_.try_push(frame))
				{
					auto dummy = core::draw_frame::empty();
					frame_buffer_.try_pop(dummy);
					frame_buffer_.try_push(frame);
						
					graph_->set_tag("dropped-frame");
				}
			}
			
			graph_->set_value("frame-time", frame_timer.elapsed()*out_format_desc_.fps*0.5);	
			event_subject_ << monitor::event("profiler/time") % frame_timer.elapsed() % out_format_desc_.fps;

			graph_->set_value("output-buffer", static_cast<float>(frame_buffer_.size())/static_cast<float>(frame_buffer_.capacity()));	
			event_subject_ << monitor::event("buffer") % frame_buffer_.size() % frame_buffer_.capacity();
		}
		catch(...)
		{
			exception_ = std::current_exception();
			return E_FAIL;
		}

		return S_OK;
	}
	
	core::draw_frame get_frame()
	{
		if(exception_ != nullptr)
			std::rethrow_exception(exception_);
		
		core::draw_frame frame = core::draw_frame::late();
		if(!frame_buffer_.try_pop(frame))
			graph_->set_tag("late-frame");
		graph_->set_value("output-buffer", static_cast<float>(frame_buffer_.size())/static_cast<float>(frame_buffer_.capacity()));	
		return frame;
	}
	
	std::wstring print() const
	{
		return model_name_ + L" [" + boost::lexical_cast<std::wstring>(device_index_) + L"|" + in_format_desc_.name + L"]";
	}

	void subscribe(const monitor::observable::observer_ptr& o)
	{
		event_subject_.subscribe(o);
	}

	void unsubscribe(const monitor::observable::observer_ptr& o)
	{
		event_subject_.unsubscribe(o);
	}
};
	
class decklink_producer_proxy : public core::frame_producer_base
{		
	std::unique_ptr<decklink_producer>	producer_;
	const uint32_t						length_;
	executor							executor_;
public:
	explicit decklink_producer_proxy(const core::video_format_desc& in_format_desc,
									 const spl::shared_ptr<core::frame_factory>& frame_factory, 
									 const core::video_format_desc& out_format_desc, 
									 int device_index,
									 const std::wstring& filter_str, uint32_t length)
		: executor_(L"decklink_producer[" + boost::lexical_cast<std::wstring>(device_index) + L"]")
		, length_(length)
	{
		executor_.invoke([=]
		{
			CoInitialize(nullptr);
			producer_.reset(new decklink_producer(in_format_desc, device_index, frame_factory, out_format_desc, filter_str));
		});
	}

	~decklink_producer_proxy()
	{		
		executor_.invoke([=]
		{
			producer_.reset();
			CoUninitialize();
		});
	}

	void subscribe(const monitor::observable::observer_ptr& o) override
	{
		producer_->subscribe(o);
	}

	void unsubscribe(const monitor::observable::observer_ptr& o) override
	{
		producer_->unsubscribe(o);
	}
	
	// frame_producer
				
	core::draw_frame receive_impl() override
	{		
		return producer_->get_frame();
	}
			
	uint32_t nb_frames() const override
	{
		return length_;
	}
	
	std::wstring print() const override
	{
		return producer_->print();
	}
	
	std::wstring name() const override
	{
		return L"decklink";
	}

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"decklink");
		return info;
	}
};

spl::shared_ptr<core::frame_producer> create_producer(const spl::shared_ptr<core::frame_factory>& frame_factory, const core::video_format_desc& out_format_desc, const std::vector<std::wstring>& params)
{
	if(params.empty() || !boost::iequals(params[0], "decklink"))
		return core::frame_producer::empty();

	auto device_index	= get_param(L"DEVICE", params, -1);
	if(device_index == -1)
		device_index = boost::lexical_cast<int>(params.at(1));
	
	auto filter_str		= get_param(L"FILTER", params); 	
	auto length			= get_param(L"LENGTH", params, std::numeric_limits<uint32_t>::max()); 	
	auto in_format_desc = core::video_format_desc(get_param(L"FORMAT", params, L"INVALID"));
		
	if(in_format_desc.format == core::video_format::invalid)
		in_format_desc = out_format_desc;
			
	return create_destroy_proxy(spl::make_shared<decklink_producer_proxy>(in_format_desc, frame_factory, out_format_desc, device_index, filter_str, length));
}

}}