#include "../stdafx.h"

#include "layer.h"

#include "../mixer/frame/draw_frame.h"
#include "../mixer/image/image_mixer.h"
#include "../mixer/audio/audio_mixer.h"
#include "../producer/frame_producer.h"

#include "../video_format.h"

#include <common/utility/assert.h>

namespace caspar { namespace core {

struct layer::implementation : boost::noncopyable
{					
	safe_ptr<frame_producer>	foreground_;
	safe_ptr<frame_producer>	background_;
	safe_ptr<draw_frame>		last_frame_;
	const int					index_;
	bool						is_paused_;

	double						video_gain_;
	double						video_opacity_;
	
	double						audio_gain_;

public:
	implementation(int index) 
		: foreground_(frame_producer::empty())
		, background_(frame_producer::empty())
		, last_frame_(draw_frame::empty())
		, index_(index) 
		, is_paused_(false)
		, video_gain_(1.0)
		, video_opacity_(1.0)
		, audio_gain_(1.0){}
	
	void load(const safe_ptr<frame_producer>& frame_producer, bool play_on_load)
	{			
		background_ = frame_producer;
		CASPAR_LOG(info) << print() << " " << frame_producer->print() << " => background";
		if(play_on_load)
			play();			
	}

	void preview(const safe_ptr<frame_producer>& frame_producer)
	{
		stop();
		load(frame_producer, false);		
		try
		{
			last_frame_ = frame_producer->receive();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			clear();
		}
	}
	
	void play()
	{			
		if(is_paused_)			
			is_paused_ = false;
		else
		{
			background_->set_leading_producer(foreground_);
			foreground_ = background_;
			background_ = frame_producer::empty();
			CASPAR_LOG(info) << print() << L" background => foreground";
		}
	}

	void pause()
	{
		is_paused_ = true;
	}

	void stop()
	{
		pause();
		last_frame_ = draw_frame::empty();
		foreground_ = frame_producer::empty();
		CASPAR_LOG(warning) << print() << L" empty => foreground";
	}

	void clear()
	{
		stop();
		background_ = frame_producer::empty();
		CASPAR_LOG(warning) << print() << L" empty => background";
	}
	
	safe_ptr<draw_frame> receive()
	{		
		if(is_paused_)
			return last_frame_;

		try
		{
			last_frame_ = foreground_->receive(); 
			if(last_frame_ == draw_frame::eof())
			{
				CASPAR_ASSERT(foreground_ != frame_producer::empty());

				auto following = foreground_->get_following_producer();
				following->set_leading_producer(foreground_);
				foreground_ = following;

				CASPAR_LOG(info) << print() << L" [EOF] " << foreground_->print() << " => foreground";

				last_frame_ = receive();
			}
			else
			{
				last_frame_ = draw_frame(last_frame_);
				last_frame_->get_image_transform().gain *= video_gain_;
				last_frame_->get_image_transform().alpha *= video_opacity_;
				last_frame_->get_audio_transform().gain *= audio_gain_;
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			stop();
		}

		return last_frame_;
	}

	std::wstring print() const { return L"layer[" + boost::lexical_cast<std::wstring>(index_) + L"]"; }
};

layer::layer(int index) : impl_(new implementation(index)){}
layer::layer(layer&& other) : impl_(std::move(other.impl_)){other.impl_ = nullptr;}
layer& layer::operator=(layer&& other)
{
	impl_ = std::move(other.impl_);	
	other.impl_ = nullptr;
	return *this;
}
void layer::load(const safe_ptr<frame_producer>& frame_producer, bool play_on_load){return impl_->load(frame_producer, play_on_load);}	
void layer::preview(const safe_ptr<frame_producer>& frame_producer){return impl_->preview(frame_producer);}	
void layer::set_video_gain(double value) { impl_->video_gain_ = value;}
void layer::set_video_opacity(double value) { impl_->video_opacity_ = value;}
void layer::set_audio_gain(double value) { impl_->audio_gain_ = value;}
void layer::play(){impl_->play();}
void layer::pause(){impl_->pause();}
void layer::stop(){impl_->stop();}
void layer::clear(){impl_->clear();}
safe_ptr<draw_frame> layer::receive() {return impl_->receive();}
safe_ptr<frame_producer> layer::foreground() const { return impl_->foreground_;}
safe_ptr<frame_producer> layer::background() const { return impl_->background_;}
}}