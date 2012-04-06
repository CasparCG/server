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

#include "../frame/draw_frame.h"
#include "../frame/frame_factory.h"

#include <common/executor.h>
#include <common/future.h>
#include <common/diagnostics/graph.h>

#include <core/frame/frame_transform.h>

#include <boost/foreach.hpp>
#include <boost/timer.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/range/algorithm_ext.hpp>

#include <tbb/parallel_for_each.h>

#include <functional>
#include <map>
#include <vector>

namespace caspar { namespace core {
	
struct stage::impl : public std::enable_shared_from_this<impl>
{				
	spl::shared_ptr<diagnostics::graph>							graph_;
	monitor::basic_subject										event_subject_;
	reactive::basic_subject<std::map<int, class draw_frame>>	frames_subject_;
	std::map<int, layer>										layers_;	
	std::map<int, tweened_transform>							tweens_;	
	executor													executor_;
public:
	impl(spl::shared_ptr<diagnostics::graph> graph) 
		: graph_(std::move(graph))
		, event_subject_("stage")
		, executor_(L"stage")
	{
		graph_->set_color("produce-time", diagnostics::color(0.0f, 1.0f, 0.0f));
	}
		
	std::map<int, draw_frame> operator()(const struct video_format_desc& format_desc)
	{		
		boost::timer frame_timer;

		auto frames = executor_.invoke([=]() -> std::map<int, draw_frame>
		{

			std::map<int, class draw_frame> frames;
			
			try
			{			
				std::vector<int> indices;

				BOOST_FOREACH(auto& layer, layers_)	
				{
					frames[layer.first] = draw_frame::empty();	
					indices.push_back(layer.first);
				}				

				// WORKAROUND: Compiler doesn't seem to like lambda.
				tbb::parallel_for_each(indices.begin(), indices.end(), std::bind(&stage::impl::draw, this, std::placeholders::_1, std::ref(format_desc), std::ref(frames)));
			}
			catch(...)
			{
				layers_.clear();
				CASPAR_LOG_CURRENT_EXCEPTION();
			}	
			

			return frames;
		});
		
		frames_subject_ << frames;
		
		graph_->set_value("produce-time", frame_timer.elapsed()*format_desc.fps*0.5);
		event_subject_ << monitor::event("profiler/time") % frame_timer.elapsed() % (1.0/format_desc.fps);

		return frames;
	}

	void draw(int index, const video_format_desc& format_desc, std::map<int, draw_frame>& frames)
	{
		auto& layer		= layers_[index];
		auto& tween		= tweens_[index];
				
		auto frame  = layer.receive(format_desc);					
		auto frame1 = frame;
		frame1.transform() *= tween.fetch_and_tick(1);

		if(format_desc.field_mode != core::field_mode::progressive)
		{				
			auto frame2 = frame;
			frame2.transform() *= tween.fetch_and_tick(1);
			frame1 = core::draw_frame::interlace(frame1, frame2, format_desc.field_mode);
		}

		frames[index] = frame1;
	}

	layer& get_layer(int index)
	{
		auto it = layers_.find(index);
		if(it == std::end(layers_))
		{
			it = layers_.insert(std::make_pair(index, layer(index))).first;
			it->second.subscribe(event_subject_);
		}
		return it->second;
	}
		
	boost::unique_future<void> apply_transforms(const std::vector<std::tuple<int, stage::transform_func_t, unsigned int, tweener>>& transforms)
	{
		return executor_.begin_invoke([=]
		{
			BOOST_FOREACH(auto& transform, transforms)
			{
				auto src = tweens_[std::get<0>(transform)].fetch();
				auto dst = std::get<1>(transform)(src);
				tweens_[std::get<0>(transform)] = tweened_transform(src, dst, std::get<2>(transform), std::get<3>(transform));
			}
		}, task_priority::high_priority);
	}
						
	boost::unique_future<void> apply_transform(int index, const stage::transform_func_t& transform, unsigned int mix_duration, const tweener& tween)
	{
		return executor_.begin_invoke([=]
		{
			auto src = tweens_[index].fetch();
			auto dst = transform(src);
			tweens_[index] = tweened_transform(src, dst, mix_duration, tween);
		}, task_priority::high_priority);
	}

	boost::unique_future<void> clear_transforms(int index)
	{
		return executor_.begin_invoke([=]
		{
			tweens_.erase(index);
		}, task_priority::high_priority);
	}

	boost::unique_future<void> clear_transforms()
	{
		return executor_.begin_invoke([=]
		{
			tweens_.clear();
		}, task_priority::high_priority);
	}
		
	boost::unique_future<void> load(int index, const spl::shared_ptr<frame_producer>& producer, bool preview, const boost::optional<int32_t>& auto_play_delta)
	{
		return executor_.begin_invoke([=]
		{
			get_layer(index).load(producer, preview, auto_play_delta);			
		}, task_priority::high_priority);
	}

	boost::unique_future<void> pause(int index)
	{		
		return executor_.begin_invoke([=]
		{
			layers_[index].pause();
		}, task_priority::high_priority);
	}

	boost::unique_future<void> play(int index)
	{		
		return executor_.begin_invoke([=]
		{
			layers_[index].play();
		}, task_priority::high_priority);
	}

	boost::unique_future<void> stop(int index)
	{		
		return executor_.begin_invoke([=]
		{
			layers_[index].stop();
		}, task_priority::high_priority);
	}

	boost::unique_future<void> clear(int index)
	{
		return executor_.begin_invoke([=]
		{
			layers_.erase(index);
		}, task_priority::high_priority);
	}
		
	boost::unique_future<void> clear()
	{
		return executor_.begin_invoke([=]
		{
			layers_.clear();
		}, task_priority::high_priority);
	}	
		
	boost::unique_future<void> swap_layers(stage& other)
	{
		auto other_impl = other.impl_;

		if(other_impl.get() == this)
			return async(launch::deferred, []{});
		
		auto func = [=]
		{
			auto layers			= layers_ | boost::adaptors::map_values;
			auto other_layers	= other_impl->layers_ | boost::adaptors::map_values;

			BOOST_FOREACH(auto& layer, layers)
				layer.unsubscribe(event_subject_);
			
			BOOST_FOREACH(auto& layer, other_layers)
				layer.unsubscribe(event_subject_);
			
			std::swap(layers_, other_impl->layers_);
						
			BOOST_FOREACH(auto& layer, layers)
				layer.subscribe(event_subject_);
			
			BOOST_FOREACH(auto& layer, other_layers)
				layer.subscribe(event_subject_);
		};		

		return executor_.begin_invoke([=]
		{
			other_impl->executor_.invoke(func, task_priority::high_priority);
		}, task_priority::high_priority);
	}

	boost::unique_future<void> swap_layer(int index, int other_index)
	{
		return executor_.begin_invoke([=]
		{
			std::swap(layers_[index], layers_[other_index]);
		}, task_priority::high_priority);
	}

	boost::unique_future<void> swap_layer(int index, int other_index, stage& other)
	{
		auto other_impl = other.impl_;

		if(other_impl.get() == this)
			return swap_layer(index, other_index);
		else
		{
			auto func = [=]
			{
				auto& my_layer		= get_layer(index);
				auto& other_layer	= other_impl->get_layer(other_index);

				my_layer.unsubscribe(event_subject_);
				other_layer.unsubscribe(other_impl->event_subject_);

				std::swap(my_layer, other_layer);

				my_layer.subscribe(event_subject_);
				other_layer.subscribe(other_impl->event_subject_);
			};		

			return executor_.begin_invoke([=]
			{
				other_impl->executor_.invoke(func, task_priority::high_priority);
			}, task_priority::high_priority);
		}
	}
		
	boost::unique_future<spl::shared_ptr<frame_producer>> foreground(int index)
	{
		return executor_.begin_invoke([=]
		{
			return layers_[index].foreground();
		}, task_priority::high_priority);
	}
	
	boost::unique_future<spl::shared_ptr<frame_producer>> background(int index)
	{
		return executor_.begin_invoke([=]
		{
			return layers_[index].background();
		}, task_priority::high_priority);
	}

	boost::unique_future<boost::property_tree::wptree> info()
	{
		return executor_.begin_invoke([this]() -> boost::property_tree::wptree
		{
			boost::property_tree::wptree info;
			BOOST_FOREACH(auto& layer, layers_)			
				info.add_child(L"layers.layer", layer.second.info())
					.add(L"index", layer.first);	
			return info;
		}, task_priority::high_priority);
	}

	boost::unique_future<boost::property_tree::wptree> info(int index)
	{
		return executor_.begin_invoke([=]
		{
			return layers_[index].info();
		}, task_priority::high_priority);
	}		
	
	boost::unique_future<std::wstring> call(int index, const std::wstring& params)
	{
		return flatten(executor_.begin_invoke([=]
		{
			return make_shared(layers_[index].foreground()->call(params));
		}, task_priority::high_priority));
	}
};

stage::stage(spl::shared_ptr<diagnostics::graph> graph) : impl_(new impl(std::move(graph))){}
boost::unique_future<std::wstring> stage::call(int index, const std::wstring& params){return impl_->call(index, params);}
boost::unique_future<void> stage::apply_transforms(const std::vector<stage::transform_tuple_t>& transforms){return impl_->apply_transforms(transforms);}
boost::unique_future<void> stage::apply_transform(int index, const std::function<core::frame_transform(core::frame_transform)>& transform, unsigned int mix_duration, const tweener& tween){return impl_->apply_transform(index, transform, mix_duration, tween);}
boost::unique_future<void> stage::clear_transforms(int index){return impl_->clear_transforms(index);}
boost::unique_future<void> stage::clear_transforms(){return impl_->clear_transforms();}
boost::unique_future<void> stage::load(int index, const spl::shared_ptr<frame_producer>& producer, bool preview, const boost::optional<int32_t>& auto_play_delta){return impl_->load(index, producer, preview, auto_play_delta);}
boost::unique_future<void> stage::pause(int index){return impl_->pause(index);}
boost::unique_future<void> stage::play(int index){return impl_->play(index);}
boost::unique_future<void> stage::stop(int index){return impl_->stop(index);}
boost::unique_future<void> stage::clear(int index){return impl_->clear(index);}
boost::unique_future<void> stage::clear(){return impl_->clear();}
boost::unique_future<void> stage::swap_layers(stage& other){return impl_->swap_layers(other);}
boost::unique_future<void> stage::swap_layer(int index, int other_index){return impl_->swap_layer(index, other_index);}
boost::unique_future<void> stage::swap_layer(int index, int other_index, stage& other){return impl_->swap_layer(index, other_index, other);}
boost::unique_future<spl::shared_ptr<frame_producer>> stage::foreground(int index) {return impl_->foreground(index);}
boost::unique_future<spl::shared_ptr<frame_producer>> stage::background(int index) {return impl_->background(index);}
boost::unique_future<boost::property_tree::wptree> stage::info() const{return impl_->info();}
boost::unique_future<boost::property_tree::wptree> stage::info(int index) const{return impl_->info(index);}
std::map<int, class draw_frame> stage::operator()(const video_format_desc& format_desc){return (*impl_)(format_desc);}
void stage::subscribe(const monitor::observable::observer_ptr& o) {impl_->event_subject_.subscribe(o);}
void stage::unsubscribe(const monitor::observable::observer_ptr& o) {impl_->event_subject_.unsubscribe(o);}
void stage::subscribe(const frame_observable::observer_ptr& o) {impl_->frames_subject_.subscribe(o);}
void stage::unsubscribe(const frame_observable::observer_ptr& o) {impl_->frames_subject_.unsubscribe(o);}
}}