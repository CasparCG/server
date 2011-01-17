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
 
#include "../../stdafx.h"

#include "decklink_producer.h"

#include "../../mixer/frame/draw_frame.h"
#include "../../consumer/decklink/DeckLinkAPI_h.h" // TODO: Change this
#include "../../consumer/decklink/util.h" // TODO: Change this

#include <common/concurrency/executor.h>
#include <common/exception/exceptions.h>

#include <tbb/concurrent_queue.h>
#include <tbb/atomic.h>

#include <boost/algorithm/string.hpp>

#pragma warning(push)
#pragma warning(disable : 4996)

	#include <atlbase.h>

	#include <atlcom.h>
	#include <atlhost.h>

#pragma warning(push)

namespace caspar { namespace core {

class decklink_input : public IDeckLinkInputCallback
{
	struct co_init
	{
		co_init(){CoInitialize(nullptr);}
		~co_init(){CoUninitialize();}
	} co_;

	const video_format_desc format_desc_;
	const size_t device_index_;

	CComPtr<IDeckLink>			decklink_;
	CComQIPtr<IDeckLinkInput>	input_;

	std::shared_ptr<frame_factory> frame_factory_;

	tbb::concurrent_bounded_queue<safe_ptr<draw_frame>> frame_buffer_;
	safe_ptr<draw_frame> head_;
	safe_ptr<draw_frame> tail_;

public:

	decklink_input(size_t device_index, const video_format_desc& format_desc, const std::shared_ptr<frame_factory>& frame_factory)
		: device_index_(device_index)
		, format_desc_(format_desc)
		, frame_factory_(frame_factory)
		, head_(draw_frame::empty())
		, tail_(draw_frame::empty())
	{
		frame_buffer_.set_capacity(4);
		
		CComPtr<IDeckLinkIterator> pDecklinkIterator;
		if(FAILED(pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("decklink_producer: No Decklink drivers installed."));

		size_t n = 0;
		while(n < device_index_ && pDecklinkIterator->Next(&decklink_) == S_OK){++n;}	

		if(n != device_index_ || !decklink_)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("decklink_producer: No Decklink card found.") << arg_name_info("device_index") << arg_value_info(boost::lexical_cast<std::string>(device_index_)));

		input_ = decklink_;

		BSTR pModelName;
		decklink_->GetModelName(&pModelName);
		if(pModelName != nullptr)
			CASPAR_LOG(info) << "decklink_producer: Modelname: " << pModelName;
		
		unsigned long decklinkVideoFormat = GetDecklinkVideoFormat(format_desc_.format);
		if(decklinkVideoFormat == ULONG_MAX) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("decklink_producer: Card does not support requested videoformat."));

		BMDDisplayModeSupport displayModeSupport;
		if(FAILED(input_->DoesSupportVideoMode((BMDDisplayMode)decklinkVideoFormat, bmdFormat8BitYUV, &displayModeSupport)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("decklink_producer: Card does not support requested videoformat."));

		// NOTE: bmdFormat8BitABGR does not seem to work with Decklink HD Extreme 3D
		if(FAILED(input_->EnableVideoInput((BMDDisplayMode)decklinkVideoFormat, bmdFormat8BitYUV, 0))) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Could not enable video input."));
			
		if (FAILED(input_->SetCallback(this)) != S_OK)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("decklink_producer: Failed to set input callback."));
			
		if(FAILED(input_->StartStreams()))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Failed to start input stream."));

		CASPAR_LOG(info) << "decklink_producer: Successfully initialized decklink for " << format_desc_.name;
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
		
	virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents /*notificationEvents*/, IDeckLinkDisplayMode* /*newDisplayMode*/, BMDDetectedVideoInputFormatFlags /*detectedSignalFlags*/)
	{
		return S_OK;
	}

	// TODO: Enable audio input
	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame* video, IDeckLinkAudioInputPacket* /*audio*/)
	{		
		if(!frame_buffer_.try_push(head_) || video == nullptr)
			return S_OK;		
				
		void* bytes = nullptr;
		if(FAILED(video->GetBytes(&bytes)))
			return S_OK;

		pixel_format_desc desc;
		desc.pix_fmt = pixel_format::ycbcr;
		desc.planes.push_back(pixel_format_desc::plane(format_desc_.width,   format_desc_.height, 1));
		desc.planes.push_back(pixel_format_desc::plane(format_desc_.width/2, format_desc_.height, 1));
		desc.planes.push_back(pixel_format_desc::plane(format_desc_.width/2, format_desc_.height, 1));			
		auto frame = frame_factory_->create_frame(desc);

		unsigned char* data = reinterpret_cast<unsigned char*>(bytes);
		int frame_size = (format_desc_.width * 16 / 8) * format_desc_.height;

		// Convert to planar YUV422
		unsigned char* y  = frame->image_data(0).begin();
		unsigned char* cb = frame->image_data(1).begin();
		unsigned char* cr = frame->image_data(2).begin();
		
		tbb::parallel_for(tbb::blocked_range<size_t>(0, frame_size/4), 
		[&](const tbb::blocked_range<size_t>& r)
		{
			for(auto n = r.begin(); n != r.end(); ++n)
			{
				cb[n]	  = data[n*4+0];
				y [n*2+0] = data[n*4+1];
				cr[n]	  = data[n*4+2];
				y [n*2+1] = data[n*4+3];
			}
		});

		head_ = frame;

		return S_OK;
	}

	safe_ptr<draw_frame> get_frame()
	{
		frame_buffer_.try_pop(tail_);
		return tail_;
	}
};
	
// NOTE: With this design it will probably not be possible to do input and output from same decklink device, as there will be 2 threads trying to use the same device.
// decklink_consumer and decklink_producer will need to use a common active object decklink_device which handles all processing for decklink device.
class decklink_producer : public frame_producer
{
	executor executor_;
	
	const video_format_desc format_desc_;
	const size_t device_index_;

	std::unique_ptr<decklink_input> input_;
		
public:

	explicit decklink_producer(const video_format_desc format_desc, size_t device_index)
		: device_index_(device_index)
		, format_desc_(format_desc){}

	~decklink_producer()
	{	
		executor_.invoke([this]
		{
			input_ = nullptr;
		});
	}
	
	virtual safe_ptr<draw_frame> receive()
	{
		return input_->get_frame();
	}

	virtual void initialize(const safe_ptr<frame_factory>& frame_factory)
	{
		executor_.start();
		executor_.invoke([=]
		{
			input_.reset(new decklink_input(device_index_, format_desc_, frame_factory));
		});
	}
	
	virtual std::wstring print() const { return + L"decklink"; }
};

safe_ptr<frame_producer> create_decklink_producer(const std::vector<std::wstring>& params)
{
	if(params.empty() || !boost::iequals(params[0], "decklink"))
		return frame_producer::empty();

	size_t device_index = 1;
	if(params.size() > 1)
	{
		try{device_index = boost::lexical_cast<int>(params[1]);}
		catch(boost::bad_lexical_cast&){}
	}

	video_format_desc format_desc = video_format_desc::get(L"PAL");
	if(params.size() > 2)
	{
		format_desc = video_format_desc::get(params[2]);
		format_desc = format_desc.format != video_format::invalid ? format_desc : video_format_desc::get(L"PAL");
	}

	return make_safe<decklink_producer>(format_desc, device_index);
}

}}