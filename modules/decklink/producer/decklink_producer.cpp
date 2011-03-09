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

#include "../interop/DeckLinkAPI_h.h" // TODO: Change this
#include "../util/util.h" // TODO: Change this

#include <common/diagnostics/graph.h>
#include <common/concurrency/executor.h>
#include <common/exception/exceptions.h>
#include <common/utility/timer.h>

#include <mixer/frame/write_frame.h>

#include <tbb/concurrent_queue.h>
#include <tbb/atomic.h>

#include <boost/algorithm/string.hpp>

#pragma warning(push)
#pragma warning(disable : 4996)

	#include <atlbase.h>

	#include <atlcom.h>
	#include <atlhost.h>

#pragma warning(push)

#include <functional>

namespace caspar { 

class decklink_input : public IDeckLinkInputCallback
{
	struct co_init
	{
		co_init(){CoInitialize(nullptr);}
		~co_init(){CoUninitialize();}
	} co_;

	printer parent_printer_;
	const core::video_format_desc format_desc_;
	std::wstring model_name_;
	const size_t device_index_;

	std::shared_ptr<diagnostics::graph> graph_;
	timer perf_timer_;

	CComPtr<IDeckLink>			decklink_;
	CComQIPtr<IDeckLinkInput>	input_;
	IDeckLinkDisplayMode*		d_mode_;

	std::vector<short>			audio_data_;

	std::shared_ptr<core::frame_factory> frame_factory_;

	tbb::concurrent_bounded_queue<safe_ptr<core::draw_frame>> frame_buffer_;
	safe_ptr<core::draw_frame> tail_;

public:

	decklink_input(const core::video_format_desc& format_desc, size_t device_index, const std::shared_ptr<core::frame_factory>& frame_factory, const printer& parent_printer)
		: parent_printer_(parent_printer)
		, format_desc_(format_desc)
		, device_index_(device_index)
		, model_name_(L"DECKLINK")
		, frame_factory_(frame_factory)
		, tail_(core::draw_frame::empty())
	{
		frame_buffer_.set_capacity(4);
		
		CComPtr<IDeckLinkIterator> pDecklinkIterator;
		if(FAILED(pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " No Decklink drivers installed."));

		size_t n = 0;
		while(n < device_index_ && pDecklinkIterator->Next(&decklink_) == S_OK){++n;}	

		if(n != device_index_ || !decklink_)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Decklink card not found.") << arg_name_info("device_index") << arg_value_info(boost::lexical_cast<std::string>(device_index_)));

		input_ = decklink_;

		BSTR pModelName;
		decklink_->GetModelName(&pModelName);
		model_name_ = std::wstring(pModelName);

		graph_ = diagnostics::create_graph(boost::bind(&decklink_input::print, this));
		graph_->add_guide("tick-time", 0.5);
		graph_->set_color("tick-time", diagnostics::color(0.1f, 0.7f, 0.8f));
		
		unsigned long decklinkVideoFormat = GetDecklinkVideoFormat(format_desc.format);
		if(decklinkVideoFormat == ULONG_MAX) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Card does not support requested videoformat."));

		d_mode_ = get_display_mode((BMDDisplayMode)decklinkVideoFormat);
		if(d_mode_ == nullptr) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Card does not support requested videoformat."));

		BMDDisplayModeSupport displayModeSupport;
		if(FAILED(input_->DoesSupportVideoMode((BMDDisplayMode)decklinkVideoFormat, bmdFormat8BitYUV, bmdVideoOutputFlagDefault, &displayModeSupport, nullptr)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Card does not support requested videoformat."));

		// NOTE: bmdFormat8BitARGB does not seem to work with Decklink HD Extreme 3D
		if(FAILED(input_->EnableVideoInput((BMDDisplayMode)decklinkVideoFormat, bmdFormat8BitYUV, 0))) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not enable video input."));

		if(FAILED(input_->EnableAudioInput(bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger, 2))) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not enable audio input."));
			
		if (FAILED(input_->SetCallback(this)) != S_OK)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to set input callback."));
			
		if(FAILED(input_->StartStreams()))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to start input stream."));

		CASPAR_LOG(info) << print() << " successfully initialized decklink for " << format_desc_.name;
	}

	~decklink_input()
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
		d_mode_ = newDisplayMode;
		return S_OK;
	}

	// TODO: Enable audio input
	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame* video, IDeckLinkAudioInputPacket* audio)
	{	
		graph_->update_value("tick-time", static_cast<float>(perf_timer_.elapsed()/format_desc_.interval*0.5));
		perf_timer_.reset();

		if(!video)
			return S_OK;		
				
		void* bytes = nullptr;
		if(FAILED(video->GetBytes(&bytes)))
			return S_OK;

		core::pixel_format_desc desc;
		desc.pix_fmt = core::pixel_format::ycbcr;
		desc.planes.push_back(core::pixel_format_desc::plane(d_mode_->GetWidth(),   d_mode_->GetHeight(), 1));
		desc.planes.push_back(core::pixel_format_desc::plane(d_mode_->GetWidth()/2, d_mode_->GetHeight(), 1));
		desc.planes.push_back(core::pixel_format_desc::plane(d_mode_->GetWidth()/2, d_mode_->GetHeight(), 1));			
		auto frame = frame_factory_->create_frame(desc);

		unsigned char* data = reinterpret_cast<unsigned char*>(bytes);
		int frame_size = (d_mode_->GetWidth() * 16 / 8) * d_mode_->GetHeight();

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

		if(audio && SUCCEEDED(audio->GetBytes(&bytes)))
		{
			auto sample_frame_count = audio->GetSampleFrameCount();
			auto audio_data = reinterpret_cast<short*>(bytes);
			audio_data_.insert(audio_data_.end(), audio_data, audio_data + sample_frame_count*2);

			if(audio_data_.size() > 3840)
			{
				frame->audio_data() = std::vector<short>(audio_data_.begin(), audio_data_.begin() + 3840);
				audio_data_.erase(audio_data_.begin(), audio_data_.begin() + 3840);
				frame_buffer_.try_push(frame);
			}
		}
		else
			frame_buffer_.try_push(frame);

		return S_OK;
	}

	IDeckLinkDisplayMode* get_display_mode(BMDDisplayMode mode)
	{	
		CComPtr<IDeckLinkDisplayModeIterator>	iterator;
		IDeckLinkDisplayMode*					d_mode;
	
		if (input_->GetDisplayModeIterator(&iterator) != S_OK)
			return nullptr;

		while (iterator->Next(&d_mode) == S_OK)
		{
			if(d_mode->GetDisplayMode() == mode)
				return d_mode;
		}
		return nullptr;
	}

	safe_ptr<core::draw_frame> get_frame()
	{
		frame_buffer_.try_pop(tail_);
		return tail_;
	}

	std::wstring print() const
	{
		return (parent_printer_ ? parent_printer_() + L"/" : L"") + L" [" + model_name_ + L"device:" + boost::lexical_cast<std::wstring>(device_index_) + L"]";
	}
};
	
class decklink_producer : public core::frame_producer
{	
	const size_t device_index_;
	printer parent_printer_;

	std::unique_ptr<decklink_input> input_;
	
	const core::video_format_desc format_desc_;
	
	executor executor_;
public:

	explicit decklink_producer(const core::video_format_desc& format_desc, size_t device_index)
		: format_desc_(format_desc) 
		, device_index_(device_index){}

	~decklink_producer()
	{	
		executor_.invoke([this]
		{
			input_ = nullptr;
		});
	}

	virtual void initialize(const safe_ptr<core::frame_factory>& frame_factory)
	{
		executor_.start();
		executor_.invoke([=]
		{
			input_.reset(new decklink_input(format_desc_, device_index_, frame_factory, parent_printer_));
		});
	}

	virtual void set_parent_printer(const printer& parent_printer) { parent_printer_ = parent_printer;}
	
	virtual safe_ptr<core::draw_frame> receive()
	{
		return input_->get_frame();
	}
	
	std::wstring print() const
	{
		return  (parent_printer_ ? parent_printer_() + L"/" : L"") + (input_ ? input_->print() : L"Unknown Decklink [input-" + boost::lexical_cast<std::wstring>(device_index_) + L"]");
	}
};

safe_ptr<core::frame_producer> create_decklink_producer(const std::vector<std::wstring>& params)
{
	if(params.empty() || !boost::iequals(params[0], "decklink"))
		return core::frame_producer::empty();

	size_t device_index = 1;
	if(params.size() > 1)
	{
		try{device_index = boost::lexical_cast<int>(params[1]);}
		catch(boost::bad_lexical_cast&){}
	}

	core::video_format_desc format_desc = core::video_format_desc::get(L"PAL");
	if(params.size() > 2)
	{
		format_desc = core::video_format_desc::get(params[2]);
		format_desc = format_desc.format != core::video_format::invalid ? format_desc : core::video_format_desc::get(L"PAL");
	}

	return make_safe<decklink_producer>(format_desc, device_index);
}

}