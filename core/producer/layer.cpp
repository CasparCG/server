#include "../stdafx.h"

#include "layer.h"
#include "frame_producer.h"

#include "../video_format.h"

#include <common/concurrency/executor.h>
#include <common/utility/assert.h>

#include <mixer/frame/draw_frame.h>
#include <mixer/image/image_mixer.h>
#include <mixer/audio/audio_mixer.h>
#include <mixer/audio/audio_transform.h>

namespace caspar { namespace core {

class frame_producer_remover
{
	executor executor_;
	tbb::atomic<int> count_;

	void do_remove(safe_ptr<frame_producer>& producer)
	{
		producer = frame_producer::empty();
		CASPAR_LOG(info) << L"frame_remover[" + boost::lexical_cast<std::wstring>(--count_) + L"] removed: " << producer->print();
	}
public:

	frame_producer_remover()
	{
		executor_.start();
		count_ = 0;
	}

	void remove(safe_ptr<frame_producer>&& producer)
	{
		CASPAR_ASSERT(producer.unique());
		CASPAR_LOG(info) << L"frame_remover[" + boost::lexical_cast<std::wstring>(++count_) + L"] removing: " << producer->print();
		executor_.begin_invoke(std::bind(&frame_producer_remover::do_remove, this, std::move(producer)));
	}
};

frame_producer_remover g_remover;

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
	
	void load(const safe_ptr<frame_producer>& frame_producer, bool play_on_load)
	{			
		background_ = frame_producer;
		is_paused_ = false;
		if(play_on_load)
			play();		
	}

	void preview(const safe_ptr<frame_producer>& frame_producer)
	{
		load(frame_producer, true);
		receive();
		pause();
	}
	
	void play()
	{			
		if(!is_paused_)			
		{
			background_->set_leading_producer(foreground_);
			foreground_ = background_;
			CASPAR_LOG(info) << foreground_->print() << L" started";
			background_ = frame_producer::empty();
		}
		is_paused_ = false;
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
	}

	void clear()
	{
		stop();
		background_ = frame_producer::empty();
	}
	
	safe_ptr<draw_frame> receive()
	{		
		if(is_paused_)
		{
			last_frame_->get_audio_transform().set_gain(0.0);
			return last_frame_;
		}

		try
		{
			last_frame_ = foreground_->receive(); 
			if(last_frame_ == draw_frame::eof())
			{
				CASPAR_ASSERT(foreground_ != frame_producer::empty());

				auto following = foreground_->get_following_producer();
				following->set_leading_producer(foreground_);
				g_remover.remove(std::move(foreground_));
				foreground_ = following;
				CASPAR_LOG(info) << foreground_->print() << L" started";

				last_frame_ = receive();
			}
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			stop();
		}

		return last_frame_;
	}
};

layer::layer() 
{
	impl_ = new implementation();
}
layer::layer(layer&& other) 
{
	impl_ = other.impl_.compare_and_swap(nullptr, other.impl_);
}
layer::~layer()
{
	delete impl_.fetch_and_store(nullptr);
}
layer& layer::operator=(layer&& other)
{
	impl_ = other.impl_.compare_and_swap(nullptr, other.impl_);
	return *this;
}
void layer::swap(layer& other)
{
	impl_ = other.impl_.compare_and_swap(impl_, other.impl_);
}
void layer::load(const safe_ptr<frame_producer>& frame_producer, bool play_on_load){return impl_->load(frame_producer, play_on_load);}	
void layer::preview(const safe_ptr<frame_producer>& frame_producer){return impl_->preview(frame_producer);}	
void layer::play(){impl_->play();}
void layer::pause(){impl_->pause();}
void layer::stop(){impl_->stop();}
void layer::clear(){impl_->clear();}
safe_ptr<draw_frame> layer::receive() {return impl_->receive();}
safe_ptr<frame_producer> layer::foreground() const { return impl_->foreground_;}
safe_ptr<frame_producer> layer::background() const { return impl_->background_;}
}}