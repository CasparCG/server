#include "../StdAfx.h"

#include "frame_muxer.h"

#include "frame/basic_frame.h"
#include "../mixer/write_frame.h"

namespace caspar { namespace core {
	
struct display_mode
{
	enum type
	{
		simple,
		duplicate,
		half,
		interlace,
		deinterlace,
		deinterlace_half,
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
			case deinterlace:
				return L"deinterlace";
			case deinterlace_half:
				return L"deinterlace_half";
			default:
				return L"invalid";
		}
	}
};

display_mode::type get_display_mode(const core::video_mode::type in_mode, double in_fps, const core::video_mode::type out_mode, double out_fps)
{		
	if(in_mode == core::video_mode::invalid || out_mode == core::video_mode::invalid)
		return display_mode::invalid;

	static const auto epsilon = 2.0;

	if(std::abs(in_fps - out_fps) < epsilon)
	{
		if(in_mode != core::video_mode::progressive && out_mode == core::video_mode::progressive)
			return display_mode::deinterlace_half;
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
			return display_mode::deinterlace;
		else
			return display_mode::duplicate;
	}

	return display_mode::invalid;
}

struct frame_muxer::implementation
{	
	std::queue<safe_ptr<write_frame>> video_frames_;
	std::queue<std::vector<int16_t>>  audio_chunks_;
	std::queue<safe_ptr<basic_frame>> frame_buffer_;
	display_mode::type				  display_mode_;
	const double					  in_fps_;
	const double					  out_fps_;
	const video_mode::type			  out_mode_;

	implementation(double in_fps, const core::video_mode::type out_mode, double out_fps)
		: display_mode_(display_mode::invalid)
		, in_fps_(in_fps)
		, out_fps_(out_fps)
		, out_mode_(out_mode)
	{
	}

	void push(const safe_ptr<write_frame>& video_frame)
	{
		video_frames_.push(video_frame);
		process();
	}

	void push(const std::vector<int16_t>& audio_chunk)
	{
		audio_chunks_.push(audio_chunk);
		process();
	}

	safe_ptr<basic_frame> pop()
	{		
		auto frame = frame_buffer_.front();
		frame_buffer_.pop();
		return frame;
	}

	size_t size() const
	{
		return frame_buffer_.size();
	}

	void process()
	{
		if(video_frames_.empty() || audio_chunks_.empty())
			return;

		if(display_mode_ == display_mode::invalid)
			display_mode_ = get_display_mode(video_frames_.front()->get_type(), in_fps_, out_mode_, out_fps_);

		switch(display_mode_)
		{
		case display_mode::simple:
			return simple();
		case display_mode::duplicate:
			return duplicate();
		case display_mode::half:
			return half();
		case display_mode::interlace:
			return interlace();
		case display_mode::deinterlace:
			return deinterlace();
		case display_mode::deinterlace_half:
			return deinterlace_half();
		default:
			BOOST_THROW_EXCEPTION(invalid_operation());
		}
	}

	void simple()
	{
		if(video_frames_.empty() || audio_chunks_.empty())
			return;

		auto frame1 = video_frames_.front();
		video_frames_.pop();

		frame1->audio_data() = audio_chunks_.front();
		audio_chunks_.pop();

		frame_buffer_.push(frame1);
	}

	void duplicate()
	{		
		if(video_frames_.empty() || audio_chunks_.size() < 2)
			return;

		auto frame = video_frames_.front();
		video_frames_.pop();

		auto frame1 = make_safe<core::write_frame>(*frame); // make a copy
		frame1->audio_data() = audio_chunks_.front();
		audio_chunks_.pop();

		auto frame2 = frame;
		frame2->audio_data() = audio_chunks_.front();
		audio_chunks_.pop();

		frame_buffer_.push(frame1);
		frame_buffer_.push(frame2);
	}

	void half()
	{	
		if(video_frames_.size() < 2 || audio_chunks_.empty())
			return;
						
		auto frame1 = video_frames_.front();
		video_frames_.pop();
		frame1->audio_data() = audio_chunks_.front();
		audio_chunks_.pop();
				
		video_frames_.pop(); // Throw away

		frame_buffer_.push(frame1);
	}
	
	void interlace()
	{		
		if(video_frames_.size() < 2 || audio_chunks_.empty())
			return;
		
		auto frame1 = video_frames_.front();
		video_frames_.pop();

		frame1->audio_data() = audio_chunks_.front();
		audio_chunks_.pop();
				
		auto frame2 = video_frames_.front();
		video_frames_.pop();

		frame_buffer_.push(core::basic_frame::interlace(frame1, frame2, out_mode_));		
	}
	
	void deinterlace()
	{
		BOOST_THROW_EXCEPTION(not_implemented() << msg_info("deinterlace"));
	}

	void deinterlace_half()
	{
		BOOST_THROW_EXCEPTION(not_implemented() << msg_info("deinterlace_half"));
	}
};

frame_muxer::frame_muxer(double in_fps, const core::video_mode::type out_mode, double out_fps)
	: impl_(implementation(in_fps, out_mode, out_fps)){}
void frame_muxer::push(const safe_ptr<write_frame>& video_frame){impl_->push(video_frame);}
void frame_muxer::push(const std::vector<int16_t>& audio_chunk){return impl_->push(audio_chunk);}
safe_ptr<basic_frame> frame_muxer::pop(){return impl_->pop();}
size_t frame_muxer::size() const {return impl_->size();}
bool frame_muxer::empty() const {return impl_->size() == 0;}
size_t frame_muxer::video_frames() const{return impl_->video_frames_.size();}
size_t frame_muxer::audio_chunks() const{return impl_->audio_chunks_.size();}

}}