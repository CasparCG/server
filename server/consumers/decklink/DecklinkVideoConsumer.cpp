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
 
#include "..\..\stdafx.h"

#include "DecklinkVideoConsumer.h"
#include "util.h"

#include "..\..\Application.h"

#include "..\..\frame\FramePlaybackControl.h"
#include "..\..\frame\Frame.h"
#include "..\..\frame\FramePlaybackStrategy.h"

#include "DeckLinkAPI_h.h"

#include <boost/thread.hpp>
#include <boost/circular_buffer.hpp>

#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>

#include <queue>
#include <array>

namespace caspar {namespace decklink {
	
struct decklink_output: public IDeckLinkVideoOutputCallback, public IDeckLinkAudioOutputCallback, boost::noncopyable
{		
	struct co_init
	{
		co_init(){CoInitialize(nullptr);}
		~co_init(){CoUninitialize();}
	} co_;
	
	std::wstring	model_name_;
	const size_t	device_index_;
	
	std::array<std::pair<void*, CComPtr<IDeckLinkMutableVideoFrame>>, 3> reserved_frames_;
	boost::circular_buffer<std::vector<short>> audio_container_;
	
	const bool		embed_audio_;
	const bool		internal_key;

	CComPtr<IDeckLink>			decklink_;
	CComQIPtr<IDeckLinkOutput>	output_;
	CComQIPtr<IDeckLinkKeyer>	keyer_;
	
	FrameFormatDescription format_desc_;

	BMDTimeScale frame_time_scale_;
	BMDTimeValue frame_duration_;
	unsigned long frames_scheduled_;
	unsigned long audio_scheduled_;
	
	tbb::concurrent_bounded_queue<FramePtr> video_frame_buffer_;
	tbb::concurrent_bounded_queue<std::vector<short>> audio_frame_buffer_;

public:
	decklink_output(size_t device_index, bool embed_audio, bool internalKey) 
		: model_name_(L"DECKLINK")
		, device_index_(device_index)
		, audio_container_(5)
		, embed_audio_(embed_audio)
		, internal_key(internalKey)
		, frames_scheduled_(0)
		, audio_scheduled_(0)
	{}

	~decklink_output()
	{			
		if(output_ != nullptr) 
		{
			output_->StopScheduledPlayback(0, nullptr, 0);
			if(embed_audio_)
				output_->DisableAudioOutput();
			output_->DisableVideoOutput();
		}
		LOG << print() << L" Shutting down.";	
	}

	void initialize(const FrameFormatDescription& format_desc)
	{
		format_desc_ = format_desc;
		CComPtr<IDeckLinkIterator> pDecklinkIterator;
		if(FAILED(pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator)))
			throw std::exception("DECKLINK: No Decklink drivers installed."); //(caspar_exception() << msg_info(narrow(print()) + " No Decklink drivers installed."));
		
		static bool print_ver = false;
		if(!print_ver)
		{
			LOG << "DeckLinkAPI Version: " << get_version(pDecklinkIterator);
			print_ver = true;
		}

		size_t n = 0;
		while(n < device_index_ && pDecklinkIterator->Next(&decklink_) == S_OK){++n;}	

		if(n != device_index_ || !decklink_)
			throw std::runtime_error("DECKLINK: No Decklink card found.");//(caspar_exception() << msg_info(narrow(print()) + " No Decklink card found.") << arg_name_info("device_index") << arg_value_info(boost::lexical_cast<std::string>(device_index_)));

		output_ = decklink_;
		keyer_ = decklink_;

		BSTR pModelName;
		decklink_->GetModelName(&pModelName);
		model_name_ = std::wstring(pModelName);
						
		auto display_mode = get_display_mode(output_.p, GetVideoFormat(format_desc_.name));
		if(display_mode == nullptr) 
			throw std::runtime_error("DECKLINK: Card does not support requested videoformat.");//N(caspar_exception() << msg_info(narrow(print()) + " Card does not support requested videoformat."));
		
		display_mode->GetFrameRate(&frame_duration_, &frame_time_scale_);

		BMDDisplayModeSupport displayModeSupport;
		if(FAILED(output_->DoesSupportVideoMode(display_mode->GetDisplayMode(), bmdFormat8BitBGRA, &displayModeSupport)))
			throw std::runtime_error("DECKLINK: Card does not support requested videoformat.");//(caspar_exception() << msg_info(narrow(print()) + " Card does not support requested videoformat."));
		
		if(embed_audio_)
		{
			if(FAILED(output_->EnableAudioOutput(bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger, 2, bmdAudioOutputStreamTimestamped)))
				throw std::runtime_error("DECKLINK: Could not enable audio output.");//(caspar_exception() << msg_info(narrow(print()) + " Could not enable audio output."));
				
			if(FAILED(output_->SetAudioCallback(this)))
				throw std::runtime_error("DECKLINK: Could not set audio callback.");//(caspar_exception() << msg_info(narrow(print()) + " Could not set audio callback."));
		}

		if(FAILED(output_->EnableVideoOutput(display_mode->GetDisplayMode(), bmdVideoOutputFlagDefault))) 
			throw std::exception("DECKLINK: Could not enable video output.");//(caspar_exception() << msg_info(narrow(print()) + " Could not enable video output."));

		if(FAILED(output_->SetScheduledFrameCompletionCallback(this)))
			throw std::exception("DECKLINK: Failed to set playback completion callback.");//(caspar_exception() << msg_info(narrow(print()) + " Failed to set playback completion callback."));
			
		if(internal_key) 
		{
			if(FAILED(keyer_->Enable(FALSE)))			
				LOG << print() << L" Failed to enable internal keyer.";			
			else if(FAILED(keyer_->SetLevel(255)))			
				LOG << print() << L" Failed to set key-level to max.";
			else
				LOG << print() << L" Successfully configured internal keyer.";		
		}
		else
		{
			if(FAILED(keyer_->Enable(TRUE)))			
				LOG << print() << L" Failed to enable external keyer.";	
			else
				LOG << print() << L" Successfully configured external keyer.";			
		}
		
		for(size_t n = 0; n < reserved_frames_.size(); ++n)
		{
			if(FAILED(output_->CreateVideoFrame(format_desc_.width, format_desc_.height, format_desc_.size/format_desc_.height, bmdFormat8BitBGRA, bmdFrameFlagDefault, &reserved_frames_[n].second)))
				throw std::exception("DECKLINK: Failed to create frame.");//(caspar_exception() << msg_info(narrow(print()) + " Failed to create frame."));

			if(FAILED(reserved_frames_[n].second->GetBytes(&reserved_frames_[n].first)))
				throw std::exception("DECKLINK: Failed to get frame bytes.");//(caspar_exception() << msg_info(narrow(print()) + " Failed to get frame bytes."));
		}
					
		auto buffer_size = static_cast<size_t>(frame_time_scale_/frame_duration_)/4;
		for(size_t n = 0; n < buffer_size; ++n)
			schedule_next_video(FramePtr());

		video_frame_buffer_.set_capacity(buffer_size);
		audio_frame_buffer_.set_capacity(buffer_size);
		
		if(FAILED(output_->StartScheduledPlayback(0, frame_time_scale_, 1.0))) 
			throw std::exception("DECKLINK: Failed to schedule playback.");//(caspar_exception() << msg_info(narrow(print()) + " Failed to schedule playback."));
		
		LOG << print() << L" Successfully initialized for " << format_desc_.name;	
	}
	
	virtual HRESULT STDMETHODCALLTYPE	QueryInterface (REFIID, LPVOID*)	{return E_NOINTERFACE;}
	virtual ULONG STDMETHODCALLTYPE		AddRef ()							{return 1;}
	virtual ULONG STDMETHODCALLTYPE		Release ()							{return 1;}
	
	virtual HRESULT STDMETHODCALLTYPE ScheduledFrameCompleted (IDeckLinkVideoFrame* /*completedFrame*/, BMDOutputFrameCompletionResult /*result*/)
	{
		FramePtr frame;		
		video_frame_buffer_.try_pop(frame);		
		schedule_next_video(frame);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ScheduledPlaybackHasStopped (void)
	{
		return S_OK;
	}
		
	virtual HRESULT STDMETHODCALLTYPE RenderAudioSamples (BOOL /*preroll*/)
	{
		int audio_samples = 1920;
		std::vector<short> audio_data(audio_samples*2, 0);
		audio_frame_buffer_.try_pop(audio_data);
		schedule_next_audio(audio_data);
		return S_OK;
	}

	void schedule_next_audio(std::vector<short>& audio_data)
	{
		int audio_samples = 1920;
				
		if(FAILED(output_->ScheduleAudioSamples(audio_data.data(), audio_samples, (frames_scheduled_) * audio_samples, 48000, nullptr)))
			LOG << L"DECKLINK: Failed to schedule audio.";
	}

	void MixAudio(short* dest, const AudioDataChunkList& frame_audio_data, size_t audio_samples, size_t audio_nchannels)
	{		
		size_t size = audio_samples*audio_nchannels;
		memset(dest, 0, size*2);
		std::for_each(frame_audio_data.begin(), frame_audio_data.end(), [&](const audio::AudioDataChunkPtr& chunk)
		{
			short* src = reinterpret_cast<short*>(chunk->GetDataPtr());
			for(size_t n = 0; n < size; ++n)
				dest[n] = static_cast<short>(static_cast<int>(dest[n])+static_cast<int>(src[n]));
		});
	}
			
	void schedule_next_video(const FramePtr& frame)
	{
		if(frame)
			std::copy(frame->GetDataPtr(), frame->GetDataPtr()+frame->GetDataSize(), static_cast<char*>(reserved_frames_.front().first));
		else
			std::fill_n(static_cast<int*>(reserved_frames_.front().first), 0, format_desc_.size/4);

		if(FAILED(output_->ScheduleVideoFrame(reserved_frames_.front().second, (frames_scheduled_++) * frame_duration_, frame_duration_, frame_time_scale_)))
			LOG << print() << L" Failed to schedule video.";

		std::rotate(reserved_frames_.begin(), reserved_frames_.begin() + 1, reserved_frames_.end());
	}

	void send(const FramePtr& frame)
	{
		if(!frame)
			return;

		video_frame_buffer_.push(frame);
		if(embed_audio_)
		{
			int audio_samples = 1920;
			std::vector<short> frame_audio_data(audio_samples*2, 0);
			if(frame)
				MixAudio(frame_audio_data.data(), frame->GetAudioData(), audio_samples, 2);	

			audio_frame_buffer_.push(frame_audio_data);
		}
	}

	std::wstring print() const
	{
		return L"DECKLINK: " + model_name_ + L"[" + boost::lexical_cast<std::wstring>(device_index_) + L"]";
	}
};

struct DecklinkVideoConsumer::implementation : public std::enable_shared_from_this<implementation>, public IFramePlaybackStrategy
{
	decklink_output output_;
	DecklinkVideoConsumer* consumer_;
	FrameManagerPtr frame_manager_;
	FrameFormatDescription format_desc_;
	FramePlaybackControlPtr pPlaybackControl_;
public:
	explicit implementation(size_t device_index, DecklinkVideoConsumer* consumer)
		: output_(device_index, 
					GetApplication()->GetSetting(L"embedded-audio") == L"true",
					GetApplication()->GetSetting(L"internal-key") == L"true")
		, consumer_(consumer)
	{
		auto desired_fmt = caspar::GetApplication()->GetSetting(TEXT("videomode"));
		if(desired_fmt.empty())
			desired_fmt = L"PAL";
		FrameFormat caspar_fmt = caspar::GetVideoFormat(desired_fmt);
		caspar_fmt = caspar_fmt != FFormatInvalid ? caspar_fmt : FFormatPAL;
		format_desc_ = FrameFormatDescription::FormatDescriptions[caspar_fmt];
		frame_manager_ = std::make_shared<SystemFrameManager>(format_desc_);
		output_.initialize(format_desc_);
	}

	void initialize()
	{
		pPlaybackControl_.reset(new FramePlaybackControl(FramePlaybackStrategyPtr(this)));
		pPlaybackControl_->Start();
	}

	~implementation()
	{
		pPlaybackControl_.reset();
	}

	void DisplayFrame(FramePtr frame)
	{
		output_.send(frame);
	}

	IVideoConsumer* GetConsumer() { return consumer_; }
	
	FramePtr GetReservedFrame() 
	{
		return frame_manager_->CreateFrame();
	}

	FrameManagerPtr GetFrameManager()
	{
		return frame_manager_;
	}
};

DecklinkVideoConsumer::DecklinkVideoConsumer(unsigned int device_index) : impl_(new implementation(device_index, this))
{
	impl_->initialize();
}

DecklinkVideoConsumer::~DecklinkVideoConsumer()
{}

IPlaybackControl* DecklinkVideoConsumer::GetPlaybackControl() const
{
	return impl_->pPlaybackControl_.get();
}

const FrameFormatDescription& DecklinkVideoConsumer::GetFrameFormatDescription() const 
{
	return impl_->format_desc_;
}

const TCHAR* DecklinkVideoConsumer::GetFormatDescription() const 
{
	return impl_->format_desc_.name;
}

bool DecklinkVideoConsumer::SetVideoFormat(const tstring& strDesiredFrameFormat)
{
	return false;
}

int DecklinkVideoConsumer::EnumerateDevices()
{
	CComPtr<IDeckLinkIterator> pDecklinkIterator;
	if(FAILED(pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator)))
		throw std::exception(); //(caspar_exception() << msg_info(narrow(print()) + " No Decklink drivers installed."));
	
	CComPtr<IDeckLink> decklink;
	size_t n = 0;
	while(pDecklinkIterator->Next(&decklink) == S_OK){++n;}	
	return n;
}

}}