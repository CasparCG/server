#include "../StdAfx.h"

#include "frame_muxer.h"

#include "filter/filter.h"

#include "util.h"
#include "display_mode.h"

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
	std::deque<std::queue<safe_ptr<write_frame>>>	video_streams_;
	std::deque<core::audio_buffer>					audio_streams_;
	std::deque<safe_ptr<basic_frame>>				frame_buffer_;
	display_mode::type								display_mode_;
	const double									in_fps_;
	const video_format_desc							format_desc_;
	bool											auto_transcode_;

	size_t											audio_sample_count_;
	size_t											video_frame_count_;
		
	size_t											processed_audio_sample_count_;
	size_t											processed_video_frame_count_;

	filter											filter_;
	safe_ptr<core::frame_factory>					frame_factory_;
	std::wstring									filter_str_;
		
	implementation(double in_fps, const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filter_str)
		: video_streams_(1)
		, audio_streams_(1)
		, display_mode_(display_mode::invalid)
		, in_fps_(in_fps)
		, format_desc_(frame_factory->get_video_format_desc())
		, auto_transcode_(env::properties().get("configuration.producers.auto-transcode", false))
		, audio_sample_count_(0)
		, video_frame_count_(0)
		, frame_factory_(frame_factory)
		, filter_str_(filter_str)
	{
	}

	void push(const std::shared_ptr<AVFrame>& video_frame, int hints)
	{		
		if(!video_frame)
		{	
			CASPAR_LOG(debug) << L"video-frame-count: " << static_cast<float>(video_frame_count_);
			video_frame_count_ = 0;
			video_streams_.push_back(std::queue<safe_ptr<write_frame>>());
			return;
		}

		if(video_frame->data[0] == nullptr)
		{
			video_streams_.back().push(make_safe<core::write_frame>(this));
			++video_frame_count_;
			display_mode_ = display_mode::simple;
			return;
		}

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

		if(video_streams_.back().size() > 8)
			BOOST_THROW_EXCEPTION(invalid_operation() << source_info("frame_muxer") << msg_info("video-stream overflow. This can be caused by incorrect frame-rate. Check clip meta-data."));
	}

	void push(const std::shared_ptr<core::audio_buffer>& audio_samples)
	{
		if(!audio_samples)	
		{
			CASPAR_LOG(debug) << L"audio-chunk-count: " << audio_sample_count_/format_desc_.audio_samples_per_frame;
			audio_streams_.push_back(core::audio_buffer());
			audio_sample_count_ = 0;
			return;
		}

		audio_sample_count_ += audio_samples->size();

		boost::range::push_back(audio_streams_.back(), *audio_samples);

		if(audio_streams_.back().size() > 8*format_desc_.audio_samples_per_frame)
			BOOST_THROW_EXCEPTION(invalid_operation() << source_info("frame_muxer") << msg_info("audio-stream overflow. This can be caused by incorrect frame-rate. Check clip meta-data."));
	}

	safe_ptr<basic_frame> pop()
	{		
		auto frame = frame_buffer_.front();
		frame_buffer_.pop_front();		
		return frame;
	}

	size_t size() const
	{
		return frame_buffer_.size();
	}

	safe_ptr<core::write_frame> pop_video()
	{
		auto frame = video_streams_.front().front();
		video_streams_.front().pop();
		
		return frame;
	}

	core::audio_buffer pop_audio()
	{
		auto begin = audio_streams_.front().begin();
		auto end   = begin + format_desc_.audio_samples_per_frame;

		auto samples = core::audio_buffer(begin, end);
		audio_streams_.front().erase(begin, end);

		return samples;
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
			return video_streams_.front().size() >= 2;
		default:										
			return !video_streams_.front().empty();
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
		
	void commit()
	{
		if(video_streams_.size() > 1 && audio_streams_.size() > 1 && (!video_ready2() || !audio_ready2()))
		{
			if(!video_streams_.front().empty() || !audio_streams_.front().empty())
				CASPAR_LOG(debug) << "Truncating: " << video_streams_.front().size() << L" video-frames, " << audio_streams_.front().size() << L" audio-samples.";

			video_streams_.pop_front();
			audio_streams_.pop_front();
		}

		if(!video_ready2() || !audio_ready2())
			return;
		
		switch(display_mode_)
		{
		case display_mode::simple:
		case display_mode::deinterlace_bob:
		case display_mode::deinterlace:					
			return simple(frame_buffer_);
		case display_mode::duplicate:					
			return duplicate(frame_buffer_);
		case display_mode::half:						
			return half(frame_buffer_);
		case display_mode::interlace:
		case display_mode::deinterlace_bob_reinterlace:	
			return interlace(frame_buffer_);
		default:										
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("invalid display-mode"));
		}
	}
	
	void simple(std::deque<safe_ptr<basic_frame>>& dest)
	{		
		auto frame1 = pop_video();
		frame1->audio_data() = pop_audio();

		dest.push_back(frame1);		
	}

	void duplicate(std::deque<safe_ptr<basic_frame>>& dest)
	{		
		auto frame = pop_video();

		auto frame1 = make_safe<core::write_frame>(*frame); // make a copy
		frame1->audio_data() = pop_audio();

		auto frame2 = frame;
		frame2->audio_data() = pop_audio();

		dest.push_back(frame1);
		dest.push_back(frame2);
	}

	void half(std::deque<safe_ptr<basic_frame>>& dest)
	{							
		auto frame1 = pop_video();
		frame1->audio_data() = pop_audio();
				
		video_streams_.front().pop(); // Throw away

		dest.push_back(frame1);
	}
	
	void interlace(std::deque<safe_ptr<basic_frame>>& dest)
	{				
		auto frame1 = pop_video();
		frame1->audio_data() = pop_audio();
				
		auto frame2 = pop_video();

		dest.push_back(core::basic_frame::interlace(frame1, frame2, format_desc_.field_mode));		
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
			
		filter_ = filter(filter_str_);

		CASPAR_LOG(info) << "[frame_muxer] " << display_mode::print(display_mode);

		display_mode_ = display_mode;
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

frame_muxer::frame_muxer(double in_fps, const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filter_str)
	: impl_(new implementation(in_fps, frame_factory, filter_str)){}
void frame_muxer::push(const std::shared_ptr<AVFrame>& video_frame, int hints){impl_->push(video_frame, hints);}
void frame_muxer::push(const std::shared_ptr<core::audio_buffer>& audio_samples){return impl_->push(audio_samples);}
void frame_muxer::commit(){impl_->commit();}
safe_ptr<basic_frame> frame_muxer::pop(){return impl_->pop();}
size_t frame_muxer::size() const {return impl_->size();}
bool frame_muxer::empty() const {return impl_->size() == 0;}
bool frame_muxer::video_ready() const{return impl_->video_ready();}
bool frame_muxer::audio_ready() const{return impl_->audio_ready();}
int64_t frame_muxer::calc_nb_frames(int64_t nb_frames) const {return impl_->calc_nb_frames(nb_frames);}

}}