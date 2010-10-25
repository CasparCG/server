#include "../stdafx.h"

#include "layer.h"

#include "../producer/frame_producer.h"

#include "../frame/frame_format.h"

namespace caspar { namespace renderer {

struct layer::implementation
{		
	implementation() : preview_frame_(nullptr), active_(nullptr), background_(nullptr) {}
	
	void load(const frame_producer_ptr& frame_producer, load_option option)
	{
		if(frame_producer == nullptr) 
			BOOST_THROW_EXCEPTION(null_argument() << arg_name_info("frame_producer"));
			
		if(option == load_option::preview)		
		{
			preview_frame_ = frame_producer->get_frame();
			if(preview_frame_ != nullptr)
				preview_frame_->audio_data().clear(); // No audio
			active_ = nullptr;	
			background_ = frame_producer;
		}
		else if(option == load_option::auto_play)
		{
			background_ = frame_producer;
			play();		
		}
		else
			background_ = frame_producer;
	}
	
	void play()
	{			
		if(background_ == nullptr)
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("No background clip to play."));

		background_->set_leading_producer(active_);
		active_ = background_;
		background_ = nullptr;
		preview_frame_ = nullptr;
	}

	void stop()
	{
		active_ = nullptr;
		preview_frame_ = nullptr;
	}

	void clear()
	{
		active_ = nullptr;
		background_ = nullptr;
	}
	
	gpu_frame_ptr get_frame()
	{		
		if(!active_)
			return preview_frame_;

		gpu_frame_ptr frame;
		try
		{
			frame = active_->get_frame();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			active_ = nullptr;
			CASPAR_LOG(warning) << "Removed producer from layer.";
		}

		if(frame == nullptr)
		{
			active_ = active_->get_following_producer();
			frame = get_frame();
		}
		return frame;
	}	
			
	gpu_frame_ptr preview_frame_;
	frame_producer_ptr active_;
	frame_producer_ptr background_;
};

layer::layer() : impl_(new implementation()){}
layer::layer(layer&& other) : impl_(std::move(other.impl_)){other.impl_ = nullptr;}
layer::layer(const layer& other) : impl_(new implementation(*other.impl_)) {}
layer& layer::operator=(layer&& other)
{
	impl_ = std::move(other.impl_);	
	other.impl_ = nullptr;
	return *this;
}
layer& layer::operator=(const layer& other)
{
	layer temp(other);
	impl_.swap(temp.impl_);
	return *this;
}
void layer::load(const frame_producer_ptr& frame_producer, load_option option){return impl_->load(frame_producer, option);}	
void layer::play(){impl_->play();}
void layer::stop(){impl_->stop();}
void layer::clear(){impl_->clear();}
gpu_frame_ptr layer::get_frame() {return impl_->get_frame();}
frame_producer_ptr layer::active() const { return impl_->active_;}
frame_producer_ptr layer::background() const { return impl_->background_;}
}}