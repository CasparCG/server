/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "../StdAfx.h"

#include "stage.h"

#include "layer.h"

#include "frame/basic_frame.h"
#include "frame/frame_factory.h"

#include <common/concurrency/executor.h>

#include <core/producer/frame/frame_transform.h>

#include <boost/foreach.hpp>
#include <boost/timer.hpp>

#include <tbb/parallel_for_each.h>
#include <tbb/concurrent_unordered_map.h>

#include <boost/property_tree/ptree.hpp>

#include <map>

namespace caspar { namespace core {
	
template<typename T>
class tweened_transform
{
	T source_;
	T dest_;
	int duration_;
	int time_;
	tweener_t tweener_;
public:	
	tweened_transform()
		: duration_(0)
		, time_(0)
		, tweener_(get_tweener(L"linear")){}
	tweened_transform(const T& source, const T& dest, int duration, const std::wstring& tween = L"linear")
		: source_(source)
		, dest_(dest)
		, duration_(duration)
		, time_(0)
		, tweener_(get_tweener(tween)){}
	
	const T& source() const
	{
		return source_;
	}
	
	const T& dest() const
	{
		return dest_;
	}

	T fetch()
	{
		return time_ == duration_ ? dest_ : tween(static_cast<double>(time_), source_, dest_, static_cast<double>(duration_), tweener_);
	}

	T fetch_and_tick(int num)
	{						
		time_ = std::min(time_+num, duration_);
		return fetch();
	}
};

struct stage::implementation : public std::enable_shared_from_this<implementation>
							 , boost::noncopyable
{		
	safe_ptr<diagnostics::graph>												 graph_;
	safe_ptr<stage::target_t>													 target_;
	video_format_desc															 format_desc_;
																				 
	boost::timer																 produce_timer_;
	boost::timer																 tick_timer_;
																				 
	std::map<int, layer>														 layers_;	
	tbb::concurrent_unordered_map<int, tweened_transform<core::frame_transform>> transforms_;	

	executor						executor_;
public:
	implementation(const safe_ptr<diagnostics::graph>& graph, const safe_ptr<stage::target_t>& target, const video_format_desc& format_desc)  
		: graph_(graph)
		, format_desc_(format_desc)
		, target_(target)
		, executor_(L"stage")
	{
		graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f, 0.8));	
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
				auto transform = transforms_[layer.first].fetch_and_tick(1);

				int hints = frame_producer::NO_HINT;
				if(format_desc_.field_mode != field_mode::progressive)
				{
					hints |= std::abs(transform.fill_scale[1]  - 1.0) > 0.0001 ? frame_producer::DEINTERLACE_HINT : frame_producer::NO_HINT;
					hints |= std::abs(transform.fill_translation[1]) > 0.0001 ? frame_producer::DEINTERLACE_HINT : frame_producer::NO_HINT;
				}

				if(transform.is_key)
					hints |= frame_producer::ALPHA_HINT;

				auto frame = layer.second.receive(hints);	
				
				auto frame1 = make_safe<core::basic_frame>(frame);
				frame1->get_frame_transform() = transform;

				if(format_desc_.field_mode != core::field_mode::progressive)
				{				
					auto frame2 = make_safe<core::basic_frame>(frame);
					frame2->get_frame_transform() = transforms_[layer.first].fetch_and_tick(1);
					frame1 = core::basic_frame::interlace(frame1, frame2, format_desc_.field_mode);
				}

				frames[layer.first] = frame1;
			});
			
			graph_->set_value("produce-time", produce_timer_.elapsed()*format_desc_.fps*0.5);
			
			std::shared_ptr<void> ticket(nullptr, [self](void*)
			{
				auto self2 = self.lock();
				if(self2)				
					self2->executor_.begin_invoke([=]{tick(self);});				
			});

			target_->send(std::make_pair(frames, ticket));

			graph_->set_value("tick-time", tick_timer_.elapsed()*format_desc_.fps*0.5);
			tick_timer_.restart();
		}
		catch(...)
		{
			layers_.clear();
			CASPAR_LOG_CURRENT_EXCEPTION();
		}		
	}
		
	void set_transform(int index, const frame_transform& transform, unsigned int mix_duration, const std::wstring& tween)
	{
		executor_.begin_invoke([=]
		{
			auto src = transforms_[index].fetch();
			auto dst = transform;
			transforms_[index] = tweened_transform<frame_transform>(src, dst, mix_duration, tween);
		}, high_priority);
	}
					
	void apply_transforms(const std::vector<std::tuple<int, stage::transform_func_t, unsigned int, std::wstring>>& transforms)
	{
		executor_.begin_invoke([=]
		{
			BOOST_FOREACH(auto& transform, transforms)
			{
				auto& tween = transforms_[std::get<0>(transform)];
				auto src = tween.fetch();
				auto dst = std::get<1>(transform)(tween.dest());
				transforms_[std::get<0>(transform)] = tweened_transform<frame_transform>(src, dst, std::get<2>(transform), std::get<3>(transform));
			}
		}, high_priority);
	}
						
	void apply_transform(int index, const stage::transform_func_t& transform, unsigned int mix_duration, const std::wstring& tween)
	{
		executor_.begin_invoke([=]
		{
			auto src = transforms_[index].fetch();
			auto dst = transform(src);
			transforms_[index] = tweened_transform<frame_transform>(src, dst, mix_duration, tween);
		}, high_priority);
	}

	void clear_transforms(int index)
	{
		executor_.begin_invoke([=]
		{
			transforms_[index] = tweened_transform<core::frame_transform>();
		}, high_priority);
	}

	void clear_transforms()
	{
		executor_.begin_invoke([=]
		{
			transforms_.clear();
		}, high_priority);
	}
		
	void load(int index, const safe_ptr<frame_producer>& producer, bool preview, int auto_play_delta)
	{
		executor_.begin_invoke([=]
		{
			layers_[index].load(producer, preview, auto_play_delta);
		}, high_priority);
	}

	void pause(int index)
	{		
		executor_.begin_invoke([=]
		{
			layers_[index].pause();
		}, high_priority);
	}

	void play(int index)
	{		
		executor_.begin_invoke([=]
		{
			layers_[index].play();
		}, high_priority);
	}

	void stop(int index)
	{		
		executor_.begin_invoke([=]
		{
			layers_[index].stop();
		}, high_priority);
	}

	void clear(int index)
	{
		executor_.begin_invoke([=]
		{
			layers_.erase(index);
		}, high_priority);
	}
		
	void clear()
	{
		executor_.begin_invoke([=]
		{
			layers_.clear();
		}, high_priority);
	}	
	
	boost::unique_future<std::wstring> call(int index, bool foreground, const std::wstring& param)
	{
		return std::move(*executor_.invoke([=]
		{
			return std::make_shared<boost::unique_future<std::wstring>>(std::move(layers_[index].call(foreground, param)));
		}, high_priority));
	}
	
	void swap_layers(const safe_ptr<stage>& other)
	{
		if(other->impl_.get() == this)
			return;
		
		auto func = [=]
		{
			std::swap(layers_, other->impl_->layers_);
		};		
		executor_.begin_invoke([=]
		{
			other->impl_->executor_.invoke(func, high_priority);
		}, high_priority);
	}

	void swap_layer(int index, size_t other_index)
	{
		executor_.begin_invoke([=]
		{
			std::swap(layers_[index], layers_[other_index]);
		}, high_priority);
	}

	void swap_layer(int index, size_t other_index, const safe_ptr<stage>& other)
	{
		if(other->impl_.get() == this)
			swap_layer(index, other_index);
		else
		{
			auto func = [=]
			{
				std::swap(layers_[index], other->impl_->layers_[other_index]);
			};		
			executor_.begin_invoke([=]
			{
				other->impl_->executor_.invoke(func, high_priority);
			}, high_priority);
		}
	}
		
	boost::unique_future<safe_ptr<frame_producer>> foreground(int index)
	{
		return executor_.begin_invoke([=]
		{
			return layers_[index].foreground();
		}, high_priority);
	}
	
	boost::unique_future<safe_ptr<frame_producer>> background(int index)
	{
		return executor_.begin_invoke([=]
		{
			return layers_[index].background();
		}, high_priority);
	}
	
	void set_video_format_desc(const video_format_desc& format_desc)
	{
		executor_.begin_invoke([=]
		{
			format_desc_ = format_desc;
		}, high_priority);
	}

	boost::unique_future<boost::property_tree::wptree> info()
	{
		return std::move(executor_.begin_invoke([this]() -> boost::property_tree::wptree
		{
			boost::property_tree::wptree info;
			BOOST_FOREACH(auto& layer, layers_)			
				info.add_child(L"layers.layer", layer.second.info())
					.add(L"index", layer.first);	
			return info;
		}, high_priority));
	}

	boost::unique_future<boost::property_tree::wptree> info(int index)
	{
		return std::move(executor_.begin_invoke([=]() -> boost::property_tree::wptree
		{
			return layers_[index].info();
		}, high_priority));
	}
};

stage::stage(const safe_ptr<diagnostics::graph>& graph, const safe_ptr<target_t>& target, const video_format_desc& format_desc) : impl_(new implementation(graph, target, format_desc)){}
void stage::apply_transforms(const std::vector<stage::transform_tuple_t>& transforms){impl_->apply_transforms(transforms);}
void stage::apply_transform(int index, const std::function<core::frame_transform(core::frame_transform)>& transform, unsigned int mix_duration, const std::wstring& tween){impl_->apply_transform(index, transform, mix_duration, tween);}
void stage::clear_transforms(int index){impl_->clear_transforms(index);}
void stage::clear_transforms(){impl_->clear_transforms();}
void stage::spawn_token(){impl_->spawn_token();}
void stage::load(int index, const safe_ptr<frame_producer>& producer, bool preview, int auto_play_delta){impl_->load(index, producer, preview, auto_play_delta);}
void stage::pause(int index){impl_->pause(index);}
void stage::play(int index){impl_->play(index);}
void stage::stop(int index){impl_->stop(index);}
void stage::clear(int index){impl_->clear(index);}
void stage::clear(){impl_->clear();}
void stage::swap_layers(const safe_ptr<stage>& other){impl_->swap_layers(other);}
void stage::swap_layer(int index, size_t other_index){impl_->swap_layer(index, other_index);}
void stage::swap_layer(int index, size_t other_index, const safe_ptr<stage>& other){impl_->swap_layer(index, other_index, other);}
boost::unique_future<safe_ptr<frame_producer>> stage::foreground(int index) {return impl_->foreground(index);}
boost::unique_future<safe_ptr<frame_producer>> stage::background(int index) {return impl_->background(index);}
boost::unique_future<std::wstring> stage::call(int index, bool foreground, const std::wstring& param){return impl_->call(index, foreground, param);}
void stage::set_video_format_desc(const video_format_desc& format_desc){impl_->set_video_format_desc(format_desc);}
boost::unique_future<boost::property_tree::wptree> stage::info() const{return impl_->info();}
boost::unique_future<boost::property_tree::wptree> stage::info(int index) const{return impl_->info(index);}
}}