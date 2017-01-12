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

#include "../../StdAfx.h"

#include <common/future.h>
#include <common/prec_timer.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string.hpp>

#include "scene_producer.h"

#include "../../frame/draw_frame.h"
#include "../../interaction/interaction_aggregator.h"
#include "../text/text_producer.h"

namespace caspar { namespace core { namespace scene {

layer::layer(const std::wstring& name, const spl::shared_ptr<frame_producer>& producer)
	: name(name), producer(producer)
{
	crop.lower_right.x.bind(producer.get()->pixel_constraints().width);
	crop.lower_right.y.bind(producer.get()->pixel_constraints().height);
	perspective.upper_right.x.bind(producer.get()->pixel_constraints().width);
	perspective.lower_right.x.bind(producer.get()->pixel_constraints().width);
	perspective.lower_right.y.bind(producer.get()->pixel_constraints().height);
	perspective.lower_left.y.bind(producer.get()->pixel_constraints().height);
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

mark_action get_mark_action(const std::wstring& name)
{
	if (name == L"start")
		return mark_action::start;
	else if (name == L"stop")
		return mark_action::stop;
	else if (name == L"jump_to")
		return mark_action::jump_to;
	else if (name == L"remove")
		return mark_action::remove;
	else
		CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Invalid mark_action " + name));
}

struct marker
{
	mark_action		action;
	std::wstring	label_argument;

	marker(mark_action action, const std::wstring& label_argument)
		: action(action)
		, label_argument(label_argument)
	{
	}
};

struct scene_producer::impl
{
	std::wstring											producer_name_;
	constraints												pixel_constraints_;
	video_format_desc										format_desc_;
	std::list<layer>										layers_;
	interaction_aggregator									aggregator_;
	binding<double>											frame_number_;
	binding<int64_t>										timeline_frame_number_;
	binding<double>											speed_;
	mutable tbb::atomic<int64_t>							m_x_;
	mutable tbb::atomic<int64_t>							m_y_;
	binding<int64_t>										mouse_x_;
	binding<int64_t>										mouse_y_;
	double													frame_fraction_		= 0.0;
	std::map<void*, timeline>								timelines_;
	std::map<std::wstring, std::shared_ptr<core::variable>>	variables_;
	std::vector<std::wstring>								variable_names_;
	std::multimap<int64_t, marker>							markers_by_frame_;
	monitor::subject										monitor_subject_;
	bool													paused_				= true;
	bool													removed_			= false;
	bool													going_to_mark_		= false;

	impl(std::wstring producer_name, int width, int height, const video_format_desc& format_desc)
		: producer_name_(std::move(producer_name))
		, pixel_constraints_(width, height)
		, format_desc_(format_desc)
		, aggregator_([=] (double x, double y) { return collission_detect(x, y); })
	{
		auto speed_variable = std::make_shared<core::variable_impl<double>>(L"1.0", true, 1.0);
		store_variable(L"scene_speed", speed_variable);
		speed_ = speed_variable->value();

		auto frame_variable = std::make_shared<core::variable_impl<double>>(L"-1", true, -1);
		store_variable(L"frame", frame_variable);
		frame_number_ = frame_variable->value();

		auto timeline_frame_variable = std::make_shared<core::variable_impl<int64_t>>(L"-1", false, -1);
		store_variable(L"timeline_frame", timeline_frame_variable);
		timeline_frame_number_ = timeline_frame_variable->value();

		auto mouse_x_variable = std::make_shared<core::variable_impl<int64_t>>(L"0", false, 0);
		auto mouse_y_variable = std::make_shared<core::variable_impl<int64_t>>(L"0", false, 0);
		store_variable(L"mouse_x", mouse_x_variable);
		store_variable(L"mouse_y", mouse_y_variable);
		mouse_x_ = mouse_x_variable->value();
		mouse_y_ = mouse_y_variable->value();
		m_x_ = 0;
		m_y_ = 0;
	}

	layer& create_layer(
			const spl::shared_ptr<frame_producer>& producer, int x, int y, const std::wstring& name)
	{
		layer layer(name, producer);

		layer.position.x.set(x);
		layer.position.y.set(y);

		layers_.push_back(layer);

		return layers_.back();
	}

	void reverse_layers() {
		layers_.reverse();
	}

	void store_keyframe(void* timeline_identity, const keyframe& k)
	{
		timelines_[timeline_identity].keyframes.insert(std::make_pair(k.destination_frame, k));
	}

	void store_variable(
			const std::wstring& name, const std::shared_ptr<core::variable>& var)
	{
		variables_.insert(std::make_pair(name, var));
		variable_names_.push_back(name);
	}

	void add_mark(int64_t frame, mark_action action, const std::wstring& label)
	{
		markers_by_frame_.insert(std::make_pair(frame, marker(action, label)));
	}

	core::variable& get_variable(const std::wstring& name)
	{
		auto found = variables_.find(name);

		if (found == variables_.end())
			CASPAR_THROW_EXCEPTION(user_error() << msg_info(name + L" not found in scene"));

		return *found->second;
	}

	const std::vector<std::wstring>& get_variables() const
	{
		return variable_names_;
	}

	binding<int64_t> timeline_frame()
	{
		return timeline_frame_number_;
	}

	frame_transform get_transform(const layer& layer) const
	{
		frame_transform transform;

		auto& anchor		= transform.image_transform.anchor;
		auto& pos			= transform.image_transform.fill_translation;
		auto& scale			= transform.image_transform.fill_scale;
		auto& angle			= transform.image_transform.angle;
		auto& crop			= transform.image_transform.crop;
		auto& pers			= transform.image_transform.perspective;

		anchor[0]	= layer.anchor.x.get()										/ layer.producer.get()->pixel_constraints().width.get();
		anchor[1]	= layer.anchor.y.get()										/ layer.producer.get()->pixel_constraints().height.get();
		pos[0]		= layer.position.x.get()									/ pixel_constraints_.width.get();
		pos[1]		= layer.position.y.get()									/ pixel_constraints_.height.get();
		scale[0]	= layer.producer.get()->pixel_constraints().width.get()		/ pixel_constraints_.width.get();
		scale[1]	= layer.producer.get()->pixel_constraints().height.get()	/ pixel_constraints_.height.get();
		crop.ul[0]	= layer.crop.upper_left.x.get()								/ layer.producer.get()->pixel_constraints().width.get();
		crop.ul[1]	= layer.crop.upper_left.y.get()								/ layer.producer.get()->pixel_constraints().height.get();
		crop.lr[0]	= layer.crop.lower_right.x.get()							/ layer.producer.get()->pixel_constraints().width.get();
		crop.lr[1]	= layer.crop.lower_right.y.get()							/ layer.producer.get()->pixel_constraints().height.get();
		pers.ul[0]	= layer.perspective.upper_left.x.get()						/ layer.producer.get()->pixel_constraints().width.get();
		pers.ul[1]	= layer.perspective.upper_left.y.get()						/ layer.producer.get()->pixel_constraints().height.get();
		pers.ur[0]	= layer.perspective.upper_right.x.get()						/ layer.producer.get()->pixel_constraints().width.get();
		pers.ur[1]	= layer.perspective.upper_right.y.get()						/ layer.producer.get()->pixel_constraints().height.get();
		pers.lr[0]	= layer.perspective.lower_right.x.get()						/ layer.producer.get()->pixel_constraints().width.get();
		pers.lr[1]	= layer.perspective.lower_right.y.get()						/ layer.producer.get()->pixel_constraints().height.get();
		pers.ll[0]	= layer.perspective.lower_left.x.get()						/ layer.producer.get()->pixel_constraints().width.get();
		pers.ll[1]	= layer.perspective.lower_left.y.get()						/ layer.producer.get()->pixel_constraints().height.get();

		static const double PI = 3.141592653589793;

		angle		= layer.rotation.get() * PI / 180.0;

		transform.image_transform.opacity				= layer.adjustments.opacity.get();
		transform.image_transform.is_key				= layer.is_key.get();
		transform.image_transform.use_mipmap			= layer.use_mipmap.get();
		transform.image_transform.blend_mode			= layer.blend_mode.get();
		transform.image_transform.chroma.enable			= layer.chroma_key.enable.get();
		transform.image_transform.chroma.target_hue		= layer.chroma_key.target_hue.get();
		transform.image_transform.chroma.hue_width		= layer.chroma_key.hue_width.get();
		transform.image_transform.chroma.min_saturation	= layer.chroma_key.min_saturation.get();
		transform.image_transform.chroma.min_brightness	= layer.chroma_key.min_brightness.get();
		transform.image_transform.chroma.softness		= layer.chroma_key.softness.get();
		transform.image_transform.chroma.spill			= layer.chroma_key.spill.get();
		transform.image_transform.chroma.spill_darken	= layer.chroma_key.spill_darken.get();

		// Mark as sublayer, so it will be composited separately by the mixer.
		transform.image_transform.layer_depth = 1;

		return transform;
	}

	boost::optional<std::pair<int64_t, marker>> find_first_stop_or_jump_or_remove(int64_t start_frame, int64_t end_frame)
	{
		auto lower = markers_by_frame_.lower_bound(start_frame);
		auto upper = markers_by_frame_.upper_bound(end_frame);

		if (lower == markers_by_frame_.end())
			return boost::none;

		for (auto iter = lower; iter != upper; ++iter)
		{
			auto action = iter->second.action;

			if (action == mark_action::stop || action == mark_action::jump_to || action == mark_action::remove)
				return std::make_pair(iter->first, iter->second);
		}

		return boost::none;
	}

	boost::optional<std::pair<int64_t, marker>> find_first_start(int64_t start_frame)
	{
		auto lower = markers_by_frame_.lower_bound(start_frame);

		if (lower == markers_by_frame_.end())
			return boost::none;

		for (auto iter = lower; iter != markers_by_frame_.end(); ++iter)
		{
			auto action = iter->second.action;

			if (action == mark_action::start)
				return std::make_pair(iter->first, iter->second);
		}

		return boost::none;
	}

	draw_frame render_frame()
	{
		if (format_desc_.field_count == 1)
			return render_progressive_frame();
		else
		{
			prec_timer timer;
			timer.tick_millis(0);

			auto field1 = render_progressive_frame();

			timer.tick(0.5 / format_desc_.fps);

			auto field2 = render_progressive_frame();

			return draw_frame::interlace(field1, field2, format_desc_.field_mode);
		}
	}

	void advance()
	{
		frame_fraction_ += speed_.get();

		if (std::abs(frame_fraction_) >= 1.0)
		{
			int64_t delta = static_cast<int64_t>(frame_fraction_);
			auto previous_frame = timeline_frame_number_.get();
			auto next_frame = timeline_frame_number_.get() + delta;
			auto marker = find_first_stop_or_jump_or_remove(previous_frame + 1, next_frame);

			if (marker && marker->second.action == mark_action::remove)
			{
				remove();
			}
			if (marker && !going_to_mark_)
			{
				if (marker->second.action == mark_action::stop)
				{
					timeline_frame_number_.set(marker->first);
					frame_fraction_ = 0.0;
					paused_ = true;
				}
				else if (marker->second.action == mark_action::jump_to)
				{
					go_to_marker(marker->second.label_argument, 0);
				}
			}
			else
			{
				timeline_frame_number_.set(next_frame);
				frame_fraction_ -= delta;
			}

			going_to_mark_ = false;
		}
	}

	draw_frame render_progressive_frame()
	{
		if (removed_)
			return draw_frame::empty();

		mouse_x_.set(m_x_);
		mouse_y_.set(m_y_);

		if (!paused_)
			advance();

		frame_number_.set(frame_number_.get() + speed_.get());

		for (auto& timeline : timelines_)
			timeline.second.on_frame(timeline_frame_number_.get());

		std::vector<draw_frame> frames;

		for (auto& layer : layers_)
		{
			if (layer.hidden.get())
				continue;

			draw_frame frame(layer.producer.get()->receive());
			frame.transform() = get_transform(layer);
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
		m_x_ = static_cast<int64_t>(x * pixel_constraints_.width.get());
		m_y_ = static_cast<int64_t>(y * pixel_constraints_.height.get());

		return static_cast<bool>((collission_detect(x, y)));
	}

	boost::optional<interaction_target> collission_detect(double x, double y) const
	{
		for (auto& layer : layers_ | boost::adaptors::reversed)
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
				return std::make_pair(transform, static_cast<interaction_sink*>(layer.producer.get().get()));
			}
		}

		return boost::optional<interaction_target>();
	}

	std::future<std::wstring> call(const std::vector<std::wstring>& params)
	{
		if (!params.empty() && boost::ends_with(params.at(0), L"()"))
			return make_ready_future(handle_call(params));
		else
			return make_ready_future(handle_variable_set(params));
	}

	std::wstring handle_variable_set(const std::vector<std::wstring>& params)
	{
		for (int i = 0; i + 1 < params.size(); i += 2)
		{
			auto found = variables_.find(boost::to_lower_copy(params.at(i)));

			if (found != variables_.end() && found->second->is_public())
				found->second->from_string(params.at(i + 1));
		}

		return L"";
	}

	std::wstring handle_call(const std::vector<std::wstring>& params)
	{
		auto call = params.at(0);

		if (call == L"play()")
			go_to_marker(params.at(1), -1);
		else if (call == L"remove()")
			remove();
		else if (call == L"next()")
			next();
		else
			CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Unknown call " + call));

		return L"";
	}

	void remove()
	{
		removed_ = true;
		layers_.clear();
	}

	void next()
	{
		auto marker = find_first_start(timeline_frame_number_.get() + 1);

		if (marker)
		{
			timeline_frame_number_.set(marker->first - 1);
			frame_fraction_ = 0.0;
			paused_ = false;
			going_to_mark_ = true;
		}
		else
		{
			remove();
		}
	}

	void go_to_marker(const std::wstring& marker_name, int64_t offset)
	{
		for (auto& marker : markers_by_frame_)
		{
			if (marker.second.label_argument == marker_name && marker.second.action == mark_action::start)
			{
				timeline_frame_number_.set(marker.first + offset);
				frame_fraction_ = 0.0;
				paused_ = false;
				going_to_mark_ = true;

				return;
			}
		}

		if (marker_name == L"intro")
		{
			timeline_frame_number_.set(offset);
			frame_fraction_ = 0.0;
			paused_ = false;
			going_to_mark_ = true;
		}
		else if (marker_name == L"outro")
		{
			remove();
		}
		else
			CASPAR_LOG(info) << print() << L" no marker called " << marker_name << " found";
	}

	std::wstring print() const
	{
		return L"scene[type=" + name() + L"]";
	}

	std::wstring name() const
	{
		return producer_name_;
	}

	boost::property_tree::wptree info() const
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"scene");
		info.add(L"producer-name", name());
		info.add(L"frame-number", frame_number_.get());
		info.add(L"timeline-frame-number", timeline_frame_number_.get());

		for (auto& var : variables_)
		{
			boost::property_tree::wptree variable_info;

			variable_info.add(L"name", var.first);
			variable_info.add(L"public", var.second->is_public());
			variable_info.add(L"value", var.second->to_string());

			info.add_child(L"variables.variable", variable_info);
		}

		for (auto& layer : layers_)
		{
			boost::property_tree::wptree layer_info;

			layer_info.add(L"name", layer.name.get());
			layer_info.add_child(L"producer", layer.producer.get()->info());
			layer_info.add(L"x", layer.position.x.get());
			layer_info.add(L"y", layer.position.y.get());
			layer_info.add(L"width", layer.producer.get()->pixel_constraints().width.get());
			layer_info.add(L"height", layer.producer.get()->pixel_constraints().height.get());

			info.add_child(L"layers.layer", layer_info);
		}

		return info;
	}

	monitor::subject& monitor_output()
	{
		return monitor_subject_;
	}
};

scene_producer::scene_producer(std::wstring producer_name, int width, int height, const video_format_desc& format_desc)
	: impl_(new impl(std::move(producer_name), width, height, format_desc))
{
}

scene_producer::~scene_producer()
{
}

layer& scene_producer::create_layer(
		const spl::shared_ptr<frame_producer>& producer, int x, int y, const std::wstring& name)
{
	return impl_->create_layer(producer, x, y, name);
}

layer& scene_producer::create_layer(
		const spl::shared_ptr<frame_producer>& producer, const std::wstring& name)
{
	return impl_->create_layer(producer, 0, 0, name);
}

void scene_producer::reverse_layers() {
	impl_->reverse_layers();
}

binding<int64_t> scene_producer::timeline_frame()
{
	return impl_->timeline_frame();
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

std::future<std::wstring> scene_producer::call(const std::vector<std::wstring>& params)
{
	return impl_->call(params);
}

monitor::subject& scene_producer::monitor_output()
{
	return impl_->monitor_output();
}

void scene_producer::store_keyframe(void* timeline_identity, const keyframe& k)
{
	impl_->store_keyframe(timeline_identity, k);
}

void scene_producer::store_variable(
		const std::wstring& name, const std::shared_ptr<core::variable>& var)
{
	impl_->store_variable(name, var);
}

void scene_producer::add_mark(int64_t frame, mark_action action, const std::wstring& label)
{
	impl_->add_mark(frame, action, label);
}

core::variable& scene_producer::get_variable(const std::wstring& name)
{
	return impl_->get_variable(name);
}

const std::vector<std::wstring>& scene_producer::get_variables() const
{
	return impl_->get_variables();
}

}}}
