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
	bool						is_paused_;

public:
	implementation() 
		: foreground_(frame_producer::empty())
		, background_(frame_producer::empty())
		, last_frame_(draw_frame::empty())
		, is_paused_(false){}
	
	void load(int index, const safe_ptr<frame_producer>& frame_producer, bool play_on_load)
	{			
		background_ = frame_producer;
		CASPAR_LOG(info) << print(index) << " " << frame_producer->print() << " => background";
		if(play_on_load)
			play(index);			
	}

	void preview(int index, const safe_ptr<frame_producer>& frame_producer)
	{
		stop(index);
		load(index, frame_producer, false);		
		try
		{
			last_frame_ = frame_producer->receive();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			clear(index);
		}
	}
	
	void play(int index)
	{			
		if(is_paused_)			
			is_paused_ = false;
		else
		{
			background_->set_leading_producer(foreground_);
			foreground_ = background_;
			background_ = frame_producer::empty();
			CASPAR_LOG(info) << print(index) << L" background => foreground";
		}
	}

	void pause(int)
	{
		is_paused_ = true;
	}

	void stop(int index)
	{
		pause(index);
		last_frame_ = draw_frame::empty();
		foreground_ = frame_producer::empty();
		CASPAR_LOG(warning) << print(index) << L" empty => foreground";
	}

	void clear(int index)
	{
		stop(index);
		background_ = frame_producer::empty();
		CASPAR_LOG(warning) << print(index) << L" empty => background";
	}
	
	safe_ptr<draw_frame> receive(int index)
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

				CASPAR_LOG(info) << print(index) << L" [EOF] " << foreground_->print() << " => foreground";

				last_frame_ = receive(index);
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			stop(index);
		}

		return last_frame_;
	}

	std::wstring print(int index) const { return L"layer[" + boost::lexical_cast<std::wstring>(index) + L"]"; }
};

layer::layer() 
{
	impl_ = new implementation();
}
layer::layer(layer&& other) : impl_(std::move(other.impl_)){other.impl_ = nullptr;}
layer::~layer()
{
	delete impl_.fetch_and_store(nullptr);
}
layer& layer::operator=(layer&& other)
{
	impl_ = std::move(other.impl_);	
	other.impl_ = nullptr;
	return *this;
}
void layer::swap(layer& other)
{
	impl_ = other.impl_.compare_and_swap(impl_, other.impl_);
}
void layer::load(int index, const safe_ptr<frame_producer>& frame_producer, bool play_on_load){return impl_->load(index, frame_producer, play_on_load);}	
void layer::preview(int index, const safe_ptr<frame_producer>& frame_producer){return impl_->preview(index, frame_producer);}	
void layer::play(int index){impl_->play(index);}
void layer::pause(int index){impl_->pause(index);}
void layer::stop(int index){impl_->stop(index);}
void layer::clear(int index){impl_->clear(index);}
safe_ptr<draw_frame> layer::receive(int index) {return impl_->receive(index);}
safe_ptr<frame_producer> layer::foreground() const { return impl_->foreground_;}
safe_ptr<frame_producer> layer::background() const { return impl_->background_;}
}}