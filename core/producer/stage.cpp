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
#include "../consumer/write_frame_consumer.h"

#include <common/executor.h>
#include <common/future.h>
#include <common/diagnostics/graph.h>
#include <common/timer.h>

#include <core/frame/frame_transform.h>

#include <boost/lexical_cast.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>

#include <tbb/parallel_for_each.h>

#include <functional>
#include <map>
#include <vector>
#include <future>

namespace caspar { namespace core {

struct stage::impl : public std::enable_shared_from_this<impl>
{
	int																		channel_index_;
	spl::shared_ptr<diagnostics::graph>										graph_;
	spl::shared_ptr<monitor::subject>										monitor_subject_ = spl::make_shared<monitor::subject>("/stage");
	std::map<int, layer>													layers_;
	std::map<int, tweened_transform>										tweens_;
	interaction_aggregator													aggregator_;
	// map of layer -> map of tokens (src ref) -> layer_consumer
	std::map<int, std::map<void*, spl::shared_ptr<write_frame_consumer>>>	layer_consumers_;
	executor																executor_ { L"stage " + boost::lexical_cast<std::wstring>(channel_index_) };
public:
	impl(int channel_index, spl::shared_ptr<diagnostics::graph> graph)
		: channel_index_(channel_index)
		, graph_(std::move(graph))
		, aggregator_([=] (double x, double y) { return collission_detect(x, y); })
	{
		graph_->set_color("produce-time", diagnostics::color(0.0f, 1.0f, 0.0f));
	}

	std::map<int, draw_frame> operator()(const video_format_desc& format_desc)
	{
		caspar::timer frame_timer;

		auto frames = executor_.invoke([=]() -> std::map<int, draw_frame>
		{

			std::map<int, draw_frame> frames;

			try
			{
				std::vector<int> indices;

				for (auto& layer : layers_)
				{
					// Prevent race conditions in parallel for each later
                    frames[layer.first];
					tweens_[layer.first];
					layer_consumers_[layer.first];

					indices.push_back(layer.first);
				}

				aggregator_.translate_and_send();

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

		graph_->set_value("produce-time", frame_timer.elapsed()*format_desc.fps*0.5);
		*monitor_subject_ << monitor::message("/profiler/time") % frame_timer.elapsed() % (1.0/format_desc.fps);

		return frames;
	}

	void draw(int index, const video_format_desc& format_desc, std::map<int, draw_frame>& frames)
	{
		auto& layer		= layers_[index];
		auto& tween		= tweens_[index];
		auto& consumers	= layer_consumers_[index];

		auto frame  = layer.receive(format_desc);

		if (!consumers.empty())
		{
			auto consumer_it = consumers | boost::adaptors::map_values;
			tbb::parallel_for_each(consumer_it.begin(), consumer_it.end(), [&](decltype(*consumer_it.begin()) layer_consumer)
			{
				layer_consumer->send(frame);
			});
		}

		auto frame1 = frame;

		frame1.transform() *= tween.fetch_and_tick(1);

		if(format_desc.field_mode != core::field_mode::progressive)
		{
			auto frame2 = frame;
			frame2.transform() *= tween.fetch_and_tick(1);
			frame2.transform().audio_transform.volume = 0.0;
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
				auto& tween = tweens_[std::get<0>(transform)];
				auto src = tween.fetch();
				auto dst = std::get<1>(transform)(tween.dest());
				tweens_[std::get<0>(transform)] = tweened_transform(src, dst, std::get<2>(transform), std::get<3>(transform));
			}
		});
	}

	std::future<void> apply_transform(int index, const stage::transform_func_t& transform, unsigned int mix_duration, const tweener& tween)
	{
		return executor_.begin_invoke([=]
		{
			auto src = tweens_[index].fetch();
			auto dst = transform(src);
			tweens_[index] = tweened_transform(src, dst, mix_duration, tween);
		});
	}

	std::future<void> clear_transforms(int index)
	{
		return executor_.begin_invoke([=]
		{
			tweens_.erase(index);
		});
	}

	std::future<void> clear_transforms()
	{
		return executor_.begin_invoke([=]
		{
			tweens_.clear();
		});
	}

	std::future<frame_transform> get_current_transform(int index)
	{
		return executor_.begin_invoke([=]
		{
			return tweens_[index].fetch();
		});
	}

	std::future<void> load(int index, const spl::shared_ptr<frame_producer>& producer, bool preview, const boost::optional<int32_t>& auto_play_delta)
	{
		return executor_.begin_invoke([=]
		{
			get_layer(index).load(producer, preview, auto_play_delta);
		});
	}

	std::future<void> pause(int index)
	{
		return executor_.begin_invoke([=]
		{
			get_layer(index).pause();
		});
	}

	std::future<void> resume(int index)
	{
		return executor_.begin_invoke([=]
		{
			get_layer(index).resume();
		});
	}

	std::future<void> play(int index)
	{
		return executor_.begin_invoke([=]
		{
			get_layer(index).play();
		});
	}

	std::future<void> stop(int index)
	{
		return executor_.begin_invoke([=]
		{
			get_layer(index).stop();
		});
	}

	std::future<void> clear(int index)
	{
		return executor_.begin_invoke([=]
		{
			layers_.erase(index);
		});
	}

	std::future<void> clear()
	{
		return executor_.begin_invoke([=]
		{
			layers_.clear();
		});
	}

	std::future<void> swap_layers(stage& other, bool swap_transforms)
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

			if (swap_transforms)
				std::swap(tweens_, other_impl->tweens_);
		};

		return executor_.begin_invoke([=]
		{
			other_impl->executor_.invoke(func);
		});
	}

	std::future<void> swap_layer(int index, int other_index, bool swap_transforms)
	{
		return executor_.begin_invoke([=]
		{
			std::swap(get_layer(index), get_layer(other_index));

			if (swap_transforms)
				std::swap(tweens_[index], tweens_[other_index]);
		});
	}

	std::future<void> swap_layer(int index, int other_index, stage& other, bool swap_transforms)
	{
		auto other_impl = other.impl_;

		if(other_impl.get() == this)
			return swap_layer(index, other_index, swap_transforms);
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

				if (swap_transforms)
				{
					auto& my_tween		= tweens_[index];
					auto& other_tween	= other_impl->tweens_[other_index];
					std::swap(my_tween, other_tween);
				}
			};

			return executor_.begin_invoke([=]
			{
				other_impl->executor_.invoke(func);
			});
		}
	}

	void add_layer_consumer(void* token, int layer, const spl::shared_ptr<write_frame_consumer>& layer_consumer)
	{
		executor_.begin_invoke([=]
		{
			layer_consumers_[layer].insert(std::make_pair(token, layer_consumer));
		});
	}

	void remove_layer_consumer(void* token, int layer)
	{
		executor_.begin_invoke([=]
		{
			auto& layer_map = layer_consumers_[layer];
			layer_map.erase(token);
			if (layer_map.empty())
			{
				layer_consumers_.erase(layer);
			}
		});
	}

	std::future<std::shared_ptr<frame_producer>> foreground(int index)
	{
		return executor_.begin_invoke([=]() -> std::shared_ptr<frame_producer>
		{
			return get_layer(index).foreground();
		});
	}

	std::future<std::shared_ptr<frame_producer>> background(int index)
	{
		return executor_.begin_invoke([=]() -> std::shared_ptr<frame_producer>
		{
			return get_layer(index).background();
		});
	}

	std::future<std::wstring> call(int index, const std::vector<std::wstring>& params)
	{
		return flatten(executor_.begin_invoke([=]
		{
			return get_layer(index).foreground()->call(params).share();
		}));
	}

	void on_interaction(const interaction_event::ptr& event)
	{
		executor_.begin_invoke([=]
		{
			aggregator_.offer(event);
		});
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
				return std::make_pair(transform, static_cast<interaction_sink*>(&layer.second));
			}
		}

		return boost::optional<interaction_target>();
	}
};

stage::stage(int channel_index, spl::shared_ptr<diagnostics::graph> graph) : impl_(new impl(channel_index, std::move(graph))){}
std::future<std::wstring> stage::call(int index, const std::vector<std::wstring>& params){return impl_->call(index, params);}
std::future<void> stage::apply_transforms(const std::vector<stage::transform_tuple_t>& transforms){ return impl_->apply_transforms(transforms); }
std::future<void> stage::apply_transform(int index, const std::function<core::frame_transform(core::frame_transform)>& transform, unsigned int mix_duration, const tweener& tween){ return impl_->apply_transform(index, transform, mix_duration, tween); }
std::future<void> stage::clear_transforms(int index){ return impl_->clear_transforms(index); }
std::future<void> stage::clear_transforms(){ return impl_->clear_transforms(); }
std::future<frame_transform> stage::get_current_transform(int index){ return impl_->get_current_transform(index); }
std::future<void> stage::load(int index, const spl::shared_ptr<frame_producer>& producer, bool preview, const boost::optional<int32_t>& auto_play_delta){ return impl_->load(index, producer, preview, auto_play_delta); }
std::future<void> stage::pause(int index){ return impl_->pause(index); }
std::future<void> stage::resume(int index){ return impl_->resume(index); }
std::future<void> stage::play(int index){ return impl_->play(index); }
std::future<void> stage::stop(int index){ return impl_->stop(index); }
std::future<void> stage::clear(int index){ return impl_->clear(index); }
std::future<void> stage::clear(){ return impl_->clear(); }
std::future<void> stage::swap_layers(stage& other, bool swap_transforms){ return impl_->swap_layers(other, swap_transforms); }
std::future<void> stage::swap_layer(int index, int other_index, bool swap_transforms){ return impl_->swap_layer(index, other_index, swap_transforms); }
std::future<void> stage::swap_layer(int index, int other_index, stage& other, bool swap_transforms){ return impl_->swap_layer(index, other_index, other, swap_transforms); }
void stage::add_layer_consumer(void* token, int layer, const spl::shared_ptr<write_frame_consumer>& layer_consumer){ impl_->add_layer_consumer(token, layer, layer_consumer); }
void stage::remove_layer_consumer(void* token, int layer){ impl_->remove_layer_consumer(token, layer); }std::future<std::shared_ptr<frame_producer>> stage::foreground(int index) { return impl_->foreground(index); }
std::future<std::shared_ptr<frame_producer>> stage::background(int index) { return impl_->background(index); }
std::map<int, draw_frame> stage::operator()(const video_format_desc& format_desc){ return (*impl_)(format_desc); }
monitor::subject& stage::monitor_output(){return *impl_->monitor_subject_;}
void stage::on_interaction(const interaction_event::ptr& event) { impl_->on_interaction(event); }
}}
