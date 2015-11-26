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

#pragma once

#include <common/log.h>

#include "../frame_producer.h"
#include "../../fwd.h"

#include "../binding.h"
#include "../variable.h"

namespace caspar { namespace core { namespace scene {

struct coord
{
	binding<double> x;
	binding<double> y;
};

struct rect
{
	coord upper_left;
	coord lower_right;
};

struct corners
{
	coord upper_left;
	coord upper_right;
	coord lower_right;
	coord lower_left;
};

struct adjustments
{
	binding<double> opacity;

	adjustments();
};

struct chroma_key
{
	binding<core::chroma::type>	key;
	binding<double>				threshold;
	binding<double>				softness;
	binding<double>				spill;
};

struct layer
{
	binding<std::wstring>						name;
	scene::coord								anchor;
	scene::coord								position;
	scene::rect									crop;
	scene::corners								perspective;
	binding<double>								rotation;
	scene::adjustments							adjustments;
	binding<spl::shared_ptr<frame_producer>>	producer;
	binding<bool>								hidden;
	binding<bool>								is_key;
	binding<bool>								use_mipmap;
	binding<core::blend_mode>					blend_mode;
	scene::chroma_key							chroma_key;

	explicit layer(const std::wstring& name, const spl::shared_ptr<frame_producer>& producer);
};

struct keyframe
{
	std::function<void ()>												on_start_animate;
	std::function<void (int64_t start_frame, int64_t current_frame)>	on_animate_to;
	std::function<void ()>												on_destination_frame;
	int64_t																destination_frame;
public:
	keyframe(int64_t destination_frame)
		: destination_frame(destination_frame)
	{
	}
};

enum class mark_action
{
	start,
	stop,
	jump_to,
	remove
};

mark_action get_mark_action(const std::wstring& name);

class scene_producer : public frame_producer_base
{
public:
	scene_producer(std::wstring producer_name, int width, int height, const video_format_desc& format_desc);
	~scene_producer();

	draw_frame receive_impl() override;
	constraints& pixel_constraints() override;
	void on_interaction(const interaction_event::ptr& event) override;
	bool collides(double x, double y) const override;
	std::wstring print() const override;
	std::wstring name() const override;
	std::future<std::wstring> call(const std::vector<std::wstring>& params) override;
	boost::property_tree::wptree info() const override;
	monitor::subject& monitor_output();

	layer& create_layer(
			const spl::shared_ptr<frame_producer>& producer, int x, int y, const std::wstring& name);
	layer& create_layer(
			const spl::shared_ptr<frame_producer>& producer, const std::wstring& name);
	void reverse_layers();

	binding<int64_t> frame();
	binding<double> speed();

	template<typename T> binding<T>& create_variable(
			const std::wstring& name, bool is_public, const std::wstring& expr = L"")
	{
		std::shared_ptr<core::variable> var =
				std::make_shared<core::variable_impl<T>>(expr, is_public);

		store_variable(name, var);

		return var->as<T>();
	}

	template<typename T>
	void add_keyframe(
			binding<T>& to_affect,
			T destination_value,
			int64_t at_frame,
			const std::wstring& easing)
	{
		add_keyframe(to_affect, binding<T>(destination_value), at_frame, easing);
	}

	template<typename T>
	void add_keyframe(
			binding<T>& to_affect,
			const binding<T>& destination_value,
			int64_t at_frame,
			const std::wstring& easing)
	{
		if (easing.empty())
		{
			add_keyframe(to_affect, destination_value, at_frame);
			return;
		}

		tweener tween(easing);
		keyframe k(at_frame);

		std::shared_ptr<T> start_value(new T);

		k.on_start_animate = [=]() mutable
		{
			*start_value = to_affect.get();
			to_affect.unbind();
		};

		k.on_destination_frame = [=]() mutable
		{
			to_affect.bind(destination_value);
		};

		k.on_animate_to =
				[=](int64_t start_frame, int64_t current_frame) mutable
				{
					auto relative_frame = current_frame - start_frame;
					auto duration = at_frame - start_frame;
					auto tweened = static_cast<T>(tween(
							static_cast<double>(relative_frame),
							*start_value,
							destination_value.get() - *start_value,
							static_cast<double>(duration)));

					to_affect.set(tweened);
					
					//CASPAR_LOG(info) << relative_frame << L" " << *start_value << L" " << duration << L" " << tweened;
				};

		store_keyframe(to_affect.identity(), k);
	}

	template<typename T>
	void add_keyframe(binding<T>& to_affect, T set_value, int64_t at_frame)
	{
		add_keyframe(to_affect, binding<T>(set_value), at_frame);
	}

	template<typename T>
	void add_keyframe(binding<T>& to_affect, const binding<T>& set_value, int64_t at_frame)
	{
		keyframe k(at_frame);

		k.on_destination_frame = [=]() mutable
		{
			to_affect.bind(set_value);
		};

		store_keyframe(to_affect.identity(), k);
	}

	void add_mark(int64_t frame, mark_action action, const std::wstring& label);

	core::variable& get_variable(const std::wstring& name) override;
	const std::vector<std::wstring>& get_variables() const override;
private:
	void store_keyframe(void* timeline_identity, const keyframe& k);
	void store_variable(
			const std::wstring& name, const std::shared_ptr<core::variable>& var);

	struct impl;
	std::unique_ptr<impl> impl_;
};

}}}
