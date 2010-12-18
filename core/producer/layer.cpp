#include "../stdafx.h"

#include "layer.h"

#include "../processor/draw_frame.h"
#include "../producer/frame_producer.h"

#include "../format/video_format.h"

namespace caspar { namespace core {

struct layer::implementation
{		
	implementation(size_t index) : foreground_(frame_producer::empty()), background_(frame_producer::empty()), last_frame_(draw_frame::empty()), index_(index) {}
	
	void load(const safe_ptr<frame_producer>& frame_producer, load_option::type option)
	{			
		background_ = frame_producer;
		if(option == load_option::preview)		
		{
			foreground_ = frame_producer::empty();	
			last_frame_ = frame_producer->receive();
		}
		else if(option == load_option::auto_play)
			play();			
	}
	
	void play()
	{			
		background_->set_leading_producer(foreground_);
		foreground_ = background_;
		background_ = frame_producer::empty();
		is_paused_ = false;
		CASPAR_LOG(info) << L"layer[" << index_ << L"] Started: " << foreground_->print();
	}

	void pause()
	{
		is_paused_ = true;
	}

	void stop()
	{
		foreground_ = frame_producer::empty();
		last_frame_ = draw_frame::empty();
	}

	void clear()
	{
		foreground_ = frame_producer::empty();
		background_ = frame_producer::empty();
		last_frame_ = draw_frame::empty();
	}
	
	safe_ptr<draw_frame> receive()
	{		
		if(foreground_ == frame_producer::empty() || is_paused_)
			return last_frame_;

		try
		{
			last_frame_ = foreground_->receive();

			if(last_frame_ == draw_frame::eof())
			{
				CASPAR_LOG(info) << L"layer[" << index_ << L"] Ended:" << foreground_->print();
				auto following = foreground_->get_following_producer();
				following->set_leading_producer(foreground_);
				foreground_ = following;
				if(foreground_ != frame_producer::empty())
					CASPAR_LOG(info) << L"layer[" << index_ << L"] Started:" << foreground_->print();
				last_frame_ = receive();
			}
		}
		catch(...)
		{
			try
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				CASPAR_LOG(warning) << L"layer[" << index_ << L"] Error. Removed " << foreground_->print() << L" from layer.";
				foreground_ = frame_producer::empty();
				last_frame_ = draw_frame::empty();
			}
			catch(...){}
		}

		return last_frame_;
	}	
		
	tbb::atomic<bool>			is_paused_;
	safe_ptr<draw_frame>		last_frame_;
	safe_ptr<frame_producer>	foreground_;
	safe_ptr<frame_producer>	background_;
	size_t						index_;
};

layer::layer(size_t index) : impl_(new implementation(index)){}
layer::layer(layer&& other) : impl_(std::move(other.impl_)){other.impl_ = nullptr;}
layer& layer::operator=(layer&& other)
{
	impl_ = std::move(other.impl_);	
	other.impl_ = nullptr;
	return *this;
}
void layer::load(const safe_ptr<frame_producer>& frame_producer, load_option::type option){return impl_->load(frame_producer, option);}	
void layer::play(){impl_->play();}
void layer::pause(){impl_->pause();}
void layer::stop(){impl_->stop();}
void layer::clear(){impl_->clear();}
bool layer::empty() const { return impl_->foreground_ == frame_producer::empty() && impl_->background_ == frame_producer::empty();}
safe_ptr<draw_frame> layer::receive() {return impl_->receive();}
safe_ptr<frame_producer> layer::foreground() const { return impl_->foreground_;}
safe_ptr<frame_producer> layer::background() const { return impl_->background_;}
}}