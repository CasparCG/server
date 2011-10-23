#include "../StdAfx.h"

#include "frame_muxer.h"

#include "filter/filter.h"

#include "util.h"

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

#include <boost/foreach.hpp>

#include <agents_extras.h>

using namespace caspar::core;

using namespace Concurrency;

namespace caspar { namespace ffmpeg {

struct display_mode
{
	enum type
	{
		simple,
		duplicate,
		half,
		interlace,
		deinterlace_bob,
		deinterlace_bob_reinterlace,
		deinterlace,
		count,
		invalid
	};

	static std::wstring print(display_mode::type value)
	{
		switch(value)
		{
			case simple:						return L"simple";
			case duplicate:						return L"duplicate";
			case half:							return L"half";
			case interlace:						return L"interlace";
			case deinterlace_bob:				return L"deinterlace_bob";
			case deinterlace_bob_reinterlace:	return L"deinterlace_bob_reinterlace";
			case deinterlace:					return L"deinterlace";
			default:							return L"invalid";
		}
	}
};

display_mode::type get_display_mode(const core::field_mode::type in_mode, double in_fps, const core::field_mode::type out_mode, double out_fps)
{		
	static const auto epsilon = 2.0;

	if(in_fps < 20.0 || in_fps > 80.0)
	{
		//if(out_mode != core::field_mode::progressive && in_mode == core::field_mode::progressive)
		//	return display_mode::interlace;
		
		if(out_mode == core::field_mode::progressive && in_mode != core::field_mode::progressive)
		{
			if(in_fps < 35.0)
				return display_mode::deinterlace;
			else
				return display_mode::deinterlace_bob;
		}
	}

	if(std::abs(in_fps - out_fps) < epsilon)
	{
		if(in_mode != core::field_mode::progressive && out_mode == core::field_mode::progressive)
			return display_mode::deinterlace;
		//else if(in_mode == core::field_mode::progressive && out_mode != core::field_mode::progressive)
		//	simple(); // interlace_duplicate();
		else
			return display_mode::simple;
	}
	else if(std::abs(in_fps/2.0 - out_fps) < epsilon)
	{
		if(in_mode != core::field_mode::progressive)
			return display_mode::invalid;

		if(out_mode != core::field_mode::progressive)
			return display_mode::interlace;
		else
			return display_mode::half;
	}
	else if(std::abs(in_fps - out_fps/2.0) < epsilon)
	{
		if(out_mode != core::field_mode::progressive)
			return display_mode::invalid;

		if(in_mode != core::field_mode::progressive)
			return display_mode::deinterlace_bob;
		else
			return display_mode::duplicate;
	}

	return display_mode::invalid;
}

struct frame_muxer2::implementation : public Concurrency::agent, boost::noncopyable
{		
	ITarget<safe_ptr<core::basic_frame>>&			target_;
	display_mode::type								display_mode_;
	const double									in_fps_;
	const video_format_desc							format_desc_;
	bool											auto_transcode_;
	
	filter											filter_;
	const safe_ptr<core::frame_factory>				frame_factory_;
	
	call<safe_ptr<AVFrame>>							push_video_;
	call<safe_ptr<core::audio_buffer>>				push_audio_;
	
	transformer<safe_ptr<AVFrame>, std::shared_ptr<core::write_frame>>				video_;
	transformer<safe_ptr<core::audio_buffer>, std::shared_ptr<core::audio_buffer>>	audio_;
	
	core::audio_buffer								audio_data_;

	Concurrency::overwrite_buffer<bool>				is_running_;

	std::wstring									filter_str_;

	implementation(frame_muxer2::video_source_t* video_source,
				   frame_muxer2::audio_source_t* audio_source,
				   frame_muxer2::target_t& target,
				   double in_fps, 
				   const safe_ptr<core::frame_factory>& frame_factory,
				   const std::wstring& filter)
		: target_(target)
		, display_mode_(display_mode::invalid)
		, in_fps_(in_fps)
		, format_desc_(frame_factory->get_video_format_desc())
		, auto_transcode_(env::properties().get("configuration.producers.auto-transcode", false))
		, frame_factory_(frame_factory)
		, push_video_(std::bind(&implementation::push_video, this, std::placeholders::_1))
		, push_audio_(std::bind(&implementation::push_audio, this, std::placeholders::_1))
		, video_(std::bind(&implementation::transform_video, this, std::placeholders::_1))
		, audio_(std::bind(&implementation::transform_audio, this, std::placeholders::_1))
		, filter_str_(filter)
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
	}

	std::shared_ptr<core::write_frame> transform_video(const safe_ptr<AVFrame>& video)
	{	
		CASPAR_ASSERT(audio != loop_video());

		if(video == eof_video())
			return nullptr;

		if(video == empty_video())
			return make_safe<core::write_frame>(this);
		
		return make_write_frame(this, video, frame_factory_, 0);
	}

	std::shared_ptr<core::audio_buffer> transform_audio(const safe_ptr<core::audio_buffer>& audio)
	{			
		CASPAR_ASSERT(audio != loop_audio());

		if(audio == eof_audio())
			return nullptr;

		if(audio == empty_audio())
			return make_safe<core::audio_buffer>(format_desc_.audio_samples_per_frame, 0);

		return audio;
	}
	
	virtual void run()
	{
		try
		{
			send(is_running_, true);
			while(is_running_.value())
			{
				auto audio = receive(audio_);								
				auto video = receive(video_);
				
				if(!audio)
				{
					send(is_running_ , false);
					break;
				}

				if(!video)
				{
					send(is_running_ , false);
					break;
				}

				video->audio_data() = std::move(*audio);

				auto frame = safe_ptr<core::basic_frame>(video);

				switch(display_mode_)
				{
				case display_mode::simple:			
				case display_mode::deinterlace:
				case display_mode::deinterlace_bob:
					{
						send(target_, frame);

						break;
					}
				case display_mode::duplicate:					
					{										
						send(target_, frame);
						send(target_, frame);

						break;
					}
				case display_mode::half:						
					{								
						send(target_, frame);

						if(!receive(video_)) // throw away	
							send(is_running_ , false);

						break;
					}
				case display_mode::deinterlace_bob_reinterlace:
				case display_mode::interlace:					
					{					
						auto video2 = receive(video_);
						auto frame2 = core::basic_frame::empty();

						if(video2)
							frame2 = safe_ptr<core::basic_frame>(video2);
						else
							send(is_running_, false);						
						
						frame = core::basic_frame::interlace(frame, frame2, format_desc_.field_mode);	
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
			
	void push_video(const safe_ptr<AVFrame>& video_frame)
	{				
		if(video_frame == eof_video() || video_frame == empty_video())
		{
			send(video_, video_frame);
			return;
		}
				
		if(video_frame == loop_video())		
			return;	
		
		if(display_mode_ == display_mode::invalid)
		{
			if(auto_transcode_)
			{
				auto mode = get_mode(*video_frame);
				auto fps  = in_fps_;

				if(is_deinterlacing(filter_str_))
					mode = core::field_mode::progressive;

				if(is_double_rate(filter_str_))
					fps *= 2;

				display_mode_ = get_display_mode(mode, fps, format_desc_.field_mode, format_desc_.fps);
			
				if(display_mode_ == display_mode::simple && mode != core::field_mode::progressive && format_desc_.field_mode != core::field_mode::progressive && video_frame->height != static_cast<int>(format_desc_.height))
					display_mode_ = display_mode::deinterlace_bob_reinterlace; // The frame will most likely be scaled, we need to deinterlace->reinterlace	
				
				if(display_mode_ == display_mode::deinterlace)
					append_filter(filter_str_, L"YADIF=0:-1");
				else if(display_mode_ == display_mode::deinterlace_bob || display_mode_ == display_mode::deinterlace_bob_reinterlace)
					append_filter(filter_str_, L"YADIF=1:-1");
			}
			else
				display_mode_ = display_mode::simple;

			if(display_mode_ == display_mode::invalid)
			{
				CASPAR_LOG(warning) << L"[frame_muxer] Failed to detect display-mode.";
				display_mode_ = display_mode::simple;
			}
			
			filter_ = filter(filter_str_);

			CASPAR_LOG(info) << "[frame_muxer] " << display_mode::print(display_mode_);
		}
						
		//if(hints & core::frame_producer::ALPHA_HINT)
		//	video_frame->format = make_alpha_format(video_frame->format);
		
		auto format = video_frame->format;
		if(video_frame->format == CASPAR_PIX_FMT_LUMA) // CASPAR_PIX_FMT_LUMA is not valid for filter, change it to GRAY8
			video_frame->format = PIX_FMT_GRAY8;

		filter_.push(video_frame);

		while(true)
		{
			auto frame = filter_.poll();
			if(!frame)
				break;	

			frame->format = format;
			send(video_, make_safe_ptr(frame));
		}
	}

	void push_audio(const safe_ptr<core::audio_buffer>& audio_samples)
	{
		if(audio_samples == eof_audio() || audio_samples == empty_audio())
		{
			send(audio_, audio_samples);
			return;
		}

		if(audio_samples == loop_audio())			
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
					
	int64_t calc_nb_frames(int64_t nb_frames) const
	{
		switch(display_mode_)
		{
		case display_mode::interlace:
		case display_mode::half:
			return nb_frames/2;
		case display_mode::duplicate:
		case display_mode::deinterlace_bob:
			return nb_frames*2;
		default:
			return nb_frames;
		}
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