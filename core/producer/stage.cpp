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
#include "../interaction/interaction_aggregator.h"

#include <common/executor.h>
#include <common/future.h>
#include <common/diagnostics/graph.h>

#include <core/frame/frame_transform.h>

#include <boost/timer.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/range/algorithm_ext.hpp>

#include <tbb/parallel_for_each.h>

#include <functional>
#include <map>
#include <vector>
#include <future>

namespace caspar { namespace core {
	
struct stage::impl : public std::enable_shared_from_this<impl>
{				
	spl::shared_ptr<diagnostics::graph>							graph_;
	spl::shared_ptr<monitor::subject>							monitor_subject_;
	//reactive::basic_subject<std::map<int, class draw_frame>>	frames_subject_;
	std::map<int, layer>										layers_;	
	std::map<int, tweened_transform>							tweens_;
	interaction_aggregator										aggregator_;
	executor													executor_;
public:
	impl(spl::shared_ptr<diagnostics::graph> graph) 
		: graph_(std::move(graph))
		, monitor_subject_(spl::make_shared<monitor::subject>("/stage"))
		, aggregator_([=] (double x, double y) { return collission_detect(x, y); })
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

				for (auto& layer : layers_)	
				{
					frames[layer.first] = draw_frame::empty();	
					indices.push_back(layer.first);
				}

				aggregator_.translate_and_send();

				// WORKAROUND: Compiler doesn't seem to like lambda.
				tbb::parallel_for_each(indices.begin(), indices.end(), [&](int index)
				{
					draw(index, format_desc, frames);
				});
			}
			catch(...)
			{
				layers_.clear();
				CASPAR_LOG_CURRENT_EXCEPTION();
			}	
			

			return frames;
		});
		
		//frames_subject_ << frames;
		
		graph_->set_value("produce-time", frame_timer.elapsed()*format_desc.fps*0.5);
		*monitor_subject_ << monitor::message("/profiler/time") % frame_timer.elapsed() % (1.0/format_desc.fps);

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
			it->second.monitor_output().attach_parent(monitor_subject_);
		}
		return it->second;
	}
		
	std::future<void> apply_transforms(const std::vector<std::tuple<int, stage::transform_func_t, unsigned int, tweener>>& transforms)
	{
		return executor_.begin_invoke([=]
		{
			for (auto& transform : transforms)
			{
				auto src = tweens_[std::get<0>(transform)].fetch();
				auto dst = std::get<1>(transform)(src);
				tweens_[std::get<0>(transform)] = tweened_transform(src, dst, std::get<2>(transform), std::get<3>(transform));
			}
		}, task_priority::high_priority);
	}
						
	std::future<void> apply_transform(int index, const stage::transform_func_t& transform, unsigned int mix_duration, const tweener& tween)
	{
		return executor_.begin_invoke([=]
		{
			auto src = tweens_[index].fetch();
			auto dst = transform(src);
			tweens_[index] = tweened_transform(src, dst, mix_duration, tween);
		}, task_priority::high_priority);
	}

	std::future<void> clear_transforms(int index)
	{
		return executor_.begin_invoke([=]
		{
			tweens_.erase(index);
		}, task_priority::high_priority);
	}

	std::future<void> clear_transforms()
	{
		return executor_.begin_invoke([=]
		{
			tweens_.clear();
		}, task_priority::high_priority);
	}
		
	std::future<void> load(int index, const spl::shared_ptr<frame_producer>& producer, bool preview, const boost::optional<int32_t>& auto_play_delta)
	{
		return executor_.begin_invoke([=]
		{
			get_layer(index).load(producer, preview, auto_play_delta);			
		}, task_priority::high_priority);
	}

	std::future<void> pause(int index)
	{		
		return executor_.begin_invoke([=]
		{
			get_layer(index).pause();
		}, task_priority::high_priority);
	}

	std::future<void> play(int index)
	{		
		return executor_.begin_invoke([=]
		{
			get_layer(index).play();
		}, task_priority::high_priority);
	}

	std::future<void> stop(int index)
	{		
		return executor_.begin_invoke([=]
		{
			get_layer(index).stop();
		}, task_priority::high_priority);
	}

	std::future<void> clear(int index)
	{
		return executor_.begin_invoke([=]
		{
			layers_.erase(index);
		}, task_priority::high_priority);
	}
		
	std::future<void> clear()
	{
		return executor_.begin_invoke([=]
		{
			layers_.clear();
		}, task_priority::high_priority);
	}	
		
	std::future<void> swap_layers(stage& other)
	{
		auto other_impl = other.impl_;

		if (other_impl.get() == this)
		{
			return make_ready_future();
		}
		
		auto func = [=]
		{
			auto layers			= layers_ | boost::adaptors::map_values;
			auto other_layers	= other_impl->layers_ | boost::adaptors::map_values;

			for (auto& layer : layers)
				layer.monitor_output().detach_parent();
			
			for (auto& layer : other_layers)
				layer.monitor_output().detach_parent();
			
			std::swap(layers_, other_impl->layers_);
						
			for (auto& layer : layers)
				layer.monitor_output().attach_parent(monitor_subject_);
			
			for (auto& layer : other_layers)
				layer.monitor_output().attach_parent(monitor_subject_);
		};		

		return executor_.begin_invoke([=]
		{
			other_impl->executor_.invoke(func, task_priority::high_priority);
		}, task_priority::high_priority);
	}

	std::future<void> swap_layer(int index, int other_index)
	{
		return executor_.begin_invoke([=]
		{
			std::swap(get_layer(index), get_layer(other_index));
		}, task_priority::high_priority);
	}

	std::future<void> swap_layer(int index, int other_index, stage& other)
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

				my_layer.monitor_output().detach_parent();
				other_layer.monitor_output().detach_parent();

				std::swap(my_layer, other_layer);

				my_layer.monitor_output().attach_parent(monitor_subject_);
				other_layer.monitor_output().attach_parent(other_impl->monitor_subject_);
			};		

			return executor_.begin_invoke([=]
			{
				other_impl->executor_.invoke(func, task_priority::high_priority);
			}, task_priority::high_priority);
		}
	}
		
	std::future<std::shared_ptr<frame_producer>> foreground(int index)
	{
		return executor_.begin_invoke([=]() -> std::shared_ptr<frame_producer>
		{
			return get_layer(index).foreground();
		}, task_priority::high_priority);
	}
	
	std::future<std::shared_ptr<frame_producer>> background(int index)
	{
		return executor_.begin_invoke([=]() -> std::shared_ptr<frame_producer>
		{
			return get_layer(index).background();
		}, task_priority::high_priority);
	}

	std::future<boost::property_tree::wptree> info()
	{
		return executor_.begin_invoke([this]() -> boost::property_tree::wptree
		{
			boost::property_tree::wptree info;
			for (auto& layer : layers_)			
				info.add_child(L"layers.layer", layer.second.info())
					.add(L"index", layer.first);	
			return info;
		}, task_priority::high_priority);
	}

	std::future<boost::property_tree::wptree> info(int index)
	{
		return executor_.begin_invoke([=]
		{
			return get_layer(index).info();
		}, task_priority::high_priority);
	}		
	
	std::future<std::wstring> call(int index, const std::vector<std::wstring>& params)
	{
		return flatten(executor_.begin_invoke([=]
		{
			return get_layer(index).foreground()->call(params).share();
		}, task_priority::high_priority));
	}

	void on_interaction(const interaction_event::ptr& event)
	{
		executor_.begin_invoke([=]
		{
			aggregator_.offer(event);
		}, task_priority::high_priority);
	}

	boost::optional<interaction_target> collission_detect(double x, double y)
	{
		for (auto& layer : layers_ | boost::adaptors::reversed)
		{
			auto transform = tweens_[layer.first].fetch();
			auto translated = translate(x, y, transform);

			if (translated.first >= 0.0
				&& translated.first <= 1.0
				&& translated.second >= 0.0
				&& translated.second <= 1.0
				&& layer.second.collides(translated.first, translated.second))
			{
				return std::make_pair(transform, &layer.second);
			}
		}

		return boost::optional<interaction_target>();
	}
};

stage::stage(spl::shared_ptr<diagnostics::graph> graph) : impl_(new impl(std::move(graph))){}
std::future<std::wstring> stage::call(int index, const std::vector<std::wstring>& params){return impl_->call(index, params);}
std::future<void> stage::apply_transforms(const std::vector<stage::transform_tuple_t>& transforms){ return impl_->apply_transforms(transforms); }
std::future<void> stage::apply_transform(int index, const std::function<core::frame_transform(core::frame_transform)>& transform, unsigned int mix_duration, const tweener& tween){ return impl_->apply_transform(index, transform, mix_duration, tween); }
std::future<void> stage::clear_transforms(int index){ return impl_->clear_transforms(index); }
std::future<void> stage::clear_transforms(){ return impl_->clear_transforms(); }
std::future<void> stage::load(int index, const spl::shared_ptr<frame_producer>& producer, bool preview, const boost::optional<int32_t>& auto_play_delta){ return impl_->load(index, producer, preview, auto_play_delta); }
std::future<void> stage::pause(int index){ return impl_->pause(index); }
std::future<void> stage::play(int index){ return impl_->play(index); }
std::future<void> stage::stop(int index){ return impl_->stop(index); }
std::future<void> stage::clear(int index){ return impl_->clear(index); }
std::future<void> stage::clear(){ return impl_->clear(); }
std::future<void> stage::swap_layers(stage& other){ return impl_->swap_layers(other); }
std::future<void> stage::swap_layer(int index, int other_index){ return impl_->swap_layer(index, other_index); }
std::future<void> stage::swap_layer(int index, int other_index, stage& other){ return impl_->swap_layer(index, other_index, other); }
std::future<std::shared_ptr<frame_producer>> stage::foreground(int index) { return impl_->foreground(index); }
std::future<std::shared_ptr<frame_producer>> stage::background(int index) { return impl_->background(index); }
std::future<boost::property_tree::wptree> stage::info() const{ return impl_->info(); }
std::future<boost::property_tree::wptree> stage::info(int index) const{ return impl_->info(index); }
std::map<int, class draw_frame> stage::operator()(const video_format_desc& format_desc){return (*impl_)(format_desc);}
monitor::subject& stage::monitor_output(){return *impl_->monitor_subject_;}
//void stage::subscribe(const frame_observable::observer_ptr& o) {impl_->frames_subject_.subscribe(o);}
//void stage::unsubscribe(const frame_observable::observer_ptr& o) {impl_->frames_subject_.unsubscribe(o);}
void stage::on_interaction(const interaction_event::ptr& event) { impl_->on_interaction(event); }
}}
