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

#include "../frame_producer.h"

#include "../binding.h"

namespace caspar { namespace core { namespace scene {

struct coord
{
	binding<double> x;
	binding<double> y;
};

struct adjustments
{
	binding<double> opacity;

	adjustments();
};

struct layer
{
	binding<std::wstring> name;
	coord position;
	adjustments adjustments;
	binding<spl::shared_ptr<frame_producer>> producer;
	binding<bool> hidden;
	binding<bool> is_key;

	explicit layer(const spl::shared_ptr<frame_producer>& producer);
	layer(const std::wstring& name, const spl::shared_ptr<frame_producer>& producer);
};

struct keyframe
{
	std::function<void ()> on_start_animate;
	std::function<void (int64_t start_frame, int64_t current_frame)> on_animate_to;
	std::function<void ()> on_destination_frame;
	int64_t destination_frame;
public:
	keyframe(int64_t destination_frame)
		: destination_frame(destination_frame)
	{
	}
};

template<typename T> class parameter_holder;

class parameter_holder_base
{
public:
	virtual ~parameter_holder_base()
	{
	}

	virtual void set(const std::wstring& raw_value) = 0;

	template<typename T>
	binding<T>& value()
	{
		return dynamic_cast<parameter_holder<T>>(*this).value();
	}
};

template<typename T>
class parameter_holder : public parameter_holder_base
{
	binding<T> value_;
public:
	parameter_holder(T initial_value)
		: value_(initial_value)
	{
	}

	binding<T>& value()
	{
		return value_;
	}

	virtual void set(const std::wstring& raw_value)
	{
		value_.set(boost::lexical_cast<T>(raw_value));
	}
};

class scene_producer : public frame_producer_base
{
public:
	scene_producer(int width, int height);
	~scene_producer();

	class draw_frame receive_impl() override;
	constraints& pixel_constraints() override;
	void on_interaction(const interaction_event::ptr& event) override;
	bool collides(double x, double y) const override;
	std::wstring print() const override;
	std::wstring name() const override;
	boost::unique_future<std::wstring>	call(const std::vector<std::wstring>& params) override;
	boost::property_tree::wptree info() const override;
	void subscribe(const monitor::observable::observer_ptr& o) override;
	void unsubscribe(const monitor::observable::observer_ptr& o) override;
	layer& create_layer(
			const spl::shared_ptr<frame_producer>& producer, int x, int y);
	layer& create_layer(
			const spl::shared_ptr<frame_producer>& producer, int x, int y, const std::wstring& name);
	layer& create_layer(const spl::shared_ptr<frame_producer>& producer);
	binding<int64_t> frame();

	template<typename T> binding<T>& create_parameter(const std::wstring& name, T initial_value = T())
	{
		auto param = std::make_shared<parameter_holder<T>>(initial_value);

		store_parameter(name, param);

		return param->value();
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
					
					CASPAR_LOG(info) << relative_frame << L" " << *start_value << L" " << duration << L" " << tweened;
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
private:
	void store_parameter(
			const std::wstring& name,
			const std::shared_ptr<parameter_holder_base>& param);
	void store_keyframe(void* timeline_identity, const keyframe& k);

	struct impl;
	std::unique_ptr<impl> impl_;
};

spl::shared_ptr<frame_producer> create_dummy_scene_producer(const spl::shared_ptr<class frame_factory>& frame_factory, const video_format_desc& format_desc, const std::vector<std::wstring>& params);

}}}
