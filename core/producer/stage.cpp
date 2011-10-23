/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include "../StdAfx.h"

#include "stage.h"

#include "layer.h"

#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>

#include <common/concurrency/executor.h>
#include <common/utility/move_on_copy.h>

#include <boost/foreach.hpp>

#include <agents.h>
#include <ppl.h>

#include <map>
#include <set>

using namespace Concurrency;

namespace caspar { namespace core {

struct stage::implementation : public agent, public boost::noncopyable
{		
	overwrite_buffer<bool>						is_running_;
	Concurrency::critical_section				mutex_;
	stage::target_t&							target_;
	std::map<int, layer>						layers_;	
	safe_ptr<semaphore>							semaphore_;
public:
	implementation(stage::target_t& target, const safe_ptr<semaphore>& semaphore)  
		: target_(target)
		, semaphore_(semaphore)
	{
		start();
	}

	~implementation()
	{
		send(is_running_, false);
		semaphore_->release();
		agent::wait(this);
	}

	virtual void run()
	{
		send(is_running_, true);
		while(is_running_.value())
		{
			try
			{
				std::map<int, safe_ptr<basic_frame>> frames;
				{
					critical_section::scoped_lock lock(mutex_);
		
					BOOST_FOREACH(auto& layer, layers_)			
						frames[layer.first] = basic_frame::empty();	

					Concurrency::parallel_for_each(layers_.begin(), layers_.end(), [&](std::map<int, layer>::value_type& layer) 
					{
						frames[layer.first] = layer.second.receive();	
					});
				}

				send(target_, make_safe<message<decltype(frames)>>(frames, token(semaphore_)));
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}	
		}

		send(is_running_, false);
		done();
	}
				
	void load(int index, const safe_ptr<frame_producer>& producer, bool preview, int auto_play_delta)
	{
		critical_section::scoped_lock lock(mutex_);
		layers_[index].load(create_destroy_producer_proxy(producer), preview, auto_play_delta);
	}

	void pause(int index)
	{		
		critical_section::scoped_lock lock(mutex_);
		layers_[index].pause();
	}

	void play(int index)
	{		
		critical_section::scoped_lock lock(mutex_);
		layers_[index].play();
	}

	void stop(int index)
	{		
		critical_section::scoped_lock lock(mutex_);
		layers_[index].stop();
	}

	void clear(int index)
	{
		critical_section::scoped_lock lock(mutex_);
		layers_.erase(index);
	}
		
	void clear()
	{
		critical_section::scoped_lock lock(mutex_);
		layers_.clear();
	}	
	
	void swap_layer(int index, size_t other_index)
	{
		critical_section::scoped_lock lock(mutex_);
		std::swap(layers_[index], layers_[other_index]);
	}

	void swap_layer(int index, size_t other_index, stage& other)
	{
		if(other.impl_.get() == this)
			swap_layer(index, other_index);
		else
		{			
			critical_section::scoped_lock lock1(mutex_);
			critical_section::scoped_lock lock2(other.impl_->mutex_);
			std::swap(layers_[index], other.impl_->layers_[other_index]);
		}
	}

	void swap(stage& other)
	{
		if(other.impl_.get() == this)
			return;
		
		critical_section::scoped_lock lock1(mutex_);
		critical_section::scoped_lock lock2(other.impl_->mutex_);
		std::swap(layers_, other.impl_->layers_);
	}

	layer_status get_status(int index)
	{		
		critical_section::scoped_lock lock(mutex_);
		return layers_[index].status();
	}
	
	safe_ptr<frame_producer> foreground(int index)
	{
		critical_section::scoped_lock lock(mutex_);
		return layers_[index].foreground();
	}
	
	safe_ptr<frame_producer> background(int index)
	{
		critical_section::scoped_lock lock(mutex_);
		return layers_[index].background();
	}

	std::wstring print() const
	{
		// C-TODO
		return L"stage []";// + boost::lexical_cast<std::wstring>(channel_.index()) + L"]";
	}

};

stage::stage(target_t& target, const safe_ptr<semaphore>& semaphore) : impl_(new implementation(target, semaphore)){}
void stage::swap(stage& other){impl_->swap(other);}
void stage::load(int index, const safe_ptr<frame_producer>& producer, bool preview, int auto_play_delta){impl_->load(index, producer, preview, auto_play_delta);}
void stage::pause(int index){impl_->pause(index);}
void stage::play(int index){impl_->play(index);}
void stage::stop(int index){impl_->stop(index);}
void stage::clear(int index){impl_->clear(index);}
void stage::clear(){impl_->clear();}
void stage::swap_layer(int index, size_t other_index){impl_->swap_layer(index, other_index);}
void stage::swap_layer(int index, size_t other_index, stage& other){impl_->swap_layer(index, other_index, other);}
layer_status stage::get_status(int index){return impl_->get_status(index);}
safe_ptr<frame_producer> stage::foreground(size_t index) {return impl_->foreground(index);}
safe_ptr<frame_producer> stage::background(size_t index) {return impl_->background(index);}
}}