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

#if defined(_MSC_VER)
#pragma warning (push, 1) // TODO: Legacy code, just disable warnings
#pragma warning (disable : 4244)
#endif

#include "decklink_consumer.h"
#include "util.h"

#include "DeckLinkAPI_h.h"

#include "../../format/video_format.h"
#include "../../producer/frame_producer_device.h"

#include "../../../common/exception/exceptions.h"
#include "../../../common/utility/scope_exit.h"

#include <tbb/concurrent_queue.h>
#include <boost/thread.hpp>

#pragma warning(push)
#pragma warning(disable : 4996)

	#include <atlbase.h>

	#include <atlcom.h>
	#include <atlhost.h>

#pragma warning(push)

namespace caspar { namespace core { namespace decklink{

struct decklink_consumer::Implementation
{
	Implementation(const video_format_desc& format_desc, bool internalKey) : format_desc_(format_desc), currentFormat_(video_format::pal), internalKey_(internalKey), current_index_(0)
	{			
		input_.set_capacity(1),
		thread_ = boost::thread([=]{Run();});
	}

	~Implementation()
	{
		input_.push(nullptr);
		thread_.join();

		if(output_) 
		{
			BOOL bIsRunning = FALSE;
			output_->IsScheduledPlaybackRunning(&bIsRunning);
			if(bIsRunning)
				output_->StopScheduledPlayback(0, NULL, 0);

			output_->DisableVideoOutput();
		}
	}
	
	void display(const consumer_frame& frame)
	{
		if(exception_ != nullptr)
			std::rethrow_exception(exception_);

		input_.push(std::make_shared<consumer_frame>(frame));
	}
		
	void do_display(const consumer_frame& input_frame)
	{
		try
		{
			auto& output_frame = reserved_frames_[current_index_];
			current_index_ = (++current_index_) % reserved_frames_.size();
		
			std::copy(input_frame.data().begin(), input_frame.data().end(), static_cast<char*>(output_frame.first));
				
			if(FAILED(output_->DisplayVideoFrameSync(output_frame.second)))
				CASPAR_LOG(error) << L"DECKLINK: Failed to display frame.";
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
	}

	void Run()
	{		
		if(FAILED(CoInitialize(nullptr)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("Initialization of COM failed."));		
		CASPAR_SCOPE_EXIT(CoUninitialize);
		
		init();
		
		while(true)
		{
			try
			{	
				consumer_frame_ptr frame;
				input_.pop(frame);
				
				if(!frame)
					return;

				do_display(*frame);
			}
			catch(...)
			{
				exception_ = std::current_exception();
			}
		}
	}

	void init()
	{		
		CComPtr<IDeckLinkIterator> pDecklinkIterator;
		if(FAILED(pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("No Decklink drivers installed."));

		while(pDecklinkIterator->Next(&decklink_) == S_OK && !decklink_){}	

		if(decklink_ == nullptr)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("No Decklink card found"));

		output_ = decklink_;
		keyer_ = decklink_;

		BSTR pModelName;
		decklink_->GetModelName(&pModelName);
		if(pModelName != nullptr)
			CASPAR_LOG(info) << "DECKLINK: Modelname: " << pModelName;
		
		unsigned long decklinkVideoFormat = GetDecklinkVideoFormat(format_desc_.format);
		if(decklinkVideoFormat == ULONG_MAX) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Card does not support requested videoformat."));
		
		currentFormat_ = format_desc_.format;

		BMDDisplayModeSupport displayModeSupport;
		if(FAILED(output_->DoesSupportVideoMode((BMDDisplayMode)decklinkVideoFormat, bmdFormat8BitBGRA, &displayModeSupport)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Card does not support requested videoformat"));
		
		output_->DisableAudioOutput();
		if(FAILED(output_->EnableVideoOutput((BMDDisplayMode)decklinkVideoFormat, bmdVideoOutputFlagDefault))) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Could not enable video output"));

		if(internalKey_) 
		{
			if(FAILED(keyer_->Enable(FALSE)))			
				CASPAR_LOG(error) << "DECKLINK: Failed to enable internal keyer";			
			else if(FAILED(keyer_->SetLevel(255)))			
				CASPAR_LOG(error) << "DECKLINK: Keyer - Failed to set blend-level to max";
			else
				CASPAR_LOG(info) << "DECKLINK: Successfully configured internal keyer";		
		}
		else
		{
			if(FAILED(keyer_->Enable(TRUE)))			
				CASPAR_LOG(error) << "DECKLINK: Failed to enable external keyer";	
			else
				CASPAR_LOG(info) << "DECKLINK: Successfully configured external keyer";			
		}
		
		reserved_frames_.resize(3);
		for(int n = 0; n < reserved_frames_.size(); ++n)
		{
			if(FAILED(output_->CreateVideoFrame(format_desc_.width, format_desc_.height, format_desc_.size/format_desc_.height, bmdFormat8BitBGRA, bmdFrameFlagDefault, &reserved_frames_[n].second)))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Failed to create frame."));

			if(FAILED(reserved_frames_[n].second->GetBytes(&reserved_frames_[n].first)))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Failed to get frame bytes."));
		}

		CASPAR_LOG(info) << "DECKLINK: Successfully initialized decklink for " << format_desc_.name;
	}
			
	std::vector<std::pair<void*, CComPtr<IDeckLinkMutableVideoFrame>>> reserved_frames_;
	size_t	current_index_;

	bool						internalKey_;
	CComPtr<IDeckLink>			decklink_;
	CComQIPtr<IDeckLinkOutput>	output_;
	CComQIPtr<IDeckLinkKeyer>	keyer_;
	
	video_format::type currentFormat_;
	video_format_desc format_desc_;

	std::exception_ptr	exception_;
	boost::thread		thread_;
	tbb::concurrent_bounded_queue<consumer_frame_ptr> input_;
};

decklink_consumer::decklink_consumer(const video_format_desc& format_desc, bool internalKey) : pImpl_(new Implementation(format_desc, internalKey))
{}

void decklink_consumer::display(const consumer_frame& frame)
{
	pImpl_->display(frame);
}
	
}}}	