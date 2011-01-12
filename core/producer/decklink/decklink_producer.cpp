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

#include "../../processor/draw_frame.h"
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

class input_callback : public IDeckLinkInputCallback
{
	tbb::atomic<ULONG> ref_count_;
	size_t width_;
	size_t height_;
	std::shared_ptr<frame_processor_device> frame_processor_;

	tbb::concurrent_bounded_queue<safe_ptr<draw_frame>> frame_buffer_;
	safe_ptr<draw_frame> head_;
	safe_ptr<draw_frame> tail_;
public:

	input_callback(size_t width, size_t height, const std::shared_ptr<frame_processor_device>& frame_processor)
		: width_(width)
		, height_(height)
		, frame_processor_(frame_processor)
		, head_(draw_frame::empty())
		, tail_(draw_frame::empty())
	{
		ref_count_ = 1;
		frame_buffer_.set_capacity(8);
	}
	
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID *ppv)
	{
		HRESULT	result = E_NOINTERFACE;	
		*ppv = NULL;
	
		if (iid == IID_IUnknown)
		{
			*ppv = this;
			AddRef();
			result = S_OK;
		}
		else if (iid == IID_IDeckLinkInputCallback)
		{
			*ppv = (IDeckLinkInputCallback*)this;
			AddRef();
			result = S_OK;
		}
	
		return result;
	}

	virtual ULONG STDMETHODCALLTYPE	AddRef(void)
	{
		return ++ref_count_;
	}

	virtual ULONG STDMETHODCALLTYPE	Release(void)
	{	
		auto new_ref_count = --ref_count_;
		if (new_ref_count == 0)
		{
			delete this;
			return 0;
		}
	
		return new_ref_count;
	}
	
	virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents /*notificationEvents*/, IDeckLinkDisplayMode* /*newDisplayMode*/, BMDDetectedVideoInputFormatFlags /*detectedSignalFlags*/)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame* video, IDeckLinkAudioInputPacket* /*audio*/)
	{		
		if(!frame_buffer_.try_push(head_))
		{
			CASPAR_LOG(trace) << "decklink overflow";
			return S_OK;
		}

		// TODO: Convert from YUV422 non-planar to either a permutation of BGRA or YUV422 planar format.
		// Decklink 7.6 has IDeckLinkVideoConversion which can convert to ARGB, should probably upgrade to latest SDK version.

		//pixel_format_desc desc;
		//desc.pix_fmt = pixel_format::argb;
		//desc.planes.push_back(pixel_format_desc::plane(width_, height_, 4));	

		//auto frame = frame_processor_->create_frame(desc);
		//		
		//void* bytes;
		//if(FAILED(video->GetBytes(&bytes)))
		//	return S_OK;

		//std::copy_n(reinterpret_cast<unsigned char*>(bytes), width_*height_, frame->image_data().begin());
		head_ = draw_frame::empty();

		return S_OK;
	}

	safe_ptr<draw_frame> get_frame()
	{
		if(!frame_buffer_.try_pop(tail_))
			CASPAR_LOG(trace) << "decklink underflow";
		return tail_;
	}
};
	
// NOTE: This class is not-fully implemented. See input_callback::VideoInputFrameArrived.
// With this design it will probably not be possible to do input and output from same decklink device.
// decklink_consumer and decklink_producer will need to use a common active object decklink_device which handles all processing for decklink device.
class decklink_producer : public frame_producer
{
	executor executor_;

	CComPtr<IDeckLink>			decklink_;
	CComQIPtr<IDeckLinkInput>	input_;
	
	video_format_desc format_desc_;

	const size_t device_index_;

	std::unique_ptr<input_callback> callback_;
	
public:

	explicit decklink_producer(const video_format_desc format_desc, size_t device_index)
		: device_index_(device_index)
		, format_desc_(format_desc){}

	~decklink_producer()
	{	
		executor_.invoke([this]
		{
			if(input_ != nullptr) 
			{
				input_->StopStreams();
				input_->DisableVideoInput();
			}

			input_.Release();
			decklink_.Release();
			CoUninitialize();
		});
	}
	
	virtual safe_ptr<draw_frame> receive()
	{
		return callback_->get_frame();
	}

	virtual void initialize(const safe_ptr<frame_processor_device>& frame_processor)
	{
		executor_.start();
		executor_.invoke([=]
		{
			if(FAILED(CoInitialize(nullptr))) 
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("decklink_producer: Initialization of COM failed."));		

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
		
			unsigned long decklinkVideoFormat = decklink::GetDecklinkVideoFormat(format_desc_.format);
			if(decklinkVideoFormat == ULONG_MAX) 
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("decklink_producer: Card does not support requested videoformat."));

			BMDDisplayModeSupport displayModeSupport;
			if(FAILED(input_->DoesSupportVideoMode((BMDDisplayMode)decklinkVideoFormat, bmdFormat8BitYUV, &displayModeSupport)))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("decklink_producer: Card does not support requested videoformat."));

			// NOTE: bmdFormat8BitABGR does not seem to work with Decklink HD Extreme 3D
			if(FAILED(input_->EnableVideoInput((BMDDisplayMode)decklinkVideoFormat, bmdFormat8BitYUV, 0))) 
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Could not enable video input."));
			
			callback_.reset(new input_callback(format_desc_.width, format_desc_.height, frame_processor));
			if (FAILED(input_->SetCallback(callback_.get())) != S_OK)
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("decklink_producer: Failed to set input callback."));
			
			if(FAILED(input_->StartStreams()))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Failed to start input stream."));

			CASPAR_LOG(info) << "decklink_producer: Successfully initialized decklink for " << format_desc_.name;
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