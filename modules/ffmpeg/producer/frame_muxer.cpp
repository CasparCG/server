#include "../StdAfx.h"

#include "frame_muxer.h"

#include "filter/filter.h"

#include "util.h"

#include <core/producer/frame_producer.h>
#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_transform.h>
#include <core/producer/frame/pixel_format.h>
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
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/assign.hpp>

#include <agents_extras.h>

#include <deque>
#include <tuple>
#include <queue>
#include <vector>

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
	ITarget<frame_message_t>&														target_;
	display_mode::type																display_mode_;
	const double																	in_fps_;
	const video_format_desc															format_desc_;
	bool																			auto_transcode_;
	
	filter																			filter_;
	const safe_ptr<core::frame_factory>												frame_factory_;
	
	call<video_message_t>															push_video_;
	call<audio_message_t>															push_audio_;
	
	transformer<video_message_t, std::shared_ptr<message<std::shared_ptr<write_frame>>>>	video_;
	unbounded_buffer<audio_message_t>												audio_;
	
	core::audio_buffer																audio_data_;

	Concurrency::overwrite_buffer<bool>												is_running_;
							
	implementation(frame_muxer2::video_source_t* video_source,
				   frame_muxer2::audio_source_t* audio_source,
				   frame_muxer2::target_t& target,
				   double in_fps, 
				   const safe_ptr<core::frame_factory>& frame_factory)
		: target_(target)
		, display_mode_(display_mode::invalid)
		, in_fps_(in_fps)
		, format_desc_(frame_factory->get_video_format_desc())
		, auto_transcode_(env::properties().get("configuration.producers.auto-transcode", false))
		, frame_factory_(make_safe<core::concrt_frame_factory>(frame_factory))
		, push_video_(std::bind(&implementation::push_video, this, std::placeholders::_1))
		, push_audio_(std::bind(&implementation::push_audio, this, std::placeholders::_1))
		, video_(std::bind(&implementation::make_write_frame, this, std::placeholders::_1))
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

	std::shared_ptr<message<std::shared_ptr<core::write_frame>>> make_write_frame(const video_message_t& message)
	{
		if(message->payload == eof_video())
			return make_message<std::shared_ptr<core::write_frame>>(nullptr);

		if(message->payload == empty_video())
			return make_message<std::shared_ptr<core::write_frame>>(std::make_shared<core::write_frame>(this));

		return make_message<std::shared_ptr<core::write_frame>>(ffmpeg::make_write_frame(this, make_safe_ptr(message->payload), frame_factory_, 0), message->token);
	}	

	virtual void run()
	{
		try
		{
			send(is_running_, true);
			while(is_running_.value())
			{
				auto video = receive(video_);
				auto audio = receive(audio_);								

				if(!audio->payload)
				{
					send(is_running_ , false);
					break;
				}

				if(!video->payload)
				{
					send(is_running_ , false);
					break;
				}

				video->payload->audio_data() = std::move(*audio->payload);

				switch(display_mode_)
				{
				case display_mode::simple:			
				case display_mode::deinterlace:
				case display_mode::deinterlace_bob:
					{
						auto message = make_message(safe_ptr<core::basic_frame>(video->payload), video->token ? video->token : audio->token);
						send(target_, message);

						break;
					}
				case display_mode::duplicate:					
					{										
						auto message = make_message(safe_ptr<core::basic_frame>(video->payload), video->token ? video->token : audio->token);
						send(target_, message);
						send(target_, message);

						break;
					}
				case display_mode::half:						
					{					
						receive(video_);

						auto message = make_message(safe_ptr<core::basic_frame>(video->payload), video->token ? video->token : audio->token);
						send(target_, message);

						break;
					}
				case display_mode::deinterlace_bob_reinterlace:
				case display_mode::interlace:					
					{					
						auto frame = safe_ptr<core::basic_frame>(video->payload);
						auto video2 = receive(video_);	
						if(video->payload)
							frame = core::basic_frame::interlace(make_safe_ptr(video->payload), make_safe_ptr(video2->payload), format_desc_.field_mode);
						else
							send(is_running_, false);

						auto message = make_message<safe_ptr<core::basic_frame>>(frame, video2->token ? video2->token : audio->token);
						send(target_, message);

						break;
					}
				default:	
					BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("invalid display-mode"));
				}
			}
		}
		catch(...)
		{
			send(is_running_ , false);
			CASPAR_LOG_CURRENT_EXCEPTION();
		}

		send(target_, make_message(core::basic_frame::eof()));

		done();
	}
			
	void push_video(const video_message_t& message)
	{		
		auto video_frame = message->payload;

		if(!video_frame)
			return;
		
		if(video_frame == eof_video())
		{
			send(video_, make_message(eof_video()));
			return;
		}

		if(video_frame == loop_video())		
			return;	
				
		if(video_frame == empty_video())
		{
			send(video_, make_message(empty_video()));
			return;
		}
		
		if(display_mode_ == display_mode::invalid)
		{
			if(auto_transcode_)
			{
				auto in_mode = get_mode(*video_frame);
				display_mode_ = get_display_mode(in_mode, in_fps_, format_desc_.field_mode, format_desc_.fps);
			
				if(display_mode_ == display_mode::simple && in_mode != core::field_mode::progressive && format_desc_.field_mode != core::field_mode::progressive && video_frame->height != static_cast<int>(format_desc_.height))
					display_mode_ = display_mode::deinterlace_bob_reinterlace; // The frame will most likely be scaled, we need to deinterlace->reinterlace	
				
				if(display_mode_ == display_mode::deinterlace)
					filter_ = filter(L"YADIF=0:-1");
				else if(display_mode_ == display_mode::deinterlace_bob || display_mode_ == display_mode::deinterlace_bob_reinterlace)
					filter_ = filter(L"YADIF=1:-1");
			}
			else
				display_mode_ = display_mode::simple;

			if(display_mode_ == display_mode::invalid)
			{
				CASPAR_LOG(warning) << L"[frame_muxer] Failed to detect display-mode.";
				display_mode_ = display_mode::simple;
			}

			// copy <= We need to release frames
			if(display_mode_ != display_mode::simple && filter_.filter_str().empty())
				filter_ = filter(L"copy"); 

			CASPAR_LOG(info) << "[frame_muxer] " << display_mode::print(display_mode_);
		}
						
		//if(hints & core::frame_producer::ALPHA_HINT)
		//	video_frame->format = make_alpha_format(video_frame->format);
		
		auto format = video_frame->format;
		if(video_frame->format == CASPAR_PIX_FMT_LUMA) // CASPAR_PIX_FMT_LUMA is not valid for filter, change it to GRAY8
			video_frame->format = PIX_FMT_GRAY8;

		filter_.push(video_frame);

		BOOST_FOREACH(auto av_frame, filter_.poll_all())
		{		
			av_frame->format = format;			
			send(video_, make_message(std::shared_ptr<AVFrame>(av_frame), std::move(message->token)));
		}
	}

	void push_audio(const audio_message_t& message)
	{
		auto audio_samples = message->payload;

		if(!audio_samples)
			return;

		if(audio_samples == eof_audio())
		{
			send(audio_, make_message(std::shared_ptr<core::audio_buffer>()));
			return;
		}

		if(audio_samples == loop_audio())			
			return;		

		if(audio_samples == empty_audio())		
			send(audio_, make_message(std::make_shared<core::audio_buffer>(format_desc_.audio_samples_per_frame, 0)));		

		audio_data_.insert(audio_data_.end(), audio_samples->begin(), audio_samples->end());
		
		while(audio_data_.size() >= format_desc_.audio_samples_per_frame)
		{
			auto begin = audio_data_.begin(); 
			auto end   = begin + format_desc_.audio_samples_per_frame;
					
			send(audio_, make_message(std::make_shared<core::audio_buffer>(begin, end), message->token));

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
						   const safe_ptr<core::frame_factory>& frame_factory)
	: impl_(new implementation(video_source, audio_source, target, in_fps, frame_factory))
{
}

int64_t frame_muxer2::calc_nb_frames(int64_t nb_frames) const
{
	return impl_->calc_nb_frames(nb_frames);
}

}}