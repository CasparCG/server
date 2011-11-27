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

#include "frame/basic_frame.h"
#include "frame/frame_factory.h"

#include <common/concurrency/executor.h>

#include <boost/foreach.hpp>
#include <boost/timer.hpp>

#include <tbb/parallel_for_each.h>

#include <map>

namespace caspar { namespace core {

struct stage::implementation : public std::enable_shared_from_this<implementation>
							 , boost::noncopyable
{		
	safe_ptr<diagnostics::graph>	graph_;
	safe_ptr<stage::target_t>		target_;
	video_format_desc				format_desc_;

	boost::timer					produce_timer_;
	boost::timer					tick_timer_;

	std::map<int, layer>			layers_;	

	executor						executor_;
public:
	implementation(const safe_ptr<diagnostics::graph>& graph, const safe_ptr<stage::target_t>& target, const video_format_desc& format_desc)  
		: graph_(graph)
		, format_desc_(format_desc)
		, target_(target)
		, executor_(L"stage")
	{
		graph_->add_guide("tick-time", 0.5f);	
		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));	
		graph_->set_color("produce-time", diagnostics::color(0.0f, 1.0f, 0.0f));
	}

	void spawn_token()
	{
		std::weak_ptr<implementation> self = shared_from_this();
		executor_.begin_invoke([=]{tick(self);});
	}
							
	void tick(const std::weak_ptr<implementation>& self)
	{		
		try
		{
			produce_timer_.restart();

			std::map<int, safe_ptr<basic_frame>> frames;
		
			BOOST_FOREACH(auto& layer, layers_)			
				frames[layer.first] = basic_frame::empty();	

			tbb::parallel_for_each(layers_.begin(), layers_.end(), [&](std::map<int, layer>::value_type& layer) 
			{
				frames[layer.first] = layer.second.receive();	
			});
			
			graph_->update_value("produce-time", produce_timer_.elapsed()*format_desc_.fps*0.5);
			
			std::shared_ptr<void> ticket(nullptr, [self](void*)
			{
				auto self2 = self.lock();
				if(self2)				
					self2->executor_.begin_invoke([=]{tick(self);});				
			});

			target_->send(std::make_pair(frames, ticket));

			graph_->update_value("tick-time", tick_timer_.elapsed()*format_desc_.fps*0.5);
			tick_timer_.restart();
		}
		catch(...)
		{
			layers_.clear();
			CASPAR_LOG_CURRENT_EXCEPTION();
		}		
	}

	void load(int index, const safe_ptr<frame_producer>& producer, bool preview, int auto_play_delta)
	{
		executor_.invoke([&]
		{
			layers_[index].load(producer, preview, auto_play_delta);
		}, high_priority);
	}

	void pause(int index)
	{		
		executor_.invoke([&]
		{
			layers_[index].pause();
		}, high_priority);
	}

	void play(int index)
	{		
		executor_.invoke([&]
		{
			layers_[index].play();
		}, high_priority);
	}

	void stop(int index)
	{		
		executor_.invoke([&]
		{
			layers_[index].stop();
		}, high_priority);
	}

	void clear(int index)
	{
		executor_.invoke([&]
		{
			layers_.erase(index);
		}, high_priority);
	}
		
	void clear()
	{
		executor_.invoke([&]
		{
			layers_.clear();
		}, high_priority);
	}	
	
	boost::unique_future<std::wstring> call(int index, bool foreground, const std::wstring& param)
	{
		return std::move(*executor_.invoke([&]() -> std::shared_ptr<boost::unique_future<std::wstring>>
		{
			return std::make_shared<boost::unique_future<std::wstring>>(std::move(layers_[index].call(foreground, param)));
		}, high_priority));
	}

	void swap_layer(int index, size_t other_index)
	{
		executor_.invoke([&]
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
			executor_.invoke([&]{other.impl_->executor_.invoke(func, high_priority);}, high_priority);
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
		executor_.invoke([&]{other.impl_->executor_.invoke(func, high_priority);}, high_priority);
	}

	layer_status get_status(int index)
	{		
		return executor_.invoke([&]
		{
			return layers_[index].status();
		}, high_priority );
	}
	
	safe_ptr<frame_producer> foreground(int index)
	{
		return executor_.invoke([=]{return layers_[index].foreground();}, high_priority);
	}
	
	safe_ptr<frame_producer> background(int index)
	{
		return executor_.invoke([=]{return layers_[index].background();}, high_priority);
	}
	
	void set_video_format_desc(const video_format_desc& format_desc)
	{
		executor_.begin_invoke([=]
		{
			format_desc_ = format_desc;
		}, high_priority );
	}
};

stage::stage(const safe_ptr<diagnostics::graph>& graph, const safe_ptr<target_t>& target, const video_format_desc& format_desc) : impl_(new implementation(graph, target, format_desc)){}
void stage::spawn_token(){impl_->spawn_token();}
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
void stage::set_video_format_desc(const video_format_desc& format_desc){impl_->set_video_format_desc(format_desc);}
boost::unique_future<std::wstring> stage::call(int index, bool foreground, const std::wstring& param){return impl_->call(index, foreground, param);}
}}