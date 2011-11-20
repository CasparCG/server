#include "../../StdAfx.h"

#include "frame_muxer.h"

#include "display_mode.h"

#include "../filter/filter.h"
#include "../util/util.h"

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

#include <deque>
#include <queue>
#include <vector>

using namespace caspar::core;

namespace caspar { namespace ffmpeg {
	
struct frame_muxer::implementation : boost::noncopyable
{	
	std::queue<std::queue<safe_ptr<write_frame>>>	video_streams_;
	std::queue<core::audio_buffer>					audio_streams_;
	std::queue<safe_ptr<basic_frame>>				frame_buffer_;
	display_mode::type								display_mode_;
	const double									in_fps_;
	const video_format_desc							format_desc_;
	bool											auto_transcode_;

	size_t											audio_sample_count_;
	size_t											video_frame_count_;
		
	safe_ptr<core::frame_factory>					frame_factory_;
	
	filter											filter_;
	std::wstring									filter_str_;
		
	implementation(double in_fps, const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filter_str)
		: display_mode_(display_mode::invalid)
		, in_fps_(in_fps)
		, format_desc_(frame_factory->get_video_format_desc())
		, auto_transcode_(env::properties().get("configuration.auto-transcode", true))
		, audio_sample_count_(0)
		, video_frame_count_(0)
		, frame_factory_(frame_factory)
		, filter_str_(filter_str)
	{
		video_streams_.push(std::queue<safe_ptr<write_frame>>());
		audio_streams_.push(core::audio_buffer());
	}

	void push(const std::shared_ptr<AVFrame>& video_frame, int hints)
	{		
		if(!video_frame)
			return;
		
		if(video_frame == flush_video())
		{	
			CASPAR_LOG(trace) << L"video-frame-count: " << static_cast<float>(video_frame_count_);
			video_frame_count_ = 0;
			video_streams_.push(std::queue<safe_ptr<write_frame>>());
		}
		else if(video_frame == empty_video())
		{
			video_streams_.back().push(make_safe<core::write_frame>(this));
			++video_frame_count_;
			display_mode_ = display_mode::simple;
		}
		else
		{
			if(display_mode_ == display_mode::invalid)
				initialize_display_mode(*video_frame);
				
			if(hints & core::frame_producer::ALPHA_HINT)
				video_frame->format = make_alpha_format(video_frame->format);
		
			auto format = video_frame->format;
			if(video_frame->format == CASPAR_PIX_FMT_LUMA) // CASPAR_PIX_FMT_LUMA is not valid for filter, change it to GRAY8
				video_frame->format = PIX_FMT_GRAY8;

			filter_.push(video_frame);
			BOOST_FOREACH(auto& av_frame, filter_.poll_all())
			{
				av_frame->format = format;
				video_streams_.back().push(make_write_frame(this, av_frame, frame_factory_, hints));
				++video_frame_count_;
			}
		}

		if(video_streams_.back().size() > 32)
			BOOST_THROW_EXCEPTION(invalid_operation() << source_info("frame_muxer") << msg_info("video-stream overflow. This can be caused by incorrect frame-rate. Check clip meta-data."));
	}

	void push(const std::shared_ptr<core::audio_buffer>& audio)
	{
		if(!audio)	
			return;

		if(audio == flush_audio())
		{
			CASPAR_LOG(trace) << L"audio-frame-count: " << audio_sample_count_/format_desc_.audio_samples_per_frame;
			audio_sample_count_ = 0;
			audio_streams_.push(core::audio_buffer());
		}
		else if(audio == empty_audio())
		{
			boost::range::push_back(audio_streams_.back(), core::audio_buffer(format_desc_.audio_samples_per_frame, 0));
			audio_sample_count_ += audio->size();
		}
		else
		{
			boost::range::push_back(audio_streams_.back(), *audio);
			audio_sample_count_ += audio->size();
		}

		if(audio_streams_.back().size() > 32*format_desc_.audio_samples_per_frame)
			BOOST_THROW_EXCEPTION(invalid_operation() << source_info("frame_muxer") << msg_info("audio-stream overflow. This can be caused by incorrect frame-rate. Check clip meta-data."));
	}
	
	bool video_ready() const
	{		
		return video_streams_.size() > 1 || (video_streams_.size() >= audio_streams_.size() && video_ready2());
	}
	
	bool audio_ready() const
	{
		return audio_streams_.size() > 1 || (audio_streams_.size() >= video_streams_.size() && audio_ready2());
	}

	bool video_ready2() const
	{		
		switch(display_mode_)
		{
		case display_mode::deinterlace_bob_reinterlace:					
		case display_mode::interlace:	
		case display_mode::half:
			return video_streams_.front().size() >= 2;
		default:										
			return video_streams_.front().size() >= 1;
		}
	}
	
	bool audio_ready2() const
	{
		switch(display_mode_)
		{
		case display_mode::duplicate:					
			return audio_streams_.front().size()/2 >= format_desc_.audio_samples_per_frame;
		default:										
			return audio_streams_.front().size() >= format_desc_.audio_samples_per_frame;
		}
	}
		
	std::shared_ptr<basic_frame> poll()
	{
		if(!frame_buffer_.empty())
		{
			auto frame = frame_buffer_.front();
			frame_buffer_.pop();	
			return frame;
		}

		if(video_streams_.size() > 1 && audio_streams_.size() > 1 && (!video_ready2() || !audio_ready2()))
		{
			if(!video_streams_.front().empty() || !audio_streams_.front().empty())
				CASPAR_LOG(debug) << "Truncating: " << video_streams_.front().size() << L" video-frames, " << audio_streams_.front().size() << L" audio-samples.";

			video_streams_.pop();
			audio_streams_.pop();
		}

		if(!video_ready2() || !audio_ready2())
			return nullptr;
				
		auto frame1				= pop_video();
		frame1->audio_data()	= pop_audio();

		switch(display_mode_)
		{
		case display_mode::simple:						
		case display_mode::deinterlace_bob:				
		case display_mode::deinterlace:	
		{
			frame_buffer_.push(frame1);
			break;
		}
		case display_mode::interlace:					
		case display_mode::deinterlace_bob_reinterlace:	
		{				
			auto frame2 = pop_video();

			frame_buffer_.push(core::basic_frame::interlace(frame1, frame2, format_desc_.field_mode));	
			break;
		}
		case display_mode::duplicate:	
		{
			auto frame2				= make_safe<core::write_frame>(*frame1);
			frame2->audio_data()	= pop_audio();

			frame_buffer_.push(frame1);
			frame_buffer_.push(frame2);
			break;
		}
		case display_mode::half:	
		{				
			pop_video(); // Throw away

			frame_buffer_.push(frame1);
			break;
		}
		default:										
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("invalid display-mode"));
		}
		
		return frame_buffer_.empty() ? nullptr : poll();
	}
	
	safe_ptr<core::write_frame> pop_video()
	{
		auto frame = video_streams_.front().front();
		video_streams_.front().pop();		
		return frame;
	}

	core::audio_buffer pop_audio()
	{
		CASPAR_VERIFY(audio_streams_.front().size() >= format_desc_.audio_samples_per_frame);

		auto begin = audio_streams_.front().begin();
		auto end   = begin + format_desc_.audio_samples_per_frame;

		auto samples = core::audio_buffer(begin, end);
		audio_streams_.front().erase(begin, end);

		return samples;
	}
				
	void initialize_display_mode(const AVFrame& frame)
	{
		display_mode_ = display_mode::simple;
		if(auto_transcode_)
		{
			auto mode = get_mode(frame);
			auto fps  = in_fps_;

			if(is_deinterlacing(filter_str_))
				mode = core::field_mode::progressive;

			if(is_double_rate(filter_str_))
				fps *= 2;
			
			display_mode_ = get_display_mode(mode, fps, format_desc_.field_mode, format_desc_.fps);
			
			if((frame.height != 480 || format_desc_.height != 486) && 
				display_mode_ == display_mode::simple && mode != core::field_mode::progressive && format_desc_.field_mode != core::field_mode::progressive && 
				frame.height != static_cast<int>(format_desc_.height))
			{
				display_mode_ = display_mode::deinterlace_bob_reinterlace; // The frame will most likely be scaled, we need to deinterlace->reinterlace	
			}

			if(display_mode_ == display_mode::deinterlace)
				filter_str_ = append_filter(filter_str_, L"YADIF=0:-1");
			else if(display_mode_ == display_mode::deinterlace_bob || display_mode_ == display_mode::deinterlace_bob_reinterlace)
				filter_str_ = append_filter(filter_str_, L"YADIF=1:-1");
		}

		if(display_mode_ == display_mode::invalid)
		{
			CASPAR_LOG(warning) << L"[frame_muxer] Auto-transcode: Failed to detect display-mode.";
			display_mode_ = display_mode::simple;
		}
			
		filter_ = filter(filter_str_);

		CASPAR_LOG(info) << "[frame_muxer] " << display_mode::print(display_mode_) 
			<< L" " << frame.width << L"x" << frame.height 
			<< (frame.interlaced_frame ? L"i" : L"p") 
			<< (frame.interlaced_frame ? in_fps_*2 : in_fps_);
	}

	int64_t calc_nb_frames(int64_t nb_frames) const
	{
		switch(display_mode_) // Take into account transformation in run.
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

		if(is_double_rate(widen(filter_.filter_str()))) // Take into account transformations in filter.
			nb_frames *= 2;

		return nb_frames;
	}
};

frame_muxer::frame_muxer(double in_fps, const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filter)
	: impl_(new implementation(in_fps, frame_factory, filter)){}
void frame_muxer::push(const std::shared_ptr<AVFrame>& video_frame, int hints){impl_->push(video_frame, hints);}
void frame_muxer::push(const std::shared_ptr<core::audio_buffer>& audio_samples){return impl_->push(audio_samples);}
std::shared_ptr<basic_frame> frame_muxer::poll(){return impl_->poll();}
int64_t frame_muxer::calc_nb_frames(int64_t nb_frames) const {return impl_->calc_nb_frames(nb_frames);}
bool frame_muxer::video_ready() const{return impl_->video_ready();}
bool frame_muxer::audio_ready() const{return impl_->audio_ready();}

}}