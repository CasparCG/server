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

#include "../../video_format.h"
#include "../../processor/read_frame.h"

#include <common/concurrency/executor.h>
#include <common/exception/exceptions.h>

#include <tbb/concurrent_queue.h>

#include <boost/circular_buffer.hpp>

#pragma warning(push)
#pragma warning(disable : 4996)

	#include <atlbase.h>

	#include <atlcom.h>
	#include <atlhost.h>

#pragma warning(push)

namespace caspar { namespace core { namespace decklink{
	
struct decklink_consumer::implementation : public IDeckLinkVideoOutputCallback, public IDeckLinkAudioOutputCallback, boost::noncopyable
{		
	struct co_init
	{
		co_init(){CoInitialize(nullptr);}
		~co_init(){CoUninitialize();}
	} co_;

	std::array<std::pair<void*, CComPtr<IDeckLinkMutableVideoFrame>>, 3> reserved_frames_;
	boost::circular_buffer<std::vector<short>> audio_container_;

	const bool					embed_audio_;
	const bool					internal_key;
	CComPtr<IDeckLink>			decklink_;
	CComQIPtr<IDeckLinkOutput>	output_;
	CComQIPtr<IDeckLinkKeyer>	keyer_;
	
	video_format::type current_format_;
	video_format_desc format_desc_;

	BMDTimeScale frame_time_scale_;
	BMDTimeValue frame_duration_;
	unsigned long frames_scheduled_;
	unsigned long audio_scheduled_;

	tbb::concurrent_bounded_queue<safe_ptr<const read_frame>> video_frame_buffer_;
	tbb::concurrent_bounded_queue<safe_ptr<const read_frame>> audio_frame_buffer_;

public:
	implementation(const video_format_desc& format_desc, size_t device_index, bool embed_audio, bool internalKey) 
		: format_desc_(format_desc)
		, audio_container_(5)
		, current_format_(video_format::pal)
		, embed_audio_(embed_audio)
		, internal_key(internalKey)
		, frames_scheduled_(0)
		, audio_scheduled_(0)
	{	
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
		
		auto display_mode = get_display_mode(output_.p, format_desc_.format);
		if(display_mode == nullptr) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Card does not support requested videoformat."));
		
		display_mode->GetFrameRate(&frame_duration_, &frame_time_scale_);
		current_format_ = format_desc_.format;

		BMDDisplayModeSupport displayModeSupport;
		if(FAILED(output_->DoesSupportVideoMode(display_mode->GetDisplayMode(), bmdFormat8BitBGRA, &displayModeSupport)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Card does not support requested videoformat."));
		
		if(embed_audio_)
		{
			if(FAILED(output_->EnableAudioOutput(bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger, 2, bmdAudioOutputStreamTimestamped)))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Could not enable audio output."));
				
			if(FAILED(output_->SetAudioCallback(this)))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Could set audio callback."));
		}

		if(FAILED(output_->EnableVideoOutput(display_mode->GetDisplayMode(), bmdVideoOutputFlagDefault))) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Could not enable video output."));

		if(FAILED(output_->SetScheduledFrameCompletionCallback(this)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Failed to set playback completion callback."));
			
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
					
		auto buffer_size = static_cast<size_t>(frame_time_scale_/frame_duration_)/4;
		for(size_t n = 0; n < buffer_size; ++n)
			schedule_next_video(safe_ptr<const read_frame>());

		video_frame_buffer_.set_capacity(buffer_size);
		audio_frame_buffer_.set_capacity(buffer_size);
		for(size_t n = 0; n < std::max<size_t>(2, buffer_size-2); ++n)
		{
			video_frame_buffer_.try_push(safe_ptr<const read_frame>());
			if(embed_audio_)
				audio_frame_buffer_.try_push(safe_ptr<const read_frame>());
		}

		if(FAILED(output_->StartScheduledPlayback(0, frame_time_scale_, 1.0))) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info("DECKLINK: Failed to schedule playback."));
		
		CASPAR_LOG(info) << "DECKLINK: Successfully initialized decklink for " << format_desc_.name;		
	}

	~implementation()
	{			
		if(output_ != nullptr) 
		{
			output_->StopScheduledPlayback(0, nullptr, 0);
			if(embed_audio_)
				output_->DisableAudioOutput();
			output_->DisableVideoOutput();
		}
	}
	
	virtual HRESULT STDMETHODCALLTYPE	QueryInterface (REFIID, LPVOID*)	{return E_NOINTERFACE;}
	virtual ULONG STDMETHODCALLTYPE		AddRef ()							{return 1;}
	virtual ULONG STDMETHODCALLTYPE		Release ()							{return 1;}
	
	virtual HRESULT STDMETHODCALLTYPE ScheduledFrameCompleted (IDeckLinkVideoFrame* /*completedFrame*/, BMDOutputFrameCompletionResult /*result*/)
	{
		safe_ptr<const read_frame> frame;		
		video_frame_buffer_.pop(frame);		
		schedule_next_video(frame);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ScheduledPlaybackHasStopped (void)
	{
		return S_OK;
	}
		
	virtual HRESULT STDMETHODCALLTYPE RenderAudioSamples (BOOL /*preroll*/)
	{
		safe_ptr<const read_frame> frame;		
		audio_frame_buffer_.pop(frame);		
		schedule_next_audio(frame);		
		return S_OK;
	}

	void schedule_next_audio(const safe_ptr<const read_frame>& frame)
	{
		static std::vector<short> silence(48000, 0);

		int audio_samples = static_cast<size_t>(48000.0 / format_desc_.fps);

		auto frame_audio_data = frame->audio_data().empty() ? silence.data() : const_cast<short*>(frame->audio_data().begin());

		audio_container_.push_back(std::vector<short>(frame->audio_data().begin(), frame->audio_data().end()));

		if(FAILED(output_->ScheduleAudioSamples(frame_audio_data, audio_samples, (audio_scheduled_++) * audio_samples, 48000, nullptr)))
			CASPAR_LOG(error) << L"DECKLINK: Failed to schedule audio.";
	}
			
	void schedule_next_video(const safe_ptr<const read_frame>& frame)
	{
		if(!frame->image_data().empty())
			std::copy(frame->image_data().begin(), frame->image_data().end(), static_cast<char*>(reserved_frames_.front().first));
		else
			std::fill_n(static_cast<int*>(reserved_frames_.front().first), 0, format_desc_.size/4);

		if(FAILED(output_->ScheduleVideoFrame(reserved_frames_.front().second, (frames_scheduled_++) * frame_duration_, frame_duration_, frame_time_scale_)))
			CASPAR_LOG(error) << L"DECKLINK: Failed to schedule video.";

		std::rotate(reserved_frames_.begin(), reserved_frames_.begin() + 1, reserved_frames_.end());
	}

	void send(const safe_ptr<const read_frame>& frame)
	{
		video_frame_buffer_.push(frame);
		if(embed_audio_)
			audio_frame_buffer_.push(frame);
	}

	size_t buffer_depth() const
	{
		return 1;
	}
};

decklink_consumer::decklink_consumer(const video_format_desc& format_desc, size_t device_index, bool embed_audio, bool internalKey) : impl_(new implementation(format_desc, device_index, embed_audio, internalKey)){}
decklink_consumer::decklink_consumer(decklink_consumer&& other) : impl_(std::move(other.impl_)){}
void decklink_consumer::send(const safe_ptr<const read_frame>& frame){impl_->send(frame);}
size_t decklink_consumer::buffer_depth() const{return impl_->buffer_depth();}
	
}}}	