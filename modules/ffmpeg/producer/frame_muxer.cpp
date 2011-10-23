#include "../StdAfx.h"

#include "frame_muxer.h"

#include "filter/filter.h"

#include "util.h"
#include "display_mode.h"

#include <core/producer/frame_producer.h>
#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>
#include <core/mixer/write_frame.h>

#include <common/env.h>
#include <common/exception/exceptions.h>
#include <common/log/log.h>

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

#include <agents.h>

using namespace Concurrency;

namespace caspar { namespace ffmpeg {
	
struct frame_muxer2::implementation : public Concurrency::agent, boost::noncopyable
{		
	ITarget<safe_ptr<core::basic_frame>>&			target_;
	mutable single_assignment<display_mode::type>	display_mode_;
	const double									in_fps_;
	const core::video_format_desc					format_desc_;
	const bool										auto_transcode_;
	
	mutable single_assignment<safe_ptr<filter>>		filter_;
	const safe_ptr<core::frame_factory>				frame_factory_;
	
	call<std::shared_ptr<AVFrame>>					push_video_;
	call<std::shared_ptr<core::audio_buffer>>		push_audio_;
	
	unbounded_buffer<safe_ptr<AVFrame>>				video_;
	unbounded_buffer<safe_ptr<core::audio_buffer>>	audio_;
	
	core::audio_buffer								audio_data_;

	overwrite_buffer<bool>							is_running_;

	std::wstring									filter_str_;
	
	implementation(frame_muxer2::video_source_t* video_source,
				   frame_muxer2::audio_source_t* audio_source,
				   frame_muxer2::target_t& target,
				   double in_fps, 
				   const safe_ptr<core::frame_factory>& frame_factory,
				   const std::wstring& filter)
		: target_(target)
		, in_fps_(in_fps)
		, format_desc_(frame_factory->get_video_format_desc())
		, auto_transcode_(env::properties().get("configuration.producers.auto-transcode", false))
		, frame_factory_(frame_factory)
		, push_video_(std::bind(&implementation::push_video, this, std::placeholders::_1))
		, push_audio_(std::bind(&implementation::push_audio, this, std::placeholders::_1))
	{
		if(video_source)
			video_source->link_target(&push_video_);
		if(audio_source)
			audio_source->link_target(&push_audio_);
		
		start();
	}

	~implementation()
	{
		send(is_running_, false);
		agent::wait(this);
		CASPAR_LOG(trace) << "[frame_muxer] Stopped.";
	}

	std::shared_ptr<core::write_frame> receive_video()
	{	
		auto video = receive(video_);

		if(!is_running_.value() || video == eof_video())
		{	
			send(is_running_ , false);
			return nullptr;
		}

		CASPAR_ASSERT(video != loop_video());

		try
		{
			if(video == empty_video())
				return make_safe<core::write_frame>(this);

			return make_write_frame(this, video, frame_factory_, 0);
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			send(is_running_, false);
			return nullptr;
		}
	}

	std::shared_ptr<core::audio_buffer> receive_audio()
	{		
		auto audio = receive(audio_);

		if(!is_running_.value() || audio == eof_audio())
		{
			send(is_running_ , false);
			return nullptr;
		}

		CASPAR_ASSERT(audio != loop_audio());

		try
		{
			if(audio == empty_audio())
				return make_safe<core::audio_buffer>(format_desc_.audio_samples_per_frame, 0);

			return audio;
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			send(is_running_, false);
			return nullptr;
		}
	}
	
	virtual void run()
	{
		try
		{
			send(is_running_, true);
			while(is_running_.value())
			{
				auto audio = receive_audio();	
				if(!audio)
					break;
											
				auto video = receive_video();
				if(!video)
					break;

				video->audio_data() = std::move(*audio);

				switch(display_mode_.value())
				{
				case display_mode::simple:			
				case display_mode::deinterlace:
				case display_mode::deinterlace_bob:
					{
						send(target_, safe_ptr<core::basic_frame>(std::move(video)));

						break;
					}
				case display_mode::duplicate:					
					{										
						send(target_, safe_ptr<core::basic_frame>(video));

						auto audio2 = receive_audio();
						if(audio2)
						{
							auto video2 = make_safe<core::write_frame>(*video);
							video2->audio_data() = std::move(*audio2);
							send(target_, safe_ptr<core::basic_frame>(video2));
						}

						break;
					}
				case display_mode::half:						
					{								
						send(target_, safe_ptr<core::basic_frame>(std::move(video)));
						receive_video();
						break;
					}
				case display_mode::deinterlace_bob_reinterlace:
				case display_mode::interlace:					
					{					
						auto frame = safe_ptr<core::basic_frame>(std::move(video));
						auto video2 = receive_video();
						auto frame2 = core::basic_frame::empty();

						if(video2)						
							frame2 = safe_ptr<core::basic_frame>(video2);								
						
						frame = core::basic_frame::interlace(std::move(frame), std::move(frame2), format_desc_.field_mode);	
						send(target_, frame);

						break;
					}
				default:	
					BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("invalid display-mode"));
				}
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
		
		send(is_running_ , false);
		send(target_, core::basic_frame::eof());

		done();
	}
			
	void push_video(const std::shared_ptr<AVFrame>& video_frame)
	{		
		if(!video_frame)
			return;

		if(video_frame == eof_video() || video_frame == empty_video())
		{
			send(video_, make_safe_ptr(video_frame));
			return;
		}
				
		if(video_frame == loop_video())		
			return;	
		
		try
		{
			if(!is_running_.value())
				return;

			if(!display_mode_.has_value())
				initialize_display_mode(*video_frame);
						
			//send(video_, make_safe_ptr(video_frame));

			//if(hints & core::frame_producer::ALPHA_HINT)
			//	video_frame->format = make_alpha_format(video_frame->format);
		
			auto format = video_frame->format;
			if(video_frame->format == CASPAR_PIX_FMT_LUMA) // CASPAR_PIX_FMT_LUMA is not valid for filter, change it to GRAY8
				video_frame->format = PIX_FMT_GRAY8;

			filter_.value()->push(video_frame);

			while(true)
			{
				auto frame = filter_.value()->poll();
				if(!frame)
					break;	

				frame->format = format;
				send(video_, make_safe_ptr(frame));
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			send(is_running_ , false);
			send(video_, eof_video());
		}
	}

	void push_audio(const std::shared_ptr<core::audio_buffer>& audio_samples)
	{
		if(!audio_samples)
			return;

		if(audio_samples == eof_audio() || audio_samples == empty_audio())
		{
			send(audio_, make_safe_ptr(audio_samples));
			return;
		}

		if(audio_samples == loop_audio())			
			return;		

		try
		{		
			if(!is_running_.value())
				return;

			audio_data_.insert(audio_data_.end(), audio_samples->begin(), audio_samples->end());
		
			while(audio_data_.size() >= format_desc_.audio_samples_per_frame)
			{
				auto begin = audio_data_.begin(); 
				auto end   = begin + format_desc_.audio_samples_per_frame;
					
				send(audio_, make_safe<core::audio_buffer>(begin, end));

				audio_data_.erase(begin, end);
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			send(is_running_ , false);
			send(audio_, eof_audio());
		}
	}

	void initialize_display_mode(AVFrame& frame)
	{
		auto display_mode = display_mode::invalid;

		if(auto_transcode_)
		{
			auto mode = get_mode(frame);
			auto fps  = in_fps_;

			if(is_deinterlacing(filter_str_))
				mode = core::field_mode::progressive;

			if(is_double_rate(filter_str_))
				fps *= 2;

			display_mode = get_display_mode(mode, fps, format_desc_.field_mode, format_desc_.fps);
			
			if(display_mode == display_mode::simple && mode != core::field_mode::progressive && format_desc_.field_mode != core::field_mode::progressive && frame.height != static_cast<int>(format_desc_.height))
				display_mode = display_mode::deinterlace_bob_reinterlace; // The frame will most likely be scaled, we need to deinterlace->reinterlace	
				
			if(display_mode == display_mode::deinterlace)
				append_filter(filter_str_, L"YADIF=0:-1");
			else if(display_mode == display_mode::deinterlace_bob || display_mode == display_mode::deinterlace_bob_reinterlace)
				append_filter(filter_str_, L"YADIF=1:-1");
		}
		else
			display_mode = display_mode::simple;

		if(display_mode == display_mode::invalid)
		{
			CASPAR_LOG(warning) << L"[frame_muxer] Failed to detect display-mode.";
			display_mode = display_mode::simple;
		}
			
		send(filter_, make_safe<filter>(filter_str_));

		CASPAR_LOG(info) << "[frame_muxer] " << display_mode::print(display_mode);

		send(display_mode_, display_mode);
	}
					
	int64_t calc_nb_frames(int64_t nb_frames) const
	{
		switch(display_mode_.value()) // Take into account transformation in run.
		{
		case display_mode::deinterlace_bob_reinterlace:
		case display_mode::interlace:	
		case display_mode::half:
			nb_frames /= 2;
			break;
		case display_mode::duplicate:
			nb_frames *= 2;
			break;
		}

		if(is_double_rate(widen(filter_.value()->filter_str()))) // Take into account transformations in filter.
			nb_frames *= 2;

		return nb_frames;
	}
};

frame_muxer2::frame_muxer2(video_source_t* video_source, 
						   audio_source_t* audio_source,
						   target_t& target,
						   double in_fps, 
						   const safe_ptr<core::frame_factory>& frame_factory,
						   const std::wstring& filter)
	: impl_(new implementation(video_source, audio_source, target, in_fps, frame_factory, filter))
{
}

int64_t frame_muxer2::calc_nb_frames(int64_t nb_frames) const
{
	return impl_->calc_nb_frames(nb_frames);
}

}}