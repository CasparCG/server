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

#include "decklink_consumer.h"

#include "util.h"

#include "DeckLinkAPI_h.h"

#include "../../format/video_format.h"
#include "../../processor/read_frame.h"

#include <common/concurrency/executor.h>
#include <common/exception/exceptions.h>

#include <tbb/concurrent_queue.h>
#include <boost/thread.hpp>

#pragma warning(push)
#pragma warning(disable : 4996)

	#include <atlbase.h>

	#include <atlcom.h>
	#include <atlhost.h>

#pragma warning(push)

namespace caspar { namespace core { namespace decklink{

struct decklink_consumer::implementation : boost::noncopyable
{			
	boost::unique_future<void> active_;
	executor executor_;

	std::array<std::pair<void*, CComPtr<IDeckLinkMutableVideoFrame>>, 3> reserved_frames_;

	bool						internal_key;
	CComPtr<IDeckLink>			decklink_;
	CComQIPtr<IDeckLinkOutput>	output_;
	CComQIPtr<IDeckLinkKeyer>	keyer_;
	
	video_format::type current_format_;
	video_format_desc format_desc_;

public:
	implementation(const video_format_desc& format_desc, size_t device_index, bool internalKey) 
		: format_desc_(format_desc)
		, current_format_(video_format::pal)
		, internal_key(internalKey)
	{	
		executor_.start();
		executor_.invoke([=]
		{
			if(FAILED(CoInitialize(nullptr))) 
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Initialization of COM failed."));		

			CComPtr<IDeckLinkIterator> pDecklinkIterator;
			if(FAILED(pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator)))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: No Decklink drivers installed."));

			size_t n = 0;
			while(n < device_index && pDecklinkIterator->Next(&decklink_) == S_OK){++n;}	

			if(n != device_index || !decklink_)
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: No Decklink card found.") << arg_name_info("device_index") << arg_value_info(boost::lexical_cast<std::string>(device_index)));

			output_ = decklink_;
			keyer_ = decklink_;

			BSTR pModelName;
			decklink_->GetModelName(&pModelName);
			if(pModelName != nullptr)
				CASPAR_LOG(info) << "DECKLINK: Modelname: " << pModelName;
		
			unsigned long decklinkVideoFormat = GetDecklinkVideoFormat(format_desc_.format);
			if(decklinkVideoFormat == ULONG_MAX) 
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Card does not support requested videoformat."));
		
			current_format_ = format_desc_.format;

			BMDDisplayModeSupport displayModeSupport;
			if(FAILED(output_->DoesSupportVideoMode((BMDDisplayMode)decklinkVideoFormat, bmdFormat8BitBGRA, &displayModeSupport)))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Card does not support requested videoformat."));
		
			output_->DisableAudioOutput();
			if(FAILED(output_->EnableVideoOutput((BMDDisplayMode)decklinkVideoFormat, bmdVideoOutputFlagDefault))) 
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Could not enable video output."));

			if(internal_key) 
			{
				if(FAILED(keyer_->Enable(FALSE)))			
					CASPAR_LOG(error) << "DECKLINK: Failed to enable internal keye.r";			
				else if(FAILED(keyer_->SetLevel(255)))			
					CASPAR_LOG(error) << "DECKLINK: Keyer - Failed to set key-level to max.";
				else
					CASPAR_LOG(info) << "DECKLINK: Successfully configured internal keyer.";		
			}
			else
			{
				if(FAILED(keyer_->Enable(TRUE)))			
					CASPAR_LOG(error) << "DECKLINK: Failed to enable external keyer.";	
				else
					CASPAR_LOG(info) << "DECKLINK: Successfully configured external keyer.";			
			}
		
			for(size_t n = 0; n < reserved_frames_.size(); ++n)
			{
				if(FAILED(output_->CreateVideoFrame(format_desc_.width, format_desc_.height, format_desc_.size/format_desc_.height, bmdFormat8BitBGRA, bmdFrameFlagDefault, &reserved_frames_[n].second)))
					BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Failed to create frame."));

				if(FAILED(reserved_frames_[n].second->GetBytes(&reserved_frames_[n].first)))
					BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Failed to get frame bytes."));
			}

			CASPAR_LOG(info) << "DECKLINK: Successfully initialized decklink for " << format_desc_.name;
		});
		active_ = executor_.begin_invoke([]{});
	}

	~implementation()
	{				
		executor_.invoke([this]
		{
			if(output_ != nullptr) 
				output_->DisableVideoOutput();

			for(size_t n = 0; n < reserved_frames_.size(); ++n)
				reserved_frames_[n].second.Release();

			keyer_.Release();
			output_.Release();
			decklink_.Release();
			CoUninitialize();
		});
	}
	
	void send(const safe_ptr<const read_frame>& frame)
	{
		active_.get();
		active_ = executor_.begin_invoke([=]
		{		
			std::copy(frame->image_data().begin(), frame->image_data().end(), static_cast<char*>(reserved_frames_.front().first));
				
			if(FAILED(output_->DisplayVideoFrameSync(reserved_frames_.front().second)))
				CASPAR_LOG(error) << L"DECKLINK: Failed to display frame.";

			std::rotate(reserved_frames_.begin(), reserved_frames_.begin() + 1, reserved_frames_.end());
		});
	}

	size_t buffer_depth() const
	{
		return 1;
	}
};

decklink_consumer::decklink_consumer(const video_format_desc& format_desc, size_t device_index, bool internalKey) : impl_(new implementation(format_desc, device_index, internalKey)){}
decklink_consumer::decklink_consumer(decklink_consumer&& other) : impl_(std::move(other.impl_)){}
void decklink_consumer::send(const safe_ptr<const read_frame>& frame){impl_->send(frame);}
size_t decklink_consumer::buffer_depth() const{return impl_->buffer_depth();}
	
}}}	