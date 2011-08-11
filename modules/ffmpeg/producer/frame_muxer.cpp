#include "../StdAfx.h"

#include "frame_muxer.h"

#include "filter/filter.h"

#include "util.h"

#include <core/producer/frame_producer.h>
#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/image_transform.h>
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

namespace caspar {

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
			case simple:
				return L"simple";
			case duplicate:
				return L"duplicate";
			case half:
				return L"half";
			case interlace:
				return L"interlace";
			case deinterlace_bob:
				return L"deinterlace_bob";
			case deinterlace_bob_reinterlace:
				return L"deinterlace_bob_reinterlace";
			case deinterlace:
				return L"deinterlace";
			default:
				return L"invalid";
		}
	}
};

display_mode::type get_display_mode(const core::video_mode::type in_mode, double in_fps, const core::video_mode::type out_mode, double out_fps)
{		
	static const auto epsilon = 2.0;

	if(std::abs(in_fps - out_fps) < epsilon)
	{
		if(in_mode != core::video_mode::progressive && out_mode == core::video_mode::progressive)
			return display_mode::deinterlace;
		//else if(in_mode == core::video_mode::progressive && out_mode != core::video_mode::progressive)
		//	simple(); // interlace_duplicate();
		else
			return display_mode::simple;
	}
	else if(std::abs(in_fps/2.0 - out_fps) < epsilon)
	{
		if(in_mode != core::video_mode::progressive)
			return display_mode::invalid;

		if(out_mode != core::video_mode::progressive)
			return display_mode::interlace;
		else
			return display_mode::half;
	}
	else if(std::abs(in_fps - out_fps/2.0) < epsilon)
	{
		if(out_mode != core::video_mode::progressive)
			return display_mode::invalid;

		if(in_mode != core::video_mode::progressive)
			return display_mode::deinterlace_bob;
		else
			return display_mode::duplicate;
	}

	return display_mode::invalid;
}

struct frame_muxer::implementation : boost::noncopyable
{	
	std::deque<std::queue<safe_ptr<write_frame>>>	video_streams_;
	std::deque<std::vector<int16_t>>				audio_streams_;
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
		
	implementation(double in_fps, const safe_ptr<core::frame_factory>& frame_factory)
		: video_streams_(1)
		, audio_streams_(1)
		, display_mode_(display_mode::invalid)
		, in_fps_(in_fps)
		, format_desc_(frame_factory->get_video_format_desc())
		, auto_transcode_(env::properties().get("configuration.producers.auto-transcode", false))
		, audio_sample_count_(0)
		, video_frame_count_(0)
		, frame_factory_(frame_factory)
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
		{
			if(auto_transcode_)
			{
				auto in_mode = get_mode(*video_frame);
				display_mode_ = get_display_mode(in_mode, in_fps_, format_desc_.mode, format_desc_.fps);
			
				if(display_mode_ == display_mode::simple && in_mode != core::video_mode::progressive && format_desc_.mode != core::video_mode::progressive && video_frame->height != static_cast<int>(format_desc_.height))
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

			CASPAR_LOG(info) << "[frame_muxer] " << display_mode::print(display_mode_);
		}
				
		if(hints & core::frame_producer::ALPHA_HINT)
			try_make_gray(make_safe(video_frame));

		BOOST_FOREACH(auto& av_frame, filter_.execute(video_frame))
		{
			auto frame = make_write_frame(this, av_frame, frame_factory_, hints);

			// Fix field-order if needed
			if(frame->get_type() == core::video_mode::lower && format_desc_.mode == core::video_mode::upper)
				frame->get_image_transform().set_fill_translation(0.0f, 0.5/static_cast<double>(frame->get_pixel_format_desc().planes[0].height));
			else if(frame->get_type() == core::video_mode::upper && format_desc_.mode == core::video_mode::lower)
				frame->get_image_transform().set_fill_translation(0.0f, -0.5/static_cast<double>(frame->get_pixel_format_desc().planes[0].height));

			video_streams_.back().push(frame);
			++video_frame_count_;
		}
	}

	void push(const std::shared_ptr<std::vector<int16_t>>& audio_samples)
	{
		if(!audio_samples)	
		{
			CASPAR_LOG(debug) << L"audio-chunk-count: " << audio_sample_count_/format_desc_.audio_samples_per_frame;
			audio_streams_.push_back(std::vector<int16_t>());
			audio_sample_count_ = 0;
			return;
		}

		audio_sample_count_ += audio_samples->size();

		boost::range::push_back(audio_streams_.back(), *audio_samples);
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

	std::vector<int16_t> pop_audio()
	{
		CASPAR_VERIFY(audio_streams_.front().size() >= format_desc_.audio_samples_per_frame);

		auto begin = audio_streams_.front().begin();
		auto end   = begin + format_desc_.audio_samples_per_frame;

		auto samples = std::vector<int16_t>(begin, end);
		audio_streams_.front().erase(begin, end);

		return samples;
	}

	bool video_ready() const
	{
		return video_frames() > 1 && video_streams_.size() >= audio_streams_.size();
	}
	
	bool audio_ready() const
	{
		return audio_chunks() > 1 && audio_streams_.size() >= video_streams_.size();
	}

	size_t video_frames() const
	{
		return video_streams_.back().size();
	}

	size_t audio_chunks() const
	{
		return audio_streams_.back().size() / format_desc_.audio_samples_per_frame;
	}
	
	void commit()
	{
		if(video_streams_.size() > 1 && audio_streams_.size() > 1 && !ready())
		{
			if(!video_streams_.front().empty() || !audio_streams_.front().empty())
				CASPAR_LOG(debug) << "Truncating: " << video_streams_.front().size() << L" video-frames, " << audio_streams_.front().size() << L" audio-samples.";

			video_streams_.pop_front();
			audio_streams_.pop_front();
		}

		if(!ready())
			return;
		
		switch(display_mode_)
		{
		case display_mode::simple:						return simple(frame_buffer_);
		case display_mode::duplicate:					return duplicate(frame_buffer_);
		case display_mode::half:						return half(frame_buffer_);
		case display_mode::interlace:					return interlace(frame_buffer_);
		case display_mode::deinterlace_bob:				return simple(frame_buffer_);
		case display_mode::deinterlace_bob_reinterlace:	return interlace(frame_buffer_);
		case display_mode::deinterlace:					return simple(frame_buffer_);
		default:										BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("invalid display-mode"));
		}
	}

	bool ready()
	{
		switch(display_mode_)
		{
		case display_mode::deinterlace_bob:
		case display_mode::deinterlace:
		case display_mode::simple:						return !video_streams_.front().empty() && audio_streams_.front().size() >= format_desc_.audio_samples_per_frame;
		case display_mode::duplicate:					return !video_streams_.front().empty() && audio_streams_.front().size()/2 >= format_desc_.audio_samples_per_frame;
		case display_mode::half:	
		case display_mode::deinterlace_bob_reinterlace:					
		case display_mode::interlace:					return video_streams_.front().size() >= 2 && audio_streams_.front().size() >= format_desc_.audio_samples_per_frame;
		default:										return false;
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

		dest.push_back(core::basic_frame::interlace(frame1, frame2, format_desc_.mode));		
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

frame_muxer::frame_muxer(double in_fps, const safe_ptr<core::frame_factory>& frame_factory)
	: impl_(new implementation(in_fps, frame_factory)){}
void frame_muxer::push(const std::shared_ptr<AVFrame>& video_frame, int hints){impl_->push(video_frame, hints);}
void frame_muxer::push(const std::shared_ptr<std::vector<int16_t>>& audio_samples){return impl_->push(audio_samples);}
void frame_muxer::commit(){impl_->commit();}
safe_ptr<basic_frame> frame_muxer::pop(){return impl_->pop();}
size_t frame_muxer::size() const {return impl_->size();}
bool frame_muxer::empty() const {return impl_->size() == 0;}
bool frame_muxer::video_ready() const{return impl_->video_ready();}
bool frame_muxer::audio_ready() const{return impl_->audio_ready();}
int64_t frame_muxer::calc_nb_frames(int64_t nb_frames) const {return impl_->calc_nb_frames(nb_frames);}

}