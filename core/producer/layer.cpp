#include "../stdafx.h"

#include "layer.h"

#include "../producer/frame_producer.h"

#include "../format/video_format.h"

namespace caspar { namespace core {

struct layer::implementation
{		
	implementation() : foreground_(nullptr), background_(nullptr), last_frame_(nullptr) {}
	
	void load(const frame_producer_ptr& frame_producer, load_option::type option)
	{
		if(frame_producer == nullptr) 
			BOOST_THROW_EXCEPTION(null_argument() << arg_name_info("frame_producer"));
			
		background_ = frame_producer;
		if(option == load_option::preview)		
		{
			foreground_ = nullptr;	
			last_frame_ = frame_producer->render_frame();
			if(last_frame_ != nullptr)
				last_frame_->get_audio_data().clear(); // No audio
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
		last_frame_ = nullptr;
	}

	void clear()
	{
		foreground_ = nullptr;
		background_ = nullptr;
		last_frame_ = nullptr;
	}
	
	frame_ptr render_frame()
	{		
		if(!foreground_ || is_paused_)
			return last_frame_;

		try
		{
			last_frame_ = foreground_->render_frame();

			if(last_frame_ == nullptr)
			{
				CASPAR_LOG(info) << L"EOF: " << foreground_->print();
				auto following = foreground_->get_following_producer();
				following->set_leading_producer(foreground_);
				foreground_ = following;
				last_frame_ = render_frame();
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			CASPAR_LOG(warning) << L"Removed " << (foreground_ ? foreground_->print() : L"null producer") << L" from layer.";
			foreground_ = nullptr;
			last_frame_ = nullptr;
		}

		return last_frame_;
	}	
		
	tbb::atomic<bool>	is_paused_;
	frame_ptr			last_frame_;
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
frame_ptr layer::render_frame() {return impl_->render_frame();}
frame_producer_ptr layer::foreground() const { return impl_->foreground_;}
frame_producer_ptr layer::background() const { return impl_->background_;}
}}