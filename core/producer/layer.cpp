#include "../stdafx.h"

#include "layer.h"
#include "frame_producer.h"

#include <common/concurrency/executor.h>
#include <common/exception/exceptions.h>
#include <common/utility/assert.h>
#include <common/utility/printer.h>

#include "../producer/frame/basic_frame.h"
#include "../producer/frame/audio_transform.h"

#include <tbb/spin_mutex.h>

namespace caspar { namespace core {

class frame_producer_remover
{
	executor executor_;

	void do_remove(safe_ptr<frame_producer>& producer)
	{
		auto name = producer->print();
		producer = frame_producer::empty();
		CASPAR_LOG(info) << name << L" Removed.";
	}

public:

	frame_producer_remover() : executor_(L"frame_producer_remover")
	{
		executor_.start();
	}

	void remove(safe_ptr<frame_producer>&& producer)
	{
		if(producer != frame_producer::empty() && !producer.unique())
			CASPAR_LOG(debug) << producer->print() << L" was not destroyed on dedicated destruction thread.";
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
	safe_ptr<basic_frame>		last_frame_;
	bool						is_paused_;
public:
	implementation(int index, const printer& parent_printer) 
		: parent_printer_(parent_printer)
		, index_(index)
		, foreground_(frame_producer::empty())
		, background_(frame_producer::empty())
		, last_frame_(basic_frame::empty())
		, is_paused_(false){}
	
	void load(const safe_ptr<frame_producer>& frame_producer, bool play_on_load, bool preview)
	{		
		background_ = frame_producer;
		is_paused_ = false;

		if(preview)
		{
			play();
			receive();
			pause();
		}

		if(play_on_load)
			play();				
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
		last_frame_ = basic_frame::empty();
		foreground_ = frame_producer::empty();
	}
		
	safe_ptr<basic_frame> receive()
	{		
		if(is_paused_)
		{
			last_frame_->get_audio_transform().set_gain(0.0);
			return last_frame_;
		}

		try
		{
			last_frame_ = foreground_->receive(); 
			if(last_frame_ == basic_frame::eof())
			{
				CASPAR_VERIFY(foreground_ != frame_producer::empty());

				auto following = foreground_->get_following_producer();
				following->set_leading_producer(foreground_);
				following->set_parent_printer([=]{return print();});
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
		tbb::spin_mutex::scoped_lock lock(printer_mutex_); // Child-producers may call print asynchronously to the producer thread.
		return (parent_printer_ ? parent_printer_() + L"/" : L"") + L"layer[" + boost::lexical_cast<std::wstring>(index_) + L"]";
	}
};

layer::layer(int index, const printer& parent_printer) : impl_(new implementation(index, parent_printer)){}
layer::layer(layer&& other) : impl_(std::move(other.impl_)){}
layer& layer::operator=(layer&& other)
{
	impl_ = std::move(other.impl_);
	return *this;
}
void layer::swap(layer& other)
{
	impl_.swap(other.impl_);
	// Printer state is not swapped.
	tbb::spin_mutex::scoped_lock lock(impl_->printer_mutex_);
	std::swap(impl_->parent_printer_, other.impl_->parent_printer_);
	std::swap(impl_->index_, other.impl_->index_);
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