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

#include <core/video_format.h>

#include <mixer/frame/read_frame.h>

#include <common/concurrency/executor.h>
#include <common/diagnostics/graph.h>
#include <common/exception/exceptions.h>
#include <common/utility/timer.h>

#include <tbb/concurrent_queue.h>

#include <boost/circular_buffer.hpp>

#pragma warning(push)
#pragma warning(disable : 4996)

	#include <atlbase.h>

	#include <atlcom.h>
	#include <atlhost.h>

#pragma warning(push)

namespace caspar { namespace core {
	
struct decklink_output : public IDeckLinkVideoOutputCallback, public IDeckLinkAudioOutputCallback, boost::noncopyable
{		
	struct co_init
	{
		co_init(){CoInitialize(nullptr);}
		~co_init(){CoUninitialize();}
	} co_;
	
	const printer	parent_printer_;
	std::wstring	model_name_;
	const size_t	device_index_;

	std::shared_ptr<diagnostics::graph> graph_;
	timer perf_timer_;

	std::array<std::pair<void*, CComPtr<IDeckLinkMutableVideoFrame>>, 3> reserved_frames_;
	boost::circular_buffer<std::vector<short>> audio_container_;
	
	const bool		embed_audio_;
	const bool		internal_key;

	CComPtr<IDeckLink>			decklink_;
	CComQIPtr<IDeckLinkOutput>	output_;
	CComQIPtr<IDeckLinkKeyer>	keyer_;
	
	video_format_desc format_desc_;

	BMDTimeScale frame_time_scale_;
	BMDTimeValue frame_duration_;
	unsigned long frames_scheduled_;
	unsigned long audio_scheduled_;
	
	tbb::concurrent_bounded_queue<safe_ptr<const read_frame>> video_frame_buffer_;
	tbb::concurrent_bounded_queue<safe_ptr<const read_frame>> audio_frame_buffer_;

public:
	decklink_output(const printer& parent_printer, size_t device_index, bool embed_audio, bool internalKey) 
		: parent_printer_(parent_printer)
		, model_name_(L"DECKLINK")
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
		CASPAR_LOG(info) << print() << L" Shutting down.";	
	}

	void initialize(const video_format_desc& format_desc)
	{
		format_desc_ = format_desc;
		CComPtr<IDeckLinkIterator> pDecklinkIterator;
		if(FAILED(pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " No Decklink drivers installed."));
		
		size_t n = 0;
		while(n < device_index_ && pDecklinkIterator->Next(&decklink_) == S_OK){++n;}	

		if(n != device_index_ || !decklink_)
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Decklink card not found.") << arg_name_info("device_index") << arg_value_info(boost::lexical_cast<std::string>(device_index_)));

		output_ = decklink_;
		keyer_ = decklink_;

		BSTR pModelName;
		decklink_->GetModelName(&pModelName);
		model_name_ = std::wstring(pModelName);
				
		graph_ = diagnostics::create_graph(narrow(print()));
		graph_->guide("tick-time", 0.5);
		graph_->set_color("tick-time", diagnostics::color(0.1f, 0.7f, 0.8f));
		
		auto display_mode = get_display_mode(output_.p, format_desc_.format);
		if(display_mode == nullptr) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Card does not support requested videoformat."));
		
		display_mode->GetFrameRate(&frame_duration_, &frame_time_scale_);

		BMDDisplayModeSupport displayModeSupport;
		if(FAILED(output_->DoesSupportVideoMode(display_mode->GetDisplayMode(), bmdFormat8BitBGRA, &displayModeSupport)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Card does not support requested videoformat."));
		
		if(embed_audio_)
		{
			if(FAILED(output_->EnableAudioOutput(bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger, 2, bmdAudioOutputStreamTimestamped)))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not enable audio output."));
				
			if(FAILED(output_->SetAudioCallback(this)))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not set audio callback."));
		}

		if(FAILED(output_->EnableVideoOutput(display_mode->GetDisplayMode(), bmdVideoOutputFlagDefault))) 
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Could not enable video output."));

		if(FAILED(output_->SetScheduledFrameCompletionCallback(this)))
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to set playback completion callback."));
			
		if(internal_key) 
		{
			if(FAILED(keyer_->Enable(FALSE)))			
				CASPAR_LOG(error) << print() << L" Failed to enable internal keyer.";			
			else if(FAILED(keyer_->SetLevel(255)))			
				CASPAR_LOG(error) << print() << L" Failed to set key-level to max.";
			else
				CASPAR_LOG(info) << print() << L" Successfully configured internal keyer.";		
		}
		else
		{
			if(FAILED(keyer_->Enable(TRUE)))			
				CASPAR_LOG(error) << print() << L" Failed to enable external keyer.";	
			else
				CASPAR_LOG(info) << print() << L" Successfully configured external keyer.";			
		}
		
		for(size_t n = 0; n < reserved_frames_.size(); ++n)
		{
			if(FAILED(output_->CreateVideoFrame(format_desc_.width, format_desc_.height, format_desc_.size/format_desc_.height, bmdFormat8BitBGRA, bmdFrameFlagDefault, &reserved_frames_[n].second)))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to create frame."));

			if(FAILED(reserved_frames_[n].second->GetBytes(&reserved_frames_[n].first)))
				BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to get frame bytes."));
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
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(narrow(print()) + " Failed to schedule playback."));
		
		CASPAR_LOG(info) << print() << L" Successfully initialized for " << format_desc_.name;	
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

		audio_container_.push_back(std::vector<short>(frame_audio_data, frame_audio_data+audio_samples*2));

		if(FAILED(output_->ScheduleAudioSamples(audio_container_.back().data(), audio_samples, (audio_scheduled_++) * audio_samples, 48000, nullptr)))
			CASPAR_LOG(error) << print() << L" Failed to schedule audio.";
	}
			
	void schedule_next_video(const safe_ptr<const read_frame>& frame)
	{
		if(!frame->image_data().empty())
			std::copy(frame->image_data().begin(), frame->image_data().end(), static_cast<char*>(reserved_frames_.front().first));
		else
			std::fill_n(static_cast<int*>(reserved_frames_.front().first), 0, format_desc_.size/4);

		if(FAILED(output_->ScheduleVideoFrame(reserved_frames_.front().second, (frames_scheduled_++) * frame_duration_, frame_duration_, frame_time_scale_)))
			CASPAR_LOG(error) << print() << L" Failed to schedule video.";

		std::rotate(reserved_frames_.begin(), reserved_frames_.begin() + 1, reserved_frames_.end());
		graph_->update("tick-time", static_cast<float>(perf_timer_.elapsed()/format_desc_.interval*0.5));
		perf_timer_.reset();
	}

	void send(const safe_ptr<const read_frame>& frame)
	{
		video_frame_buffer_.push(frame);
		if(embed_audio_)
			audio_frame_buffer_.push(frame);
	}

	std::wstring print() const
	{
		return (parent_printer_ ? parent_printer_() + L"/" : L"") + model_name_ + L" [" + boost::lexical_cast<std::wstring>(device_index_) + L"]";
	}
};

struct decklink_consumer::implementation
{
	printer parent_printer_;
	std::unique_ptr<decklink_output> input_;

	executor executor_;
public:

	implementation(size_t device_index, bool embed_audio, bool internalKey)
		: executor_(L"DECKLINK[" + boost::lexical_cast<std::wstring>(device_index) + L"]")
	{
		executor_.start();
		executor_.invoke([&]
		{
			input_.reset(new decklink_output(parent_printer_, device_index, embed_audio, internalKey));
		});
	}

	~implementation()
	{
		executor_.invoke([&]
		{
			input_ = nullptr;
		});
	}

	void initialize(const video_format_desc& format_desc)
	{
		executor_.invoke([&]
		{
			input_->initialize(format_desc);
		});
	}

	void set_parent_printer(const printer& parent_printer)
	{
		parent_printer_ = parent_printer;
	}

	void send(const safe_ptr<const read_frame>& frame)
	{
		input_->send(frame);
	}

	size_t buffer_depth() const
	{
		return 1;
	}

	std::wstring print() const
	{
		return input_->print();
	}
};

decklink_consumer::decklink_consumer(size_t device_index, bool embed_audio, bool internalKey) : impl_(new implementation(device_index, embed_audio, internalKey)){}
decklink_consumer::decklink_consumer(decklink_consumer&& other) : impl_(std::move(other.impl_)){}
void decklink_consumer::initialize(const video_format_desc& format_desc){impl_->initialize(format_desc);}
void decklink_consumer::set_parent_printer(const printer& parent_printer){impl_->set_parent_printer(parent_printer);}
void decklink_consumer::send(const safe_ptr<const read_frame>& frame){impl_->send(frame);}
size_t decklink_consumer::buffer_depth() const{return impl_->buffer_depth();}
std::wstring decklink_consumer::print() const{return impl_->print();}

std::wstring get_decklink_version() 
{
	CComPtr<IDeckLinkIterator> pDecklinkIterator;
	if(FAILED(pDecklinkIterator.CoCreateInstance(CLSID_CDeckLinkIterator)))
		return L"Unknown";
		
	return get_version(pDecklinkIterator);
}
	
safe_ptr<frame_consumer> create_decklink_consumer(const std::vector<std::wstring>& params)
{
	if(params.size() < 1 || params[0] != L"DECKLINK")
		return frame_consumer::empty();
	
	int device_index = 1;
	bool embed_audio = false;
	bool internal_key = false;

	try{if(params.size() > 1) device_index = boost::lexical_cast<int>(params[2]);}
	catch(boost::bad_lexical_cast&){}
	try{if(params.size() > 2) embed_audio = boost::lexical_cast<bool>(params[3]);}
	catch(boost::bad_lexical_cast&){}
	try{if(params.size() > 3) internal_key = boost::lexical_cast<bool>(params[4]);}
	catch(boost::bad_lexical_cast&){}

	return make_safe<decklink_consumer>(device_index, embed_audio, internal_key);
}

}}