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

#include "scene_producer.h"

#include "../../frame/draw_frame.h"
#include "../../interaction/interaction_aggregator.h"

namespace caspar { namespace core { namespace scene {

layer::layer(const spl::shared_ptr<frame_producer>& producer)
	: producer(producer)
{
}

adjustments::adjustments()
	: opacity(1.0)
{
}

struct scene_producer::impl
{
	constraints pixel_constraints_;
	std::vector<layer> layers_;
	interaction_aggregator aggregator_;

	impl(int width, int height)
		: pixel_constraints_(width, height)
		, aggregator_([=] (double x, double y) { return collission_detect(x, y); })
	{
	}

	layer& create_layer(
			const spl::shared_ptr<frame_producer>& producer, int x, int y)
	{
		layer layer(producer);

		layer.position.x.set(x);
		layer.position.y.set(y);

		layers_.push_back(layer);

		return layers_.back();
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
		std::vector<draw_frame> frames;

		BOOST_FOREACH(auto& layer, layers_)
		{
			draw_frame frame(layer.producer.get()->receive());
			frame.transform() = get_transform(layer);;
			frames.push_back(frame);
		}

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

void scene_producer::subscribe(const monitor::observable::observer_ptr& o)
{
	impl_->subscribe(o);
}

void scene_producer::unsubscribe(const monitor::observable::observer_ptr& o)
{
	impl_->unsubscribe(o);
}

spl::shared_ptr<frame_producer> create_dummy_scene_producer(const spl::shared_ptr<class frame_factory>& frame_factory, const video_format_desc& format_desc, const std::vector<std::wstring>& params)
{
	if (params.size() < 1 || params.at(0) != L"[SCENE]")
		return core::frame_producer::empty();

	auto scene = spl::make_shared<scene_producer>(160, 90);

	std::vector<std::wstring> sub_params;

	binding<int> text_width(10);
	binding<int> padding(1);
	binding<int> panel_width = padding + text_width + padding;

	auto subscription = panel_width.on_change([&]
	{
		CASPAR_LOG(info) << "Panel width: " << panel_width.get();
	});

	text_width.set(20);
	text_width.set(10);
	padding.set(2);
	text_width.set(20);

	sub_params.push_back(L"[FREEHAND]");
	sub_params.push_back(L"100");
	sub_params.push_back(L"50");
	scene->create_layer(create_producer(frame_factory, format_desc, sub_params), 10, 10);
	sub_params.clear();

	sub_params.push_back(L"BLUE");
	scene->create_layer(create_producer(frame_factory, format_desc, sub_params), 110, 10);
	sub_params.clear();

	sub_params.push_back(L"SP");
	scene->create_layer(create_producer(frame_factory, format_desc, sub_params), 50, 50);
	sub_params.clear();

	return scene;
}

}}}
