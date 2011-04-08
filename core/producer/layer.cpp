#include "../stdafx.h"

#include "layer.h"
#include "frame_producer.h"

#include "../producer/frame/basic_frame.h"
#include "../producer/frame/audio_transform.h"

namespace caspar { namespace core {
	
struct layer::implementation : boost::noncopyable
{				
	int							index_;	
	safe_ptr<frame_producer>	foreground_;
	safe_ptr<frame_producer>	background_;
	safe_ptr<basic_frame>		last_frame_;
	bool						is_paused_;
public:
	implementation(int index) 
		: index_(index)
		, foreground_(frame_producer::empty())
		, background_(frame_producer::empty())
		, last_frame_(basic_frame::empty())
		, is_paused_(false){}
	
	void pause() 
	{
		is_paused_ = true; 
	}

	void resume()
	{
		if(is_paused_)
			CASPAR_LOG(info) << foreground_->print() << L" Resumed.";
		is_paused_ = false;
	}

	void load(const safe_ptr<frame_producer>& producer, bool play_on_load, bool preview)
	{		
		background_ = producer;

		if(play_on_load)
			play();		
		else if(preview)
		{
			play();
			receive();
			pause();
		}

		CASPAR_LOG(info) << producer->print() << L" Loaded.";
	}
	
	void play()
	{			
		if(background_ != frame_producer::empty())
		{
			background_->set_leading_producer(foreground_);
			foreground_ = background_;
			background_ = frame_producer::empty();
			CASPAR_LOG(info) << foreground_->print() << L" Active.";
		}
		resume();
	}
	
	void stop()
	{
		pause();
		last_frame_ = basic_frame::empty();
		foreground_ = frame_producer::empty();
	}
		
	safe_ptr<basic_frame> receive()
	{		
		if(is_paused_)		
			last_frame_->get_audio_transform().set_has_audio(false);		
		else
			last_frame_ = core::receive(foreground_);

		return last_frame_;
	}
		
	std::wstring print() const
	{
		return L"layer[" + boost::lexical_cast<std::wstring>(index_) + L"]";
	}
};

layer::layer(int index) : impl_(new implementation(index)){}
layer::layer(layer&& other) : impl_(std::move(other.impl_)){}
layer& layer::operator=(layer&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}
void layer::swap(layer& other)
{	
	std::swap(impl_->foreground_, other.impl_->foreground_);
	std::swap(impl_->background_, other.impl_->background_);
	std::swap(impl_->last_frame_, other.impl_->last_frame_);
	std::swap(impl_->is_paused_	, other.impl_->is_paused_ );
}
void layer::load(const safe_ptr<frame_producer>& frame_producer, bool play_on_load, bool preview){return impl_->load(frame_producer, play_on_load, preview);}	
void layer::play(){impl_->play();}
void layer::pause(){impl_->pause();}
void layer::stop(){impl_->stop();}
safe_ptr<basic_frame> layer::receive() {return impl_->receive();}
safe_ptr<frame_producer> layer::foreground() const { return impl_->foreground_;}
safe_ptr<frame_producer> layer::background() const { return impl_->background_;}
std::wstring layer::print() const { return impl_->print();}
}}