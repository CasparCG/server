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

#include <boost/range/algorithm_ext/push_back.hpp>

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
	typedef	std::pair<std::shared_ptr<core::write_frame>, ticket_t>		write_element_t;
	typedef	std::pair<std::shared_ptr<core::audio_buffer>, ticket_t>	audio_element_t;

	frame_muxer2::video_source_t* video_source_;
	frame_muxer2::audio_source_t* audio_source_;

	ITarget<frame_muxer2::target_element_t>&		target_;
	mutable single_assignment<display_mode::type>	display_mode_;
	const double									in_fps_;
	const core::video_format_desc					format_desc_;
	const bool										auto_transcode_;
	
	mutable single_assignment<safe_ptr<filter>>		filter_;
	const safe_ptr<core::frame_factory>				frame_factory_;
			
	core::audio_buffer								audio_data_;
	
	std::queue<write_element_t>						video_frames_;
	std::queue<audio_element_t>						audio_buffers_;

	std::wstring									filter_str_;
	
	implementation(frame_muxer2::video_source_t* video_source,
				   frame_muxer2::audio_source_t* audio_source,
				   frame_muxer2::target_t& target,
				   double in_fps, 
				   const safe_ptr<core::frame_factory>& frame_factory,
				   const std::wstring& filter)
		: video_source_(video_source)
		, audio_source_(audio_source)
		, target_(target)
		, in_fps_(in_fps)
		, format_desc_(frame_factory->get_video_format_desc())
		, auto_transcode_(env::properties().get("configuration.producers.auto-transcode", false))
		, frame_factory_(frame_factory)
	{		
		start();
	}

	~implementation()
	{
		agent::wait(this);
	}
				
	write_element_t receive_video()
	{	
		if(!video_frames_.empty())
		{
			auto video_frame = video_frames_.front();
			video_frames_.pop();
			return video_frame;
		}

		auto element = receive(video_source_);
		auto video	 = element.first;

		if(video == eof_video())
			return write_element_t(nullptr, ticket_t());	
		else if(video == empty_video())
			return write_element_t(make_safe<core::write_frame>(this), ticket_t());
		else if(video != loop_video())
		{
			if(!display_mode_.has_value())
				initialize_display_mode(*video);
			
			filter_.value()->push(video);
			for(auto frame = filter_.value()->poll(); frame; frame = filter_.value()->poll())			
				video_frames_.push(write_element_t(make_write_frame(this, video, frame_factory_, 0), element.second));			
		}

		return receive_video();
	}
	
	audio_element_t receive_audio()
	{		
		if(!audio_buffers_.empty())
		{
			auto audio_buffer = audio_buffers_.front();
			audio_buffers_.pop();
			return audio_buffer;
		}
		
		auto element = receive(audio_source_);
		auto audio	 = element.first;

		if(audio == eof_audio())
			return audio_element_t(nullptr, ticket_t());	
		else if(audio == empty_audio())		
			audio_data_.resize(audio_data_.size() + format_desc_.audio_samples_per_frame, 0);
		else if(audio != loop_audio())			
			audio_data_.insert(audio_data_.end(), audio->begin(), audio->end());
		
		while(audio_data_.size() >= format_desc_.audio_samples_per_frame)
		{
			auto begin = audio_data_.begin(); 
			auto end   = begin + format_desc_.audio_samples_per_frame;
			auto audio = make_safe<core::audio_buffer>(begin, end);
			audio_data_.erase(begin, end);
			audio_buffers_.push(frame_muxer2::audio_source_element_t(audio, element.second));
		}

		return receive_audio();
	}
			
	virtual void run()
	{
		try
		{
			bool eof = false;
			while(!eof)
			{
				ticket_t tickets;

				auto audio_element = receive_audio();
				boost::range::push_back(tickets, audio_element.second);

				auto audio = audio_element.first;
				if(!audio)
				{
					eof = true;
					break;
				}
				
				auto video_element = receive_video();
				boost::range::push_back(tickets, video_element.second);

				auto video = video_element.first;
				if(!video)
				{
					eof = true;
					break;
				}

				video->audio_data() = std::move(*audio);

				switch(display_mode_.value())
				{
				case display_mode::simple:			
				case display_mode::deinterlace:
				case display_mode::deinterlace_bob:
					{
						send(target_, frame_muxer2::target_element_t(video, tickets));

						break;
					}
				case display_mode::duplicate:					
					{						
						auto video2 = make_safe<core::write_frame>(*video);				
						send(target_, frame_muxer2::target_element_t(video, tickets));

						auto audio_element2 = receive_audio();
						boost::range::push_back(tickets, audio_element.second);

						auto audio2	= audio_element2.first;
						if(audio2)
						{
							video2->audio_data() = std::move(*audio2);
							send(target_, frame_muxer2::target_element_t(video2, tickets));
						}
						else
							eof = true;

						break;
					}
				case display_mode::half:						
					{								
						send(target_, frame_muxer2::target_element_t(video, tickets));

						if(!receive_video().first)
							eof = true;

						break;
					}
				case display_mode::deinterlace_bob_reinterlace:
				case display_mode::interlace:					
					{					
						auto frame = safe_ptr<core::basic_frame>(std::move(video));

						auto video_element2 = receive_video();
						boost::range::push_back(tickets, video_element.second);

						auto video2 = video_element2.first;
						auto frame2 = core::basic_frame::empty();

						if(video2)						
							frame2 = safe_ptr<core::basic_frame>(video2);			
						else
							eof = true;
						
						frame = core::basic_frame::interlace(std::move(frame), std::move(frame2), format_desc_.field_mode);	
						send(target_, frame_muxer2::target_element_t(frame, tickets));

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
		
		send(target_, frame_muxer2::target_element_t(core::basic_frame::eof(), ticket_t()));

		done();
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