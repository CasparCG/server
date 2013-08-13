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
* Author: Helge Norberg, helge.norberg@svt.se
*/

#include "../../stdafx.h"

#include <common/future.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include "scene_producer.h"

#include "../../frame/draw_frame.h"
#include "../../interaction/interaction_aggregator.h"
#include "../text/text_producer.h"

namespace caspar { namespace core { namespace scene {

layer::layer(const spl::shared_ptr<frame_producer>& producer)
	: producer(producer)
{
}
layer::layer(const std::wstring& name, const spl::shared_ptr<frame_producer>& producer)
	: name(name), producer(producer)
{
}

adjustments::adjustments()
	: opacity(1.0)
{
}

struct timeline
{
	std::map<int64_t, keyframe> keyframes;

	void on_frame(int64_t frame)
	{
		auto before = --keyframes.upper_bound(frame);
		bool found_before = before != keyframes.end() && before->first < frame;
		auto after = keyframes.upper_bound(frame);
		bool found_after = after != keyframes.end() && after->first > frame;
		auto exact_frame = keyframes.find(frame);
		bool found_exact_frame = exact_frame != keyframes.end();

		if (found_exact_frame)
		{
			exact_frame->second.on_destination_frame();

			auto next_frame = ++exact_frame;

			if (next_frame != keyframes.end() && next_frame->second.on_start_animate)
				next_frame->second.on_start_animate();
		}
		else if (found_after)
		{
			int64_t start_frame = 0;

			if (found_before)
			{
				start_frame = before->first;
			}

			if (after->second.on_start_animate && frame == 0)
				after->second.on_start_animate();
			else if (after->second.on_animate_to)
				after->second.on_animate_to(start_frame, frame);
		}
	}
};

struct scene_producer::impl
{
	constraints pixel_constraints_;
	std::list<layer> layers_;
	interaction_aggregator aggregator_;
	binding<int64_t> frame_number_;
	std::map<std::wstring, std::shared_ptr<parameter_holder_base>> parameters_;
	std::map<void*, timeline> timelines_;

	impl(int width, int height)
		: pixel_constraints_(width, height)
		, aggregator_([=] (double x, double y) { return collission_detect(x, y); })
	{
	}

	layer& create_layer(
			const spl::shared_ptr<frame_producer>& producer, int x, int y, const std::wstring& name)
	{
		layer& layer = create_layer(producer, x, y);
		layer.name.set(name);

		return layer;
	}

	layer& create_layer(
			const spl::shared_ptr<frame_producer>& producer, int x, int y)
	{
		layer& layer = create_layer(producer);

		layer.position.x.set(x);
		layer.position.y.set(y);

		return layer;
	}

	layer& create_layer(const spl::shared_ptr<frame_producer>& producer)
	{
		layer layer(producer);

		layers_.push_back(layer);

		return layers_.back();
	}

	void store_parameter(
			const std::wstring& name,
			const std::shared_ptr<parameter_holder_base>& param)
	{
		parameters_.insert(std::make_pair(boost::to_lower_copy(name), param));
	}

	void store_keyframe(void* timeline_identity, const keyframe& k)
	{
		timelines_[timeline_identity].keyframes.insert(std::make_pair(k.destination_frame, k));
	}

	binding<int64_t> frame()
	{
		return frame_number_;
	}

	frame_transform get_transform(const layer& layer) const
	{
		frame_transform transform;

		auto& pos = transform.image_transform.fill_translation;
		auto& scale = transform.image_transform.fill_scale;

		pos[0] = static_cast<double>(layer.position.x.get()) / static_cast<double>(pixel_constraints_.width.get());
		pos[1] = static_cast<double>(layer.position.y.get()) / static_cast<double>(pixel_constraints_.height.get());
		scale[0] = static_cast<double>(layer.producer.get()->pixel_constraints().width.get())
				/ static_cast<double>(pixel_constraints_.width.get());
		scale[1] = static_cast<double>(layer.producer.get()->pixel_constraints().height.get())
				/ static_cast<double>(pixel_constraints_.height.get());

		transform.image_transform.opacity = layer.adjustments.opacity.get();
		transform.image_transform.is_key = layer.is_key.get();

		return transform;
	}

	draw_frame render_frame()
	{
		BOOST_FOREACH(auto& timeline, timelines_)
			timeline.second.on_frame(frame_number_.get());

		std::vector<draw_frame> frames;

		BOOST_FOREACH(auto& layer, layers_)
		{
			if (layer.hidden.get())
				continue;

			draw_frame frame(layer.producer.get()->receive());
			frame.transform() = get_transform(layer);;
			frames.push_back(frame);
		}

		++frame_number_;

		return draw_frame(frames);
	}

	void on_interaction(const interaction_event::ptr& event)
	{
		aggregator_.translate_and_send(event);
	}

	bool collides(double x, double y) const
	{
		return collission_detect(x, y);
	}

	boost::optional<interaction_target> collission_detect(double x, double y) const
	{
		BOOST_FOREACH(auto& layer, layers_ | boost::adaptors::reversed)
		{
			if (layer.hidden.get())
				continue;

			auto transform = get_transform(layer);
			auto translated = translate(x, y, transform);

			if (translated.first >= 0.0
				&& translated.first <= 1.0
				&& translated.second >= 0.0
				&& translated.second <= 1.0
				&& layer.producer.get()->collides(translated.first, translated.second))
			{
				return std::make_pair(transform, layer.producer.get().get());
			}
		}

		return boost::optional<interaction_target>();
	}

	boost::unique_future<std::wstring> call(const std::vector<std::wstring>& params) 
	{
		for (int i = 0; i + 1 < params.size(); i += 2)
		{
			auto found = parameters_.find(boost::to_lower_copy(params[i]));

			if (found != parameters_.end())
				found->second->set(params[i + 1]);
		}

		return wrap_as_future(std::wstring(L""));
	}

	std::wstring print() const
	{
		return L"scene[]";
	}

	std::wstring name() const
	{
		return L"scene";
	}
	
	boost::property_tree::wptree info() const
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"scene");
		return info;
	}

	void subscribe(const monitor::observable::observer_ptr& o)
	{
	}

	void unsubscribe(const monitor::observable::observer_ptr& o)
	{
	}
};

scene_producer::scene_producer(int width, int height)
	: impl_(new impl(width, height))
{
}

scene_producer::~scene_producer()
{
}

layer& scene_producer::create_layer(
		const spl::shared_ptr<frame_producer>& producer, int x, int y)
{
	return impl_->create_layer(producer, x, y);
}

layer& scene_producer::create_layer(
		const spl::shared_ptr<frame_producer>& producer, int x, int y, const std::wstring& name)
{
	return impl_->create_layer(producer, x, y, name);
}

layer& scene_producer::create_layer(
		const spl::shared_ptr<frame_producer>& producer)
{
	return impl_->create_layer(producer);
}

binding<int64_t> scene_producer::frame()
{
	return impl_->frame();
}

draw_frame scene_producer::receive_impl()
{
	return impl_->render_frame();
}

constraints& scene_producer::pixel_constraints() { return impl_->pixel_constraints_; }

void scene_producer::on_interaction(const interaction_event::ptr& event)
{
	impl_->on_interaction(event);
}

bool scene_producer::collides(double x, double y) const
{
	return impl_->collides(x, y);
}

std::wstring scene_producer::print() const
{
	return impl_->print();
}

std::wstring scene_producer::name() const
{
	return impl_->name();
}

boost::property_tree::wptree scene_producer::info() const
{
	return impl_->info();
}

boost::unique_future<std::wstring> scene_producer::call(const std::vector<std::wstring>& params) 
{
	return impl_->call(params);
}

void scene_producer::subscribe(const monitor::observable::observer_ptr& o)
{
	impl_->subscribe(o);
}

void scene_producer::unsubscribe(const monitor::observable::observer_ptr& o)
{
	impl_->unsubscribe(o);
}

void scene_producer::store_parameter(
		const std::wstring& name,
		const std::shared_ptr<parameter_holder_base>& param)
{
	impl_->store_parameter(name, param);
}

void scene_producer::store_keyframe(void* timeline_identity, const keyframe& k)
{
	impl_->store_keyframe(timeline_identity, k);
}

spl::shared_ptr<frame_producer> create_dummy_scene_producer(const spl::shared_ptr<class frame_factory>& frame_factory, const video_format_desc& format_desc, const std::vector<std::wstring>& params)
{
	if (params.size() < 1 || !boost::iequals(params.at(0), L"[SCENE]"))
		return core::frame_producer::empty();

	auto scene = spl::make_shared<scene_producer>(format_desc.width, format_desc.height);

	text::text_info text_info;
	text_info.font = L"Arial";
	text_info.size = 62;
	text_info.color.r = 1;
	text_info.color.g = 1;
	text_info.color.b = 1;
	text_info.color.a = 0.5;
	auto text_area = text_producer::create(frame_factory, 0, 0, L"a", text_info, 1280, 720, false);

	auto text_width = text_area->pixel_constraints().width;
	binding<double> padding(1);
	binding<double> panel_width = padding + text_width + padding;
	binding<double> panel_height = padding + text_area->pixel_constraints().height + padding;

	auto subscription = panel_width.on_change([&]
	{
		CASPAR_LOG(info) << "Panel width: " << panel_width.get();
	});

	padding.set(2);

	auto create_param = [](std::wstring elem) -> std::vector<std::wstring>
	{
		std::vector<std::wstring> result;
		result.push_back(elem);
		return result;
	};

	//auto& car_layer = scene->create_layer(create_producer(frame_factory, format_desc, create_param(L"car")));
	//car_layer.hidden = scene->frame() % 50 > 25 || !(scene->frame() < 1000);
	std::vector<std::wstring> sub_params;
	sub_params.push_back(L"[FREEHAND]");
	sub_params.push_back(L"640");
	sub_params.push_back(L"360");
	scene->create_layer(create_producer(frame_factory, format_desc, sub_params), 10, 10);
	sub_params.clear();

	scene->create_layer(create_producer(frame_factory, format_desc, create_param(L"BLUE")), 110, 10);

	//scene->create_layer(create_producer(frame_factory, format_desc, create_param(L"SP")), 50, 50);

	auto& upper_left = scene->create_layer(create_producer(frame_factory, format_desc, create_param(L"scene/upper_left")));
	auto& upper_right = scene->create_layer(create_producer(frame_factory, format_desc, create_param(L"scene/upper_right")));
	auto& lower_left = scene->create_layer(create_producer(frame_factory, format_desc, create_param(L"scene/lower_left")));
	auto& lower_right = scene->create_layer(create_producer(frame_factory, format_desc, create_param(L"scene/lower_right")));
	auto& text_layer = scene->create_layer(text_area);

	/*
	binding<double> panel_x = (scene->frame()
			.as<double>()
			.transformed([](double v) { return std::sin(v / 20.0); })
			* 20.0
			+ 40.0)
			.transformed([](double v) { return std::floor(v); }); // snap to pixels instead of subpixels
			*/
	tweener tween(L"easeoutbounce");
	binding<double> panel_x(0);

	scene->add_keyframe(panel_x, -panel_width, 0);
	scene->add_keyframe(panel_x, 300.0, 50, L"easeinoutsine");
	scene->add_keyframe(panel_x, 300.0, 50 * 4);
	scene->add_keyframe(panel_x, 1000.0, 50 * 5, L"easeinoutsine");
	//panel_x = delay(panel_x, add_tween(panel_x, scene->frame(), 200.0, int64_t(50), L"linear"), scene->frame(), int64_t(100));
	/*binding<double> panel_x = when(scene->frame() < 50)
		.then(scene->frame().as<double>().transformed([tween](double t) { return tween(t, 0.0, 200, 50); }))
		.otherwise(200.0);*/
	//binding<double> panel_y = when(car_layer.hidden).then(500.0).otherwise(-panel_x + 300);
	binding<double> panel_y(500.0);
	scene->add_keyframe(panel_y, panel_y.get(), 50 * 4);
	scene->add_keyframe(panel_y, 720.0, 50 * 5, L"easeinexpo");
	upper_left.position.x = panel_x;
	upper_left.position.y = panel_y;
	upper_right.position.x = upper_left.position.x + upper_left.producer.get()->pixel_constraints().width + panel_width;
	upper_right.position.y = upper_left.position.y;
	lower_left.position.x = upper_left.position.x;
	lower_left.position.y = upper_left.position.y + upper_left.producer.get()->pixel_constraints().height + panel_height;
	lower_right.position.x = upper_right.position.x;
	lower_right.position.y = lower_left.position.y;
	text_layer.position.x = upper_left.position.x + upper_left.producer.get()->pixel_constraints().width + padding;
	text_layer.position.y = upper_left.position.y + upper_left.producer.get()->pixel_constraints().height + padding + text_area->current_bearing_y().as<double>();

	text_area->text().bind(scene->create_parameter<std::wstring>(L"text"));

	auto params2 = params;
	params2.erase(params2.begin());

	scene->call(params2);

	return scene;
}

}}}
