#include "../stdafx.h"

#include "layer.h"

#include "../processor/producer_frame.h"
#include "../producer/frame_producer.h"

#include "../format/video_format.h"

namespace caspar { namespace core {

struct layer::implementation
{		
	implementation() : foreground_(nullptr), background_(nullptr), last_frame_(producer_frame::empty()) {}
	
	void load(const frame_producer_ptr& frame_producer, load_option::type option)
	{			
		background_ = frame_producer;
		if(option == load_option::preview)		
		{
			foreground_ = nullptr;	
			last_frame_ = frame_producer->receive();
		}
		else if(option == load_option::auto_play)
			play();			
	}
	
	void play()
	{			
		if(background_ != nullptr)
		{
			background_->set_leading_producer(foreground_);
			foreground_ = std::move(background_);
		}

		is_paused_ = false;
	}

	void pause()
	{
		is_paused_ = true;
	}

	void stop()
	{
		foreground_ = nullptr;
		last_frame_ = producer_frame::empty();
	}

	void clear()
	{
		foreground_ = nullptr;
		background_ = nullptr;
		last_frame_ = producer_frame::empty();
	}
	
	producer_frame receive()
	{		
		if(!foreground_ || is_paused_)
			return last_frame_;

		try
		{
			last_frame_ = foreground_->receive();

			if(last_frame_ == producer_frame::eof() && foreground_->get_following_producer())
			{
				CASPAR_LOG(info) << L"EOF: " << foreground_->print();
				auto following = foreground_->get_following_producer();
				following->set_leading_producer(foreground_);
				foreground_ = following;
				last_frame_ = receive();
			}
		}
		catch(...)
		{
			try
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				CASPAR_LOG(warning) << L"Removed " << (foreground_ ? foreground_->print() : L"empty") << L" from layer.";
				foreground_ = nullptr;
				last_frame_ = producer_frame::empty();
			}
			catch(...){}
		}

		return last_frame_;
	}	
		
	tbb::atomic<bool>	is_paused_;
	producer_frame		last_frame_;
	frame_producer_ptr	foreground_;
	frame_producer_ptr	background_;
};

layer::layer() : impl_(new implementation()){}
layer::layer(layer&& other) : impl_(std::move(other.impl_)){other.impl_ = nullptr;}
layer& layer::operator=(layer&& other)
{
	impl_ = std::move(other.impl_);	
	other.impl_ = nullptr;
	return *this;
}
void layer::load(const frame_producer_ptr& frame_producer, load_option::type option){return impl_->load(frame_producer, option);}	
void layer::play(){impl_->play();}
void layer::pause(){impl_->pause();}
void layer::stop(){impl_->stop();}
void layer::clear(){impl_->clear();}
producer_frame layer::receive() {return impl_->receive();}
frame_producer_ptr layer::foreground() const { return impl_->foreground_;}
frame_producer_ptr layer::background() const { return impl_->background_;}
}}