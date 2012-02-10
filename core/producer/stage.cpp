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

#include <common/concurrency/executor.h>
#include <common/diagnostics/graph.h>

#include <core/frame/frame_transform.h>

#include <boost/foreach.hpp>
#include <boost/timer.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/range/algorithm_ext.hpp>

#include <tbb/parallel_for_each.h>

#include <map>
#include <vector>

namespace caspar { namespace core {
	
struct stage::impl : public std::enable_shared_from_this<impl>
{				
	spl::shared_ptr<diagnostics::graph> graph_;
	std::map<int, layer>				layers_;	
	std::map<int, tweened_transform>	tweens_;	
	executor							executor_;
public:
	impl(spl::shared_ptr<diagnostics::graph> graph) 
		: graph_(std::move(graph))
		, executor_(L"stage")
	{
		graph_->set_color("produce-time", diagnostics::color(0.0f, 1.0f, 0.0f));
	}
		
	std::map<int, spl::shared_ptr<draw_frame>> operator()(const struct video_format_desc& format_desc)
	{		
		return executor_.invoke([=]() -> std::map<int, spl::shared_ptr<draw_frame>>
		{
			boost::timer frame_timer;

			std::map<int, spl::shared_ptr<class draw_frame>> frames;

			try
			{					
				BOOST_FOREACH(auto& layer, layers_)			
					frames[layer.first] = draw_frame::empty();	

				auto format_desc2 = format_desc;

				tbb::parallel_for_each(layers_.begin(), layers_.end(), [&](std::map<int, layer>::value_type& layer)
				{
					auto& tween		= tweens_[layer.first];
					auto transform	= tween.fetch_and_tick(1);

					frame_producer::flags flags = frame_producer::flags::none;
					if(format_desc2.field_mode != field_mode::progressive)
					{
						flags |= std::abs(transform.fill_scale[1]  - 1.0) > 0.0001 ? frame_producer::flags::deinterlace : frame_producer::flags::none;
						flags |= std::abs(transform.fill_translation[1])  > 0.0001 ? frame_producer::flags::deinterlace : frame_producer::flags::none;
					}

					if(transform.is_key)
						flags |= frame_producer::flags::alpha_only;

					auto frame = layer.second.receive(flags);	
				
					auto frame1 = spl::make_shared<core::draw_frame>(frame);
					frame1->get_frame_transform() = transform;

					if(format_desc2.field_mode != core::field_mode::progressive)
					{				
						auto frame2 = spl::make_shared<core::draw_frame>(frame);
						frame2->get_frame_transform() = tween.fetch_and_tick(1);
						frame1 = core::draw_frame::interlace(frame1, frame2, format_desc2.field_mode);
					}

					frames[layer.first] = frame1;
				});		
			}
			catch(...)
			{
				layers_.clear();
				CASPAR_LOG_CURRENT_EXCEPTION();
			}	
			
			graph_->set_value("produce-time", frame_timer.elapsed()*format_desc.fps*0.5);

			return frames;
		});
	}
		
	void apply_transforms(const std::vector<std::tuple<int, stage::transform_func_t, unsigned int, tweener>>& transforms)
	{
		executor_.begin_invoke([=]
		{
			BOOST_FOREACH(auto& transform, transforms)
			{
				auto src = tweens_[std::get<0>(transform)].fetch();
				auto dst = std::get<1>(transform)(src);
				tweens_[std::get<0>(transform)] = tweened_transform(src, dst, std::get<2>(transform), std::get<3>(transform));
			}
		}, task_priority::high_priority);
	}
						
	void apply_transform(int index, const stage::transform_func_t& transform, unsigned int mix_duration, const tweener& tween)
	{
		executor_.begin_invoke([=]
		{
			auto src = tweens_[index].fetch();
			auto dst = transform(src);
			tweens_[index] = tweened_transform(src, dst, mix_duration, tween);
		}, task_priority::high_priority);
	}

	void clear_transforms(int index)
	{
		executor_.begin_invoke([=]
		{
			tweens_.erase(index);
		}, task_priority::high_priority);
	}

	void clear_transforms()
	{
		executor_.begin_invoke([=]
		{
			tweens_.clear();
		}, task_priority::high_priority);
	}
		
	void load(int index, const spl::shared_ptr<frame_producer>& producer, const boost::optional<int32_t>& auto_play_delta)
	{
		executor_.begin_invoke([=]
		{
			layers_[index].load(producer, auto_play_delta);
		}, task_priority::high_priority);
	}

	void pause(int index)
	{		
		executor_.begin_invoke([=]
		{
			layers_[index].pause();
		}, task_priority::high_priority);
	}

	void play(int index)
	{		
		executor_.begin_invoke([=]
		{
			layers_[index].play();
		}, task_priority::high_priority);
	}

	void stop(int index)
	{		
		executor_.begin_invoke([=]
		{
			layers_[index].stop();
		}, task_priority::high_priority);
	}

	void clear(int index)
	{
		executor_.begin_invoke([=]
		{
			layers_.erase(index);
		}, task_priority::high_priority);
	}
		
	void clear()
	{
		executor_.begin_invoke([=]
		{
			layers_.clear();
		}, task_priority::high_priority);
	}	
		
	void swap_layers(const spl::shared_ptr<stage>& other)
	{
		if(other->impl_.get() == this)
			return;
		
		auto func = [=]
		{
			std::swap(layers_, other->impl_->layers_);
		};		
		executor_.begin_invoke([=]
		{
			other->impl_->executor_.invoke(func, task_priority::high_priority);
		}, task_priority::high_priority);
	}

	void swap_layer(int index, int other_index)
	{
		executor_.begin_invoke([=]
		{
			std::swap(layers_[index], layers_[other_index]);
		}, task_priority::high_priority);
	}

	void swap_layer(int index, int other_index, const spl::shared_ptr<stage>& other)
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
				other->impl_->executor_.invoke(func, task_priority::high_priority);
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
};

stage::stage(spl::shared_ptr<diagnostics::graph> graph) : impl_(new impl(std::move(graph))){}
void stage::apply_transforms(const std::vector<stage::transform_tuple_t>& transforms){impl_->apply_transforms(transforms);}
void stage::apply_transform(int index, const std::function<core::frame_transform(core::frame_transform)>& transform, unsigned int mix_duration, const tweener& tween){impl_->apply_transform(index, transform, mix_duration, tween);}
void stage::clear_transforms(int index){impl_->clear_transforms(index);}
void stage::clear_transforms(){impl_->clear_transforms();}
void stage::load(int index, const spl::shared_ptr<frame_producer>& producer, const boost::optional<int32_t>& auto_play_delta){impl_->load(index, producer, auto_play_delta);}
void stage::pause(int index){impl_->pause(index);}
void stage::play(int index){impl_->play(index);}
void stage::stop(int index){impl_->stop(index);}
void stage::clear(int index){impl_->clear(index);}
void stage::clear(){impl_->clear();}
void stage::swap_layers(const spl::shared_ptr<stage>& other){impl_->swap_layers(other);}
void stage::swap_layer(int index, int other_index){impl_->swap_layer(index, other_index);}
void stage::swap_layer(int index, int other_index, const spl::shared_ptr<stage>& other){impl_->swap_layer(index, other_index, other);}
boost::unique_future<spl::shared_ptr<frame_producer>> stage::foreground(int index) {return impl_->foreground(index);}
boost::unique_future<spl::shared_ptr<frame_producer>> stage::background(int index) {return impl_->background(index);}
boost::unique_future<boost::property_tree::wptree> stage::info() const{return impl_->info();}
boost::unique_future<boost::property_tree::wptree> stage::info(int index) const{return impl_->info(index);}
std::map<int, spl::shared_ptr<class draw_frame>> stage::operator()(const video_format_desc& format_desc){return (*impl_)(format_desc);}
}}