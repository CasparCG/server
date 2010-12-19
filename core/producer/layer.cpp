#include "../stdafx.h"

#include "layer.h"

#include "../processor/draw_frame.h"
#include "../producer/frame_producer.h"

#include "../format/video_format.h"

namespace caspar { namespace core {

struct layer::implementation
{		
	implementation(size_t index) : foreground_(frame_producer::empty()), background_(frame_producer::empty()), last_frame_(draw_frame::empty()), index_(index) {}
	
	void load(const safe_ptr<frame_producer>& frame_producer, bool autoplay)
	{			
		background_ = frame_producer;
		CASPAR_LOG(info) << print() << " " << foreground_->print() << " => background";
		if(autoplay)
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
			CASPAR_LOG(warning) << print() << L" empty => background{" << background_->print() << "}";
			background_ = frame_producer::empty();
		}
	}
	
	void play()
	{			
		background_->set_leading_producer(foreground_);
		foreground_ = background_;
		background_ = frame_producer::empty();
		is_paused_ = false;
		CASPAR_LOG(info) << print() << L" background{" << foreground_->print() << "} => foreground";
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
				auto following = foreground_->get_following_producer();
				following->set_leading_producer(foreground_);
				foreground_ = following;
				if(foreground_ != frame_producer::empty())
					CASPAR_LOG(info) << print() << L" [EOF] following{" << foreground_->print() << "} => foreground{" << foreground_->print() << "}";
				else
					CASPAR_LOG(info) << print() << L" [EOF] empty => foreground{" << foreground_->print() << "}";
				last_frame_ = receive();
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			CASPAR_LOG(warning) << print() << L" empty -> foreground{" << foreground_->print() << "]";
			foreground_ = frame_producer::empty();
			last_frame_ = draw_frame::empty();
		}

		return last_frame_;
	}

	std::wstring print() const { return L"layer[" + boost::lexical_cast<std::wstring>(index_) + L"]"; }
				
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
void layer::load(const safe_ptr<frame_producer>& frame_producer, bool autoplay){return impl_->load(frame_producer, autoplay);}	
void layer::preview(const safe_ptr<frame_producer>& frame_producer){return impl_->preview(frame_producer);}	
void layer::play(){impl_->play();}
void layer::pause(){impl_->pause();}
void layer::stop(){impl_->stop();}
void layer::clear(){impl_->clear();}
bool layer::empty() const { return impl_->foreground_ == frame_producer::empty() && impl_->background_ == frame_producer::empty();}
safe_ptr<draw_frame> layer::receive() {return impl_->receive();}
safe_ptr<frame_producer> layer::foreground() const { return impl_->foreground_;}
safe_ptr<frame_producer> layer::background() const { return impl_->background_;}
}}