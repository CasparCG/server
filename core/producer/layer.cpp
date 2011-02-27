#include "../stdafx.h"

#include "layer.h"
#include "frame_producer.h"

#include "../video_format.h"

#include <common/concurrency/executor.h>
#include <common/utility/assert.h>
#include <common/utility/printable.h>

#include <mixer/frame/draw_frame.h>
#include <mixer/image/image_mixer.h>
#include <mixer/audio/audio_mixer.h>
#include <mixer/audio/audio_transform.h>

#include <tbb/spin_mutex.h>

namespace caspar { namespace core {

class frame_producer_remover
{
	executor executor_;
	tbb::atomic<int> count_;

	void do_remove(safe_ptr<frame_producer>& producer)
	{
		auto name = producer->print();
		producer = frame_producer::empty();
		CASPAR_LOG(info) << name << L" Removed.";
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
		executor_.begin_invoke(std::bind(&frame_producer_remover::do_remove, this, std::move(producer)));
	}
};

frame_producer_remover g_remover;


struct layer::implementation : boost::noncopyable
{				
	mutable tbb::spin_mutex		printer_mutex_;
	printer						parent_printer_;
	int							index_;
	
	safe_ptr<frame_producer>	foreground_;
	safe_ptr<frame_producer>	background_;
	safe_ptr<draw_frame>		last_frame_;
	bool						is_paused_;
public:
	implementation(int index, const printer& parent_printer) 
		: parent_printer_(parent_printer)
		, index_(index)
		, foreground_(frame_producer::empty())
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
			CASPAR_LOG(info) << foreground_->print() << L" Added.";
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
		foreground_ = frame_producer::empty();
		background_ = frame_producer::empty();
		last_frame_ = draw_frame::empty();
		is_paused_ = false;
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
				following->set_parent_printer(boost::bind(&implementation::print, this));
				g_remover.remove(std::move(foreground_));
				foreground_ = following;
				CASPAR_LOG(info) << foreground_->print() << L" Added.";

				last_frame_ = receive();
			}
		}
		catch(...)
		{
			CASPAR_LOG(error) << print() << L" Unhandled Exception: ";
			CASPAR_LOG_CURRENT_EXCEPTION();
			stop();
		}

		return last_frame_;
	}
		
	std::wstring print() const
	{
		std::wstring parent_print = L"";
		{
			tbb::spin_mutex::scoped_lock lock(printer_mutex_);
			if(parent_printer_)
				parent_print = parent_printer_() + L"/";
		}
		return parent_print + L"layer[" + boost::lexical_cast<std::wstring>(index_) + L"]";
	}
};

layer::layer(int index, const printer& parent_printer)
{
	impl_ = new implementation(index, parent_printer);
}
layer::~layer()
{
	if(!impl_)
		return;

	impl_->clear();
	delete impl_.fetch_and_store(nullptr);
}
layer::layer(layer&& other)
{
	impl_ = other.impl_.fetch_and_store(nullptr);
}
layer& layer::operator=(layer&& other)
{
	impl_ = other.impl_.fetch_and_store(nullptr);
	return *this;
}
void layer::swap(layer& other)
{
	impl_ = other.impl_.compare_and_swap(impl_, other.impl_);
	tbb::spin_mutex::scoped_lock lock(other.impl_->printer_mutex_);
	std::swap(impl_->index_, other.impl_->index_);
	std::swap(impl_->parent_printer_, other.impl_->parent_printer_);
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
std::wstring layer::print() const { return impl_->print();}
}}