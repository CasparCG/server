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
 
#include "../stdafx.h"

#include "decklink_producer.h"

#include "../interop/DeckLinkAPI_h.h"
#include "../util/util.h"

#include "../../ffmpeg/producer/util/util.h"
#include "../../ffmpeg/producer/muxer/frame_muxer.h"

#include <common/concurrency/governor.h>
#include <common/diagnostics/graph.h>
#include <common/exception/exceptions.h>
#include <common/log/log.h>
#include <common/utility/co_init.h>

#include <core/producer/frame/frame_factory.h>
#include <core/producer/frame/basic_frame.h>

#include <agents.h>

#include <boost/algorithm/string.hpp>
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

namespace caspar { namespace decklink {
		
class decklink_producer : public core::frame_producer, public IDeckLinkInputCallback
{		
	safe_ptr<diagnostics::graph>												graph_;
	
	const core::video_format_desc												format_desc_;
	const size_t																device_index_;

	safe_ptr<core::basic_frame>													last_frame_;
	const int64_t																length_;
	int64_t																		frame_number_;
	
	CComPtr<IDeckLink>															decklink_;
	CComQIPtr<IDeckLinkInput>													input_;

	const std::wstring															model_name_;

	Concurrency::unbounded_buffer<ffmpeg::frame_muxer2::video_source_element_t>	video_;
	Concurrency::unbounded_buffer<ffmpeg::frame_muxer2::audio_source_element_t>	audio_;
	Concurrency::unbounded_buffer<ffmpeg::frame_muxer2::target_element_t>		frames_;
			
	ffmpeg::frame_muxer2														muxer_;
		
	governor																	governor_;

public:
	decklink_producer(const co_init& com, const safe_ptr<core::frame_factory>& frame_factory, const core::video_format_desc& format_desc, size_t device_index, const std::wstring& filter_str, int64_t length)
		: format_desc_(format_desc)
		, device_index_(device_index)
		, length_(length)
		, frame_number_(0)
		, decklink_(get_device(device_index))
		, input_(decklink_)
		, model_name_(get_model_name(decklink_))
		, muxer_(&video_, &audio_, frames_, format_desc.fps, frame_factory, filter_str)
		, governor_(1)
	{		
		graph_->add_guide("tick-time", 0.5);
		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));	
		graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
		graph_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));
		graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
		graph_->set_color("output-buffer", diagnostics::color(0.0f, 1.0f, 0.0f));
		graph_->set_text(narrow(print()));
		diagnostics::register_graph(graph_);
		
		auto display_mode = get_display_mode(input_, format_desc_.format, bmdFormat8BitYUV, bmdVideoInputFlagDefault);
		
		Concurrency::scoped_oversubcription_token oversubscribe;
		
		// NOTE: bmdFormat8BitARGB is currently not supported by any decklink card. (2011-05-08)
		if(FAILED(input_->EnableVideoInput(display_mode, bmdFormat8BitYUV, 0))) 
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

		CASPAR_LOG(info) << print() << L" Successfully Initialized.";
	}

	~decklink_producer()
	{
		governor_.cancel();
		
		send(video_, ffmpeg::eof_video());
		send(audio_, ffmpeg::eof_audio());

		Concurrency::scoped_oversubcription_token oversubscribe;
		if(input_ != nullptr) 
		{
			input_->StopStreams();
			input_->DisableVideoInput();
		}

		CASPAR_LOG(info) << print() << L" Successfully Uninitialized.";
	}

	virtual HRESULT STDMETHODCALLTYPE	QueryInterface (REFIID, LPVOID*)	{return E_NOINTERFACE;}
	virtual ULONG STDMETHODCALLTYPE		AddRef ()							{return 1;}
	virtual ULONG STDMETHODCALLTYPE		Release ()							{return 1;}
		
	virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents /*notificationEvents*/, IDeckLinkDisplayMode* newDisplayMode, BMDDetectedVideoInputFormatFlags /*detectedSignalFlags*/)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame* raw_video, IDeckLinkAudioInputPacket* raw_audio)
	{	
		try
		{
			ticket_t ticket;
			if(!governor_.try_acquire(ticket))
			{
				graph_->add_tag("dropped-frame");
				return S_OK;
			}

			CComPtr<IDeckLinkVideoInputFrame>  video = raw_video;
			CComPtr<IDeckLinkAudioInputPacket> audio = raw_audio;

			void* bytes = nullptr;

			if(!video || FAILED(video->GetBytes(&bytes)) || !bytes)
				return S_OK;
			
			safe_ptr<AVFrame> av_frame(avcodec_alloc_frame(), av_free);	
			avcodec_get_frame_defaults(av_frame.get());
		
			av_frame = safe_ptr<AVFrame>(av_frame.get(), [av_frame, video, ticket](AVFrame*){});
								
			av_frame->data[0]			= reinterpret_cast<uint8_t*>(bytes);
			av_frame->linesize[0]		= video->GetRowBytes();			
			av_frame->format			= PIX_FMT_UYVY422;
			av_frame->width				= video->GetWidth();
			av_frame->height			= video->GetHeight();
			av_frame->interlaced_frame	= format_desc_.field_mode != core::field_mode::progressive;
			av_frame->top_field_first	= format_desc_.field_mode == core::field_mode::upper ? 1 : 0;
					
			Concurrency::send(video_, av_frame);

			// It is assumed that audio is always equal or ahead of video.
			if(!audio || FAILED(audio->GetBytes(&bytes)) || !bytes)
				Concurrency::send(audio_, make_safe<core::audio_buffer>(format_desc_.audio_samples_per_frame, 0));	
			else
			{
				auto sample_frame_count = audio->GetSampleFrameCount();
				auto audio_data = reinterpret_cast<int32_t*>(bytes);
				auto audio_buffer = make_safe<core::audio_buffer>(audio_data, audio_data + sample_frame_count*format_desc_.audio_channels);			
				audio_buffer = safe_ptr<core::audio_buffer>(audio_buffer.get(), [audio_buffer, audio, ticket](core::audio_buffer*){});
				Concurrency::send(audio_, audio_buffer);
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			return E_FAIL;
		}
		
		return S_OK;
	}
		
	virtual safe_ptr<core::basic_frame> receive(int)
	{
		auto frame = core::basic_frame::late();

		try
		{
			if(frame_number_ > length_)
				return core::basic_frame::eof();

			last_frame_ = frame = Concurrency::receive(frames_, 10).first;
			++frame_number_;
		}
		catch(Concurrency::operation_timed_out&)
		{		
			graph_->add_tag("underflow");	
		}

		return frame;
	}

	virtual safe_ptr<core::basic_frame> last_frame() const
	{
		return disable_audio(last_frame_);
	}
	
	virtual int64_t nb_frames() const 
	{
		return length_;
	}
	
	virtual std::wstring print() const
	{
		return model_name_ + L" [" + boost::lexical_cast<std::wstring>(device_index_) + L"]";
	}
};
	
safe_ptr<core::frame_producer> create_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params)
{
	if(params.empty() || !boost::iequals(params[0], "decklink"))
		return core::frame_producer::empty();
	
	std::vector<std::wstring> params2 = params;
	std::for_each(std::begin(params2), std::end(params2), std::bind(&boost::to_upper<std::wstring>, std::placeholders::_1, std::locale()));

	auto device_index	= core::get_param(L"DEVICE", params2, 1);
	auto filter_str		= core::get_param<std::wstring>(L"FILTER", params2, L""); 	
	auto length			= core::get_param(L"LENGTH", params2, std::numeric_limits<int64_t>::max()); 	
	
	boost::replace_all(filter_str, L"DEINTERLACE", L"YADIF=0:-1");
	boost::replace_all(filter_str, L"DEINTERLACE_BOB", L"YADIF=1:-1");

	auto format_desc	= core::video_format_desc::get(core::get_param<std::wstring>(L"FORMAT", params2, L"INVALID"));

	if(format_desc.format == core::video_format::invalid)
		format_desc = frame_factory->get_video_format_desc();
			
	co_init com;
	return make_safe<decklink_producer>(com, frame_factory, format_desc, device_index, filter_str, length);
}

}}