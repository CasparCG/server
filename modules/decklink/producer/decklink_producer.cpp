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

#include <common/diagnostics/graph.h>
#include <common/concurrency/com_context.h>
#include <common/exception/exceptions.h>
#include <common/memory/memclr.h>

#include <core/producer/frame/frame_factory.h>
#include <core/mixer/write_frame.h>

#include <tbb/concurrent_queue.h>
#include <tbb/atomic.h>

#include <boost/algorithm/string.hpp>
#include <boost/timer.hpp>

#pragma warning(push)
#pragma warning(disable : 4996)

	#include <atlbase.h>

	#include <atlcom.h>
	#include <atlhost.h>

#pragma warning(push)

#include <functional>

namespace caspar { 

class decklink_producer : public IDeckLinkInputCallback
{	
	CComPtr<IDeckLink>				decklink_;
	CComQIPtr<IDeckLinkInput>		input_;
	
	const std::wstring model_name_;
	const core::video_format_desc format_desc_;
	const size_t device_index_;

	std::shared_ptr<diagnostics::graph> graph_;
	boost::timer perf_timer_;
	
	std::vector<short>			audio_data_;

	std::shared_ptr<core::frame_factory> frame_factory_;

	tbb::concurrent_bounded_queue<safe_ptr<core::basic_frame>> frame_buffer_;
	safe_ptr<core::basic_frame> tail_;

	std::exception_ptr exception_;

public:
	decklink_producer(const core::video_format_desc& format_desc, size_t device_index, const std::shared_ptr<core::frame_factory>& frame_factory)
		: decklink_(get_device(device_index))
		, input_(decklink_)
		, model_name_(get_model_name(decklink_))
		, format_desc_(format_desc)
		, device_index_(device_index)
		, frame_factory_(frame_factory)
		, tail_(core::basic_frame::empty())
	{
		frame_buffer_.set_capacity(2);
		
		graph_ = diagnostics::create_graph(boost::bind(&decklink_producer::print, this));
		graph_->add_guide("tick-time", 0.5);
		graph_->set_color("tick-time", diagnostics::color(0.1f, 0.7f, 0.8f));
		graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
		graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
		graph_->set_color("output-buffer", diagnostics::color(0.0f, 1.0f, 0.0f));
		
		auto display_mode = get_display_mode(input_, format_desc_.format, bmdFormat8BitYUV, bmdVideoInputFlagDefault);
		
		// NOTE: bmdFormat8BitARGB is currently not supported by any decklink card. (2011-05-08)
		if(FAILED(input_->EnableVideoInput(display_mode, bmdFormat8BitYUV, 0))) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not enable video input."));

		if(FAILED(input_->EnableAudioInput(bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger, 2))) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not enable audio input."));
			
		if (FAILED(input_->SetCallback(this)) != S_OK)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to set input callback."));
			
		if(FAILED(input_->StartStreams()))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to start input stream."));

		CASPAR_LOG(info) << print() << L" Successfully Initialized.";
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
		try
		{
			graph_->update_value("tick-time", perf_timer_.elapsed()*format_desc_.fps*0.5);
			perf_timer_.restart();
						
			core::pixel_format_desc desc;
			desc.pix_fmt = core::pixel_format::ycbcr;
			desc.planes.push_back(core::pixel_format_desc::plane(format_desc_.width,   format_desc_.height, 1));
			desc.planes.push_back(core::pixel_format_desc::plane(format_desc_.width/2, format_desc_.height, 1));
			desc.planes.push_back(core::pixel_format_desc::plane(format_desc_.width/2, format_desc_.height, 1));			
			auto frame = frame_factory_->create_frame(this, desc);
						
			void* bytes = nullptr;
			video->GetBytes(&bytes);
			unsigned char* data = reinterpret_cast<unsigned char*>(bytes);
			const size_t frame_size = (format_desc_.width * 16 / 8) * format_desc_.height;

			// Convert to planar YUV422
			unsigned char* y  = frame->image_data(0).begin();
			unsigned char* cb = frame->image_data(1).begin();
			unsigned char* cr = frame->image_data(2).begin();
		
			tbb::parallel_for(tbb::blocked_range<size_t>(0, frame_size/4), [&](const tbb::blocked_range<size_t>& r)
			{
				for(auto n = r.begin(); n != r.end(); ++n)
				{
					cb[n]	  = data[n*4+0];
					y [n*2+0] = data[n*4+1];
					cr[n]	  = data[n*4+2];
					y [n*2+1] = data[n*4+3];
				}
			});

			frame->commit();

			// It is assumed that audio is always equal or ahead of video.
			if(audio && SUCCEEDED(audio->GetBytes(&bytes)))
			{
				const size_t audio_samples = static_cast<size_t>(48000.0 / format_desc_.fps);
				const size_t audio_nchannels = 2;

				auto sample_frame_count = audio->GetSampleFrameCount();
				auto audio_data = reinterpret_cast<short*>(bytes);
				audio_data_.insert(audio_data_.end(), audio_data, audio_data + sample_frame_count*2);

				if(audio_data_.size() > audio_samples*audio_nchannels)
				{
					frame->audio_data() = std::vector<short>(audio_data_.begin(), audio_data_.begin() +  audio_samples*audio_nchannels);
					audio_data_.erase(audio_data_.begin(), audio_data_.begin() +  audio_samples*audio_nchannels);
				}
			}

			if(!frame_buffer_.try_push(frame))
				graph_->add_tag("dropped-frame");
			graph_->set_value("output-buffer", static_cast<float>(frame_buffer_.size())/static_cast<float>(frame_buffer_.capacity()));	
		}
		catch(...)
		{
			exception_ = std::current_exception();
			return E_FAIL;
		}

		return S_OK;
	}
	
	safe_ptr<core::basic_frame> get_frame()
	{
		if(exception_ != nullptr)
			std::rethrow_exception(exception_);

		if(!frame_buffer_.try_pop(tail_))
			graph_->add_tag("late-frame");
		graph_->set_value("output-buffer", static_cast<float>(frame_buffer_.size())/static_cast<float>(frame_buffer_.capacity()));	
		return tail_;
	}
	
	std::wstring print() const
	{
		return model_name_ + L" [" + boost::lexical_cast<std::wstring>(device_index_) + L"]";
	}
};
	
class decklink_producer_proxy : public core::frame_producer
{		
	com_context<decklink_producer> context_;
public:

	explicit decklink_producer_proxy(const safe_ptr<core::frame_factory>& frame_factory, const core::video_format_desc& format_desc, size_t device_index)
		: context_(L"decklink_producer[" + boost::lexical_cast<std::wstring>(device_index) + L"]")
	{
		context_.reset([&]{return new decklink_producer(format_desc, device_index, frame_factory);}); 
	}
				
	virtual safe_ptr<core::basic_frame> receive()
	{
		return context_->get_frame();
	}
	
	std::wstring print() const
	{
		return context_->print();
	}
};

safe_ptr<core::frame_producer> create_decklink_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params)
{
	if(params.empty() || !boost::iequals(params[0], "decklink"))
		return core::frame_producer::empty();

	size_t device_index = 1;
	if(params.size() > 1)
		device_index = lexical_cast_or_default(params[1], 1);

	core::video_format_desc format_desc = core::video_format_desc::get(L"PAL");
	if(params.size() > 2)
	{
		auto desc = core::video_format_desc::get(params[2]);
		if(desc.format != core::video_format::invalid)
			format_desc = desc;
	}

	return make_safe<decklink_producer_proxy>(frame_factory, format_desc, device_index);
}

}