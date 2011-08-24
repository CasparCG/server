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

#include "../video_channel_context.h"

#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>

#include <common/concurrency/executor.h>
#include <common/utility/move_on_copy.h>

#include <boost/foreach.hpp>

#include <tbb/parallel_for_each.h>

#include <map>
#include <set>

namespace caspar { namespace core {

struct stage::implementation : boost::noncopyable
{		
	std::map<int, layer>						layers_;	
	video_channel_context&						channel_;
public:
	implementation(video_channel_context& video_channel)  
		: channel_(video_channel)
	{
	}
						
	std::map<int, safe_ptr<basic_frame>> execute()
	{		
		try
		{
			std::map<int, safe_ptr<basic_frame>> frames;
		
			BOOST_FOREACH(auto& layer, layers_)			
				frames[layer.first] = basic_frame::empty();	

			tbb::parallel_for_each(layers_.begin(), layers_.end(), [&](std::map<int, layer>::value_type& layer) 
			{
				frames[layer.first] = layer.second.receive();	
			});

			return frames;
		}
		catch(...)
		{
			CASPAR_LOG(error) << L"[stage] Error detected";
			throw;
		}		
	}

	void load(int index, const safe_ptr<frame_producer>& producer, bool preview, int auto_play_delta)
	{
		channel_.execution().invoke([&]
		{
			layers_[index].load(create_destroy_producer_proxy(channel_.destruction(), producer), preview, auto_play_delta);
		}, high_priority);
	}

	void pause(int index)
	{		
		channel_.execution().invoke([&]
		{
			layers_[index].pause();
		}, high_priority);
	}

	void play(int index)
	{		
		channel_.execution().invoke([&]
		{
			layers_[index].play();
		}, high_priority);
	}

	void stop(int index)
	{		
		channel_.execution().invoke([&]
		{
			layers_[index].stop();
		}, high_priority);
	}

	void clear(int index)
	{
		channel_.execution().invoke([&]
		{
			layers_.erase(index);
		}, high_priority);
	}
		
	void clear()
	{
		channel_.execution().invoke([&]
		{
			layers_.clear();
		}, high_priority);
	}	
	
	void swap_layer(int index, size_t other_index)
	{
		channel_.execution().invoke([&]
		{
			std::swap(layers_[index], layers_[other_index]);
		}, high_priority);
	}

	void swap_layer(int index, size_t other_index, stage& other)
	{
		if(other.impl_.get() == this)
			swap_layer(index, other_index);
		else
		{
			auto func = [&]
			{
				std::swap(layers_[index], other.impl_->layers_[other_index]);
			};		
			channel_.execution().invoke([&]{other.impl_->channel_.execution().invoke(func, high_priority);}, high_priority);
		}
	}

	void swap(stage& other)
	{
		if(other.impl_.get() == this)
			return;
		
		auto func = [&]
		{
			std::swap(layers_, other.impl_->layers_);
		};		
		channel_.execution().invoke([&]{other.impl_->channel_.execution().invoke(func, high_priority);}, high_priority);
	}

	layer_status get_status(int index)
	{		
		return channel_.execution().invoke([&]
		{
			return layers_[index].status();
		}, high_priority );
	}
	
	safe_ptr<frame_producer> foreground(int index)
	{
		return channel_.execution().invoke([=]{return layers_[index].foreground();}, high_priority);
	}
	
	safe_ptr<frame_producer> background(int index)
	{
		return channel_.execution().invoke([=]{return layers_[index].background();}, high_priority);
	}

	std::wstring print() const
	{
		return L"stage [" + boost::lexical_cast<std::wstring>(channel_.index()) + L"]";
	}

};

stage::stage(video_channel_context& video_channel) : impl_(new implementation(video_channel)){}
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
std::map<int, safe_ptr<basic_frame>> stage::execute(){return impl_->execute();}
}}