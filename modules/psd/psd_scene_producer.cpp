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
* Author: Niklas P Andersson, niklas.p.andersson@svt.se
*/

#include "psd_scene_producer.h"
#include "psd_document.h"
#include "layer.h"

#include <core/frame/pixel_format.h>
#include <core/frame/frame_factory.h>
#include <core/frame/audio_channel_layout.h>
#include <core/producer/frame_producer.h>
#include <core/producer/color/color_producer.h>
#include <core/producer/text/text_producer.h>
#include <core/producer/scene/scene_cg_proxy.h>
#include <core/producer/scene/scene_producer.h>
#include <core/producer/scene/const_producer.h>
#include <core/producer/scene/hotswap_producer.h>
#include <core/producer/media_info/media_info.h>
#include <core/frame/draw_frame.h>
#include <core/help/help_repository.h>
#include <core/help/help_sink.h>

#include <common/env.h>
#include <common/memory.h>
#include <common/log.h>
#include <common/os/filesystem.h>

#include <boost/filesystem.hpp>
#include <boost/thread/future.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/rational.hpp>

#include <future>

namespace caspar { namespace psd {

core::text::text_info get_text_info(const boost::property_tree::wptree& ptree)
{
	core::text::text_info result;
	int font_index = ptree.get(L"EngineDict.StyleRun.RunArray..StyleSheet.StyleSheetData.Font", 0);
	result.size = ptree.get(L"EngineDict.StyleRun.RunArray..StyleSheet.StyleSheetData.FontSize", 30.0f);
	//result.leading = ptree.get(L"EngineDict.StyleRun.RunArray..StyleSheet.StyleSheetData.Leading", 0);
	result.tracking = ptree.get(L"EngineDict.StyleRun.RunArray..StyleSheet.StyleSheetData.Tracking", 0);
	result.baseline_shift = ptree.get(L"EngineDict.StyleRun.RunArray..StyleSheet.StyleSheetData.BaselineShift", 0);
	//result.kerning = ptree.get(L"EngineDict.StyleRun.RunArray..StyleSheet.StyleSheetData.Kerning", 0);

	int child_index = 0;
	auto color_node = ptree.get_child_optional(L"EngineDict.StyleRun.RunArray..StyleSheet.StyleSheetData.FillColor.Values");

	if (color_node)
	{
		for(auto it = color_node->begin(); it != color_node->end(); ++it, ++child_index)
		{
			auto& value_node = (*it).second;
			float value = value_node.get_value(0.0f);
			switch(child_index)
			{
			case 0: result.color.a = value; break;
			case 1: result.color.r = value; break;
			case 2: result.color.g = value; break;
			case 3: result.color.b = value; break;
			}
		}
	}

	//find fontname
	child_index = 0;
	auto fontset_node = ptree.get_child(L"ResourceDict.FontSet");
	for(auto it = fontset_node.begin(); it != fontset_node.end(); ++it, ++child_index)
	{
		auto& font_node = (*it).second;
		if(child_index == font_index)
		{
			result.font = font_node.get(L"Name", L"");
			break;
		}
	}

	return result;
}


class dependency_resolver
{
	struct layer_record
	{
		layer_record() : layer(nullptr), tags(layer_tag::none), adjustment_x(0), adjustment_y(0), vector_mask(false)
		{}
		layer_record(caspar::core::scene::layer* l, caspar::psd::layer_tag t, double x, double y, bool v) :
			layer(l), tags(t), adjustment_x(x), adjustment_y(y), vector_mask(v)
		{}

		caspar::core::scene::layer* layer;
		layer_tag tags;
		double adjustment_x;
		double adjustment_y;
		bool vector_mask;
	};

	std::list<layer_record> layers;
	layer_record master;
	
	spl::shared_ptr<core::scene::scene_producer> scene_;
	bool is_root_;

public:
	dependency_resolver(const spl::shared_ptr<core::scene::scene_producer>& s) :
		scene_(s), is_root_(false)
	{}
	dependency_resolver(const spl::shared_ptr<core::scene::scene_producer>& s, bool root) :
		scene_(s), is_root_(root)
	{}

	spl::shared_ptr<core::scene::scene_producer> scene() { return scene_; }

	void add(caspar::core::scene::layer* layer, layer_tag tags, double adjustment_x, double adjustment_y, bool vector_mask)
	{
		layer_record rec{ layer, tags, adjustment_x, adjustment_y, vector_mask };
		layers.push_back(rec);

		//if the layer is either explicitly tagged as dynamic or at least not tagged as static/rasterized we should try to set it as master
		if((rec.tags & layer_tag::explicit_dynamic) == layer_tag::explicit_dynamic || (rec.tags & layer_tag::rasterized) == layer_tag::none)
		{
			//if we don't have a master already, just assign this
			if (master.layer == nullptr)
				master = rec;
			else if ((rec.tags & layer_tag::explicit_dynamic) == layer_tag::explicit_dynamic) 
			{
				if((master.tags & layer_tag::explicit_dynamic) == layer_tag::none) 
				{
					//if we have a master that's not explicitly tagged as dynamic but this layer is; use this as master
					master = rec;
				}
				else {
					//we have multiple layers that're tagged as dynamic, warn that the first one will be used
					CASPAR_LOG(warning) << "Multiple explicitly dynamic layers in the same group. The first one will be used";
				}
			}
		}
	}

	void calculate()
	{
		if (layers.size() == 0)
			return;

		if (master.layer == nullptr) {
			CASPAR_LOG(warning) << "Dependent layers without any dynamic layer";
			return;
		}

		for (auto &r : layers)
		{
			if (r.layer != master.layer)
			{
				//x-coords
				double slave_left = r.layer->position.x.get();
				double master_left = master.layer->position.x.get();
				double slave_right = slave_left + r.layer->producer.get()->pixel_constraints().width.get();
				double master_right = master_left + master.layer->producer.get()->pixel_constraints().width.get();

				double slave_width = slave_right - slave_left;
				double master_width = master_right - master_left;

				double offset_start_x = slave_left - master_left;
				double offset_end_x = slave_left - master_right;

				if((r.tags & layer_tag::moveable) == layer_tag::moveable)
					r.layer->position.x.bind(master.layer->position.x + master.layer->producer.get()->pixel_constraints().width + offset_end_x + r.adjustment_x);
				else if ((r.tags & layer_tag::resizable) == layer_tag::resizable) {
					r.layer->position.x.bind(master.layer->position.x + offset_start_x + r.adjustment_x);

					if(r.vector_mask)	//adjust crop-rect if we have a vector-mask
						r.layer->crop.lower_right.x.bind(master.layer->producer.get()->pixel_constraints().width + ((r.layer->crop.lower_right.x.get() - r.layer->crop.upper_left.x.get()) - master_width));
					else //just stretch the content
						r.layer->producer.get()->pixel_constraints().width.bind(master.layer->producer.get()->pixel_constraints().width + (slave_width - master_width));
				}

				//no support for changes in the y-direction yet
			}
		}
	}
};

int64_t get_frame_number(
		const core::video_format_desc& format_desc,
		const boost::rational<int>& at_second)
{
	return static_cast<int64_t>(
			boost::rational_cast<double>(at_second) * format_desc.fps);
}

boost::rational<int> get_rational(const boost::property_tree::wptree& node)
{
	return boost::rational<int>(
			node.get<int>(L"numerator"), node.get<int>(L"denominator"));
}

void create_marks_from_comments(
		const spl::shared_ptr<core::scene::scene_producer>& scene,
		const core::video_format_desc& format_desc,
		const boost::property_tree::wptree& comment_track)
{
	auto keylist = comment_track.get_child_optional(L"keyList");

	if (!keylist)
		return;

	for (auto& key : *keylist)
	{
		auto time = get_rational(key.second.get_child(L"time"));
		auto frame = get_frame_number(format_desc, time);
		auto text = key.second.get<std::wstring>(L"animKey.Vl  ");
		std::vector<std::wstring> marks;
		boost::split(marks, text, boost::is_any_of(","));

		for (auto& mark : marks)
		{
			boost::trim_if(mark, [](wchar_t c) { return isspace(c) || c == 0; });

			std::vector<std::wstring> args;
			boost::split(args, mark, boost::is_any_of(" \t"), boost::algorithm::token_compress_on);

			scene->add_mark(frame, core::scene::get_mark_action(args.at(0)), args.size() == 1 ? L"" : args.at(1));
		}
	}
}

void create_marks(
		const spl::shared_ptr<core::scene::scene_producer>& scene,
		const core::video_format_desc& format_desc,
		const boost::property_tree::wptree& global_timeline)
{
	auto time = get_rational(global_timeline.get_child(L"duration"));
	auto remove_at_frame = get_frame_number(format_desc, time);

	scene->add_mark(remove_at_frame, core::scene::mark_action::remove, L"");

	auto tracklist = global_timeline.get_child_optional(L"globalTrackList");

	if (!tracklist)
		return;

	for (auto& track : *tracklist)
	{
		auto track_id = track.second.get<std::wstring>(L"stdTrackID");

		if (track_id == L"globalCommentTrack")
			create_marks_from_comments(scene, format_desc, track.second);
	}
}

void create_timelines(
		const spl::shared_ptr<core::scene::scene_producer>& scene,
		const core::video_format_desc& format_desc,
		core::scene::layer& layer,
		const layer_ptr& psd_layer,
		double adjustment_x,
		double adjustment_y)
{
	auto timeline = psd_layer->timeline_data();
	auto start = get_rational(timeline.get_child(L"timeScope.Strt"));
	auto end_offset = get_rational(timeline.get_child(L"timeScope.outTime"));
	auto end = start + end_offset;
	auto start_frame = get_frame_number(format_desc, start);
	auto end_frame = get_frame_number(format_desc, end);

	layer.hidden = scene->frame() < start_frame || scene->frame() > end_frame;

	auto tracklist = timeline.get_child_optional(L"trackList");

	if (!tracklist)
		return;

	double original_pos_x = psd_layer->location().x;
	double original_pos_y = psd_layer->location().y;

	for (auto& track : *tracklist)
	{
		auto track_id = track.second.get<std::wstring>(L"stdTrackID");

		if (track_id == L"sheetPositionTrack")
		{
			for (auto& key : track.second.get_child(L"keyList"))
			{
				bool tween = key.second.get<std::wstring>(L"animInterpStyle")
						== L"Lnr ";
				auto time = get_rational(key.second.get_child(L"time"));
				auto hrzn = key.second.get<double>(L"animKey.Hrzn");
				auto vrtc = key.second.get<double>(L"animKey.Vrtc");
				auto x = original_pos_x + hrzn + adjustment_x;
				auto y = original_pos_y + vrtc + adjustment_y;
				auto frame = get_frame_number(format_desc, time);

				if (frame == 0) // Consider as initial value (rewind)
				{
					layer.position.x.set(x);
					layer.position.y.set(y);
				}

				frame = start_frame + frame; // translate to global timeline

				if (tween)
				{
					scene->add_keyframe(layer.position.x, x, frame, L"easeOutSine");
					scene->add_keyframe(layer.position.y, y, frame, L"easeOutSine");
				}
				else
				{
					scene->add_keyframe(layer.position.x, x, frame);
					scene->add_keyframe(layer.position.y, y, frame);
				}
			}
		}
		else if (track_id == L"opacityTrack")
		{
			auto& opacity = layer.adjustments.opacity;

			for (auto& key : track.second.get_child(L"keyList"))
			{
				bool tween = key.second.get<std::wstring>(L"animInterpStyle")
						== L"Lnr ";
				auto time = get_rational(key.second.get_child(L"time"));
				auto opct = key.second.get<double>(L"animKey.Opct.#Prc") / 100.0;
				auto frame = get_frame_number(format_desc, time);

				if (frame == 0) // Consider as initial value (rewind)
					opacity.set(opct);

				frame = start_frame + frame; // translate to global timeline

				if (tween)
					scene->add_keyframe(opacity, opct, frame, L"easeOutSine");
				else
					scene->add_keyframe(opacity, opct, frame);
			}
		}
		else
		{
			//ignore other kinds of tracks for now
		}
	}
}

spl::shared_ptr<core::frame_producer> create_psd_scene_producer(const core::frame_producer_dependencies& dependencies, const std::vector<std::wstring>& params)
{
	std::wstring filename = env::template_folder() + params.at(0) + L".psd";
	auto found_file = find_case_insensitive(filename);

	if (!found_file)
		return core::frame_producer::empty();

	psd_document doc;
	doc.parse(*found_file);

	auto root = spl::make_shared<core::scene::scene_producer>(L"psd", doc.width(), doc.height(), dependencies.format_desc);

	std::vector<std::pair<std::wstring, spl::shared_ptr<core::text_producer>>> text_producers_by_layer_name;

	std::stack<dependency_resolver> scene_stack;
	scene_stack.push(dependency_resolver{ root, true });

	auto layers_end = doc.layers().rend();
	for(auto it = doc.layers().rbegin(); it != layers_end; ++it)
	{
		auto& psd_layer = (*it);
		auto& current = scene_stack.top();

		if (psd_layer->group_mode() == layer_type::group) {
			//we've found a group. Create a new scene and add it to the scene-stack

			auto format_desc = (dependencies.format_desc.field_count == 1) ? dependencies.format_desc : core::video_format_desc{ dependencies.format_desc.format,
																																	dependencies.format_desc.width,
																																	dependencies.format_desc.height,
																																	dependencies.format_desc.square_width,
																																	dependencies.format_desc.square_height,
																																	core::field_mode::progressive,
																																	dependencies.format_desc.time_scale*2,
																																	dependencies.format_desc.duration,
																																	dependencies.format_desc.name,
																																	dependencies.format_desc.audio_cadence };
			
			auto group = spl::make_shared<core::scene::scene_producer>(psd_layer->name(), doc.width(), doc.height(), format_desc);

			auto& scene_layer = current.scene()->create_layer(group, psd_layer->location().x, psd_layer->location().y, psd_layer->name());
			scene_layer.adjustments.opacity.set(psd_layer->opacity() / 255.0);
			scene_layer.hidden.set(!psd_layer->is_visible());

			if (psd_layer->has_timeline())
				create_timelines(root, dependencies.format_desc, scene_layer, psd_layer, 0, 0);

			if (psd_layer->is_movable())
				current.add(&scene_layer, psd_layer->tags(), 0, 0, false);

			if (psd_layer->is_resizable())	//TODO: we could add support for resizable groups with vector masks
				CASPAR_LOG(warning) << "Groups doesn't support the \"resizable\"-tag.";

			if (psd_layer->is_explicit_dynamic())
				CASPAR_LOG(warning) << "Groups doesn't support the \"dynamic\"-tag.";

			scene_stack.push(group);
			continue;
		}
		else if (psd_layer->group_mode() == layer_type::group_delimiter) {
			//a group-terminator. Finish the scene
			current.scene()->reverse_layers();
			current.calculate();
			scene_stack.pop();
			continue;
		}

		std::shared_ptr<core::frame_producer> layer_producer;
		int adjustment_x = 0,
			adjustment_y = 0;

		if(psd_layer->is_visible())
		{
			auto layer_name = psd_layer->name();

			if(psd_layer->is_text() && !psd_layer->is_static())
			{
				std::wstring str = psd_layer->text_data().get(L"EngineDict.Editor.Text", L"");
			
				core::text::text_info text_info(std::move(get_text_info(psd_layer->text_data())));
				text_info.size *= psd_layer->text_scale();

				auto text_producer = core::text_producer::create(dependencies.frame_factory, 0, 0, str, text_info, doc.width(), doc.height());
				text_producer->pixel_constraints().width.set(psd_layer->size().width);
				text_producer->pixel_constraints().height.set(psd_layer->size().height);
				core::text::string_metrics metrics = text_producer->measure_string(str);
			
				adjustment_x = -2;	//the 2 offset is just a hack for now. don't know why our text is rendered 2 px to the right of that in photoshop
				adjustment_y = metrics.bearingY;
				layer_producer = text_producer;

				text_producers_by_layer_name.push_back(std::make_pair(layer_name, text_producer));
			}
			else
			{
				if(psd_layer->is_solid())
				{
					layer_producer = core::create_const_producer(core::create_color_frame(it->get(), dependencies.frame_factory, psd_layer->solid_color().to_uint32()), psd_layer->bitmap()->width(), psd_layer->bitmap()->height());
				}
				else if(psd_layer->bitmap())
				{
					if (psd_layer->is_placeholder())
					{
						auto hotswap = std::make_shared<core::hotswap_producer>(psd_layer->bitmap()->width(), psd_layer->bitmap()->height());
						hotswap->producer().set(dependencies.producer_registry->create_producer(dependencies, layer_name));
						layer_producer = hotswap;
					}
					else
					{
						core::pixel_format_desc pfd(core::pixel_format::bgra);
						pfd.planes.push_back(core::pixel_format_desc::plane(psd_layer->bitmap()->width(), psd_layer->bitmap()->height(), 4));

						auto frame = dependencies.frame_factory->create_frame(it->get(), pfd, core::audio_channel_layout::invalid());
						auto destination = frame.image_data().data();
						auto source = psd_layer->bitmap()->data();
						memcpy(destination, source, frame.image_data().size());

						layer_producer = core::create_const_producer(core::draw_frame(std::move(frame)), psd_layer->bitmap()->width(), psd_layer->bitmap()->height());
					}
				}
			}

			if (layer_producer)
			{
				auto& scene_layer = current.scene()->create_layer(spl::make_shared_ptr(layer_producer), psd_layer->location().x + adjustment_x, psd_layer->location().y + adjustment_y, layer_name);
				scene_layer.adjustments.opacity.set(psd_layer->opacity() / 255.0);
				scene_layer.hidden.set(!psd_layer->is_visible());	//this will always evaluate to true

				if (psd_layer->mask().has_vector()) {
					
					//this rectangle is in document-coordinates
					auto mask = psd_layer->mask().vector()->rect();	
					
					//remap to layer-coordinates
					auto left = static_cast<double>(mask.location.x) - scene_layer.position.x.get();
					auto right = left + static_cast<double>(mask.size.width);
					auto top = static_cast<double>(mask.location.y) - scene_layer.position.y.get();
					auto bottom = top + static_cast<double>(mask.size.height);

					scene_layer.crop.upper_left.x.unbind();
					scene_layer.crop.upper_left.x.set(left);
					scene_layer.crop.upper_left.y.unbind();
					scene_layer.crop.upper_left.y.set(top);

					scene_layer.crop.lower_right.x.unbind();
					scene_layer.crop.lower_right.x.set(right);

					scene_layer.crop.lower_right.y.unbind();
					scene_layer.crop.lower_right.y.set(bottom);
				}

				if (psd_layer->has_timeline())
					create_timelines(root, dependencies.format_desc, scene_layer, psd_layer, adjustment_x, adjustment_y);

				if (psd_layer->is_movable() || psd_layer->is_resizable() || (psd_layer->is_text() && !psd_layer->is_static()))
					current.add(&scene_layer, psd_layer->tags(), -adjustment_x, -adjustment_y, psd_layer->mask().has_vector());
			}
		}
	}
	if (scene_stack.size() != 1) {
		
	}
	root->reverse_layers();
	scene_stack.top().calculate();

	if (doc.has_timeline())
		create_marks(root, dependencies.format_desc, doc.timeline());

	// Reset all dynamic text fields to empty strings and expose them as a scene parameter.
	for (auto& text_layer : text_producers_by_layer_name) {
		text_layer.second->text().set(L"");
		text_layer.second->text().bind(root->create_variable<std::wstring>(boost::to_lower_copy(text_layer.first), true, L""));
	}

	auto params2 = params;
	params2.erase(params2.begin());

	root->call(params2);

	return root;
}

void init(core::module_dependencies dependencies)
{
	dependencies.cg_registry->register_cg_producer(
			L"psd",
			{ L".psd" },
			[](const std::wstring& filename) { return ""; },
			[](const spl::shared_ptr<core::frame_producer>& producer)
			{
				return spl::make_shared<core::scene::scene_cg_proxy>(producer);
			},
			[](
					const core::frame_producer_dependencies& dependencies,
					const std::wstring& filename)
			{
				return create_psd_scene_producer(dependencies, { filename });
			},
			false);
}

}}
