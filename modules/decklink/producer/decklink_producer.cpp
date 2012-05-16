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

#include <common/concurrency/com_context.h>
#include <common/diagnostics/graph.h>
#include <common/exception/exceptions.h>
#include <common/log/log.h>
#include <common/memory/memclr.h>
#include <common/utility/param.h>

#include <core/mixer/write_frame.h>
#include <core/producer/frame/frame_transform.h>
#include <core/producer/frame/frame_factory.h>

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
	safe_ptr<diagnostics::graph>								graph_;
	boost::timer												tick_timer_;
	boost::timer												frame_timer_;

	CComPtr<IDeckLink>											decklink_;
	CComQIPtr<IDeckLinkInput>									input_;
	CComQIPtr<IDeckLinkAttributes >								attributes_;
	
	const std::wstring											model_name_;
	const size_t												device_index_;
	const std::wstring											filter_;
	
	core::video_format_desc										format_desc_;
	std::vector<size_t>											audio_cadence_;
	boost::circular_buffer<size_t>								sync_buffer_;
	ffmpeg::frame_muxer											muxer_;
			
	tbb::atomic<int>											hints_;
	safe_ptr<core::frame_factory>								frame_factory_;

	tbb::concurrent_bounded_queue<safe_ptr<core::basic_frame>>	frame_buffer_;

	std::exception_ptr											exception_;		

public:
	decklink_producer(const core::video_format_desc& format_desc, size_t device_index, const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filter, std::size_t buffer_depth)
		: decklink_(get_device(device_index))
		, input_(decklink_)
		, attributes_(decklink_)
		, model_name_(get_model_name(decklink_))
		, device_index_(device_index)
		, filter_(filter)
		, format_desc_(format_desc)
		, audio_cadence_(format_desc.audio_cadence)
		, muxer_(format_desc.fps, frame_factory, filter)
		, sync_buffer_(format_desc.audio_cadence.size())
		, frame_factory_(frame_factory)
	{		
		hints_ = 0;
		frame_buffer_.set_capacity(buffer_depth);
		
		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));	
		graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
		graph_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));
		graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
		graph_->set_color("output-buffer", diagnostics::color(0.0f, 1.0f, 0.0f));
		graph_->set_text(print());
		diagnostics::register_graph(graph_);
		
		auto display_mode = get_display_mode(input_, format_desc_.format, bmdFormat8BitYUV, bmdVideoInputFlagDefault);
				
		// NOTE: bmdFormat8BitARGB is currently not supported by any decklink card. (2011-05-08)
		if(FAILED(input_->EnableVideoInput(display_mode, bmdFormat8BitYUV, bmdVideoInputFlagDefault))) 
			BOOST_THROW_EXCEPTION(caspar_exception() 
									<< msg_info(narrow(print()) + " Could not enable video input.")
									<< boost::errinfo_api_function("EnableVideoInput"));

		if(FAILED(input_->EnableAudioInput(bmdAudioSampleRate48kHz, bmdAudioSampleType32bitInteger, format_desc_.audio_channels))) 
			BOOST_THROW_EXCEPTION(caspar_exception() 
									<< msg_info(narrow(print()) + " Could not enable audio input.")
									<< boost::errinfo_api_function("EnableAudioInput"));
			
		if (FAILED(input_->SetCallback(this)) != S_OK)
			BOOST_THROW_EXCEPTION(caspar_exception() 
									<< msg_info(narrow(print()) + " Failed to set input callback.")
									<< boost::errinfo_api_function("SetCallback"));
			
		if(FAILED(input_->StartStreams()))
			BOOST_THROW_EXCEPTION(caspar_exception() 
									<< msg_info(narrow(print()) + " Failed to start input stream.")
									<< boost::errinfo_api_function("StartStreams"));
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
		
	virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents, IDeckLinkDisplayMode* newDisplayMode, BMDDetectedVideoInputFormatFlags /*detectedSignalFlags*/)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame* video, IDeckLinkAudioInputPacket* audio)
	{	
		if(!video)
			return S_OK;

		try
		{
			graph_->set_value("tick-time", tick_timer_.elapsed()*format_desc_.fps*0.5);
			tick_timer_.restart();

			frame_timer_.restart();

			// PUSH

			void* bytes = nullptr;
			if(FAILED(video->GetBytes(&bytes)) || !bytes)
				return S_OK;
			
			safe_ptr<AVFrame> av_frame(avcodec_alloc_frame(), av_free);	
			avcodec_get_frame_defaults(av_frame.get());
						
			av_frame->data[0]			= reinterpret_cast<uint8_t*>(bytes);
			av_frame->linesize[0]		= video->GetRowBytes();			
			av_frame->format			= PIX_FMT_UYVY422;
			av_frame->width				= video->GetWidth();
			av_frame->height			= video->GetHeight();
			av_frame->interlaced_frame	= format_desc_.field_mode != core::field_mode::progressive;
			av_frame->top_field_first	= format_desc_.field_mode == core::field_mode::upper ? 1 : 0;
				
			std::shared_ptr<core::audio_buffer> audio_buffer;

			// It is assumed that audio is always equal or ahead of video.
			if(audio && SUCCEEDED(audio->GetBytes(&bytes)) && bytes)
			{
				auto sample_frame_count = audio->GetSampleFrameCount();
				auto audio_data = reinterpret_cast<int32_t*>(bytes);
				audio_buffer = std::make_shared<core::audio_buffer>(audio_data, audio_data + sample_frame_count*format_desc_.audio_channels);
			}
			else			
				audio_buffer = std::make_shared<core::audio_buffer>(audio_cadence_.front(), 0);
			
			// Note: Uses 1 step rotated cadence for 1001 modes (1602, 1602, 1601, 1602, 1601)
			// This cadence fills the audio mixer most optimally.

			sync_buffer_.push_back(audio_buffer->size());		
			if(!boost::range::equal(sync_buffer_, audio_cadence_))
			{
				CASPAR_LOG(trace) << print() << L" Syncing audio.";
				return S_OK;
			}

			muxer_.push(audio_buffer);
			muxer_.push(av_frame, hints_);	
											
			boost::range::rotate(audio_cadence_, std::begin(audio_cadence_)+1);
			
			// POLL
			
			for(auto frame = muxer_.poll(); frame; frame = muxer_.poll())
			{
				if(!frame_buffer_.try_push(make_safe_ptr(frame)))
				{
					auto dummy = core::basic_frame::empty();
					frame_buffer_.try_pop(dummy);

					frame_buffer_.try_push(make_safe_ptr(frame));

					graph_->set_tag("dropped-frame");
				}
			}

			graph_->set_value("frame-time", frame_timer_.elapsed()*format_desc_.fps*0.5);

			graph_->set_value("output-buffer", static_cast<float>(frame_buffer_.size())/static_cast<float>(frame_buffer_.capacity()));	
		}
		catch(...)
		{
			exception_ = std::current_exception();
			return E_FAIL;
		}

		return S_OK;
	}
	
	safe_ptr<core::basic_frame> get_frame(int hints)
	{
		if(exception_ != nullptr)
			std::rethrow_exception(exception_);

		hints_ = hints;

		safe_ptr<core::basic_frame> frame = core::basic_frame::late();
		if(!frame_buffer_.try_pop(frame))
			graph_->set_tag("late-frame");
		graph_->set_value("output-buffer", static_cast<float>(frame_buffer_.size())/static_cast<float>(frame_buffer_.capacity()));	
		return frame;
	}
	
	std::wstring print() const
	{
		return model_name_ + L" [" + boost::lexical_cast<std::wstring>(device_index_) + L"|" + format_desc_.name + L"]";
	}
};
	
class decklink_producer_proxy : public core::frame_producer
{		
	safe_ptr<core::basic_frame>		last_frame_;
	com_context<decklink_producer>	context_;
	const uint32_t					length_;
public:

	explicit decklink_producer_proxy(const safe_ptr<core::frame_factory>& frame_factory, const core::video_format_desc& format_desc, size_t device_index, const std::wstring& filter_str, uint32_t length, std::size_t buffer_depth)
		: context_(L"decklink_producer[" + boost::lexical_cast<std::wstring>(device_index) + L"]")
		, last_frame_(core::basic_frame::empty())
		, length_(length)
	{
		context_.reset([&]{return new decklink_producer(format_desc, device_index, frame_factory, filter_str, buffer_depth);}); 
	}
	
	// frame_producer
				
	virtual safe_ptr<core::basic_frame> receive(int hints) override
	{
		auto frame = context_->get_frame(hints);
		if(frame != core::basic_frame::late())
			last_frame_ = frame;
		return frame;
	}

	virtual safe_ptr<core::basic_frame> last_frame() const override
	{
		return disable_audio(last_frame_);
	}
	
	virtual uint32_t nb_frames() const override
	{
		return length_;
	}
	
	std::wstring print() const override
	{
		return context_->print();
	}

	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"decklink-producer");
		return info;
	}
};

safe_ptr<core::frame_producer> create_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params)
{
	if(params.empty() || !boost::iequals(params[0], "decklink"))
		return core::frame_producer::empty();

	auto device_index	= get_param(L"DEVICE", params, -1);
	if(device_index == -1)
		device_index = boost::lexical_cast<int>(params.at(1));
	auto filter_str		= get_param(L"FILTER", params); 	
	auto length			= get_param(L"LENGTH", params, std::numeric_limits<uint32_t>::max()); 	
	auto buffer_depth	= get_param(L"BUFFER", params, 2); 	
	auto format_desc	= core::video_format_desc::get(get_param(L"FORMAT", params, L"INVALID"));
	
	boost::replace_all(filter_str, L"DEINTERLACE", L"YADIF=0:-1");
	boost::replace_all(filter_str, L"DEINTERLACE_BOB", L"YADIF=1:-1");
	
	if(format_desc.format == core::video_format::invalid)
		format_desc = frame_factory->get_video_format_desc();
			
	return create_producer_print_proxy(
		   create_producer_destroy_proxy(
			make_safe<decklink_producer_proxy>(frame_factory, format_desc, device_index, filter_str, length, buffer_depth)));
}

}}