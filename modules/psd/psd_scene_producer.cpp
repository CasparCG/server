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
#include "layer.h"
#include "doc.h"

#include <core/frame/pixel_format.h>
#include <core/frame/frame_factory.h>
#include <core/producer/frame_producer.h>
#include <core/producer/color/color_producer.h>
#include <core/producer/text/text_producer.h>
#include <core/producer/scene/scene_producer.h>
#include <core/producer/scene/const_producer.h>
#include <core/producer/scene/hotswap_producer.h>
#include <core/frame/draw_frame.h>

#include <common/env.h>
#include <common/memory.h>
#include <common/log.h>

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/thread/future.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/rational.hpp>

#include <future>

namespace caspar { namespace psd {

void init()
{
	core::register_producer_factory(create_psd_scene_producer);
}

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
	auto color_node = ptree.get_child(L"EngineDict.StyleRun.RunArray..StyleSheet.StyleSheetData.FillColor.Values");
	for(auto it = color_node.begin(); it != color_node.end(); ++it, ++child_index)
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

	class layer_link_constructor
	{
		struct linked_layer_record
		{
			caspar::core::scene::layer* layer;
			int link_id;
			bool is_master;
			double adjustment_x;
			double adjustment_y;
		};

		std::vector<linked_layer_record> layers;
		std::vector<linked_layer_record> masters;

	public:
		void add(caspar::core::scene::layer* layer, int link_group, bool master, double adjustment_x, double adjustment_y)
		{
			linked_layer_record rec;
			rec.layer = layer;
			rec.link_id = link_group;
			rec.is_master = master;
			rec.adjustment_x = adjustment_x;
			rec.adjustment_y = adjustment_y;
			layers.push_back(rec);

			if(rec.is_master)
			{
				auto it = std::find_if(masters.begin(), masters.end(), [=](linked_layer_record const &r){ return r.link_id == link_group; });
				if(it == masters.end())
					masters.push_back(rec);
				else
					masters.erase(it);	//ambigous, two linked layers with locked position
			}
		}

		void calculate()
		{
			for(auto it = masters.begin(); it != masters.end(); ++it)
			{
				auto& master = (*it);
				//std::for_each(layers.begin(), layers.end(), [&master](linked_layer_record &r) mutable { 
				BOOST_FOREACH(auto &r, layers) {
					if(r.link_id == master.link_id && r.layer != master.layer)
					{
						{	//x-coords
							double slave_left = r.layer->position.x.get() + r.adjustment_x;
							double slave_right = slave_left + r.layer->producer.get()->pixel_constraints().width.get();

							double master_left = master.layer->position.x.get() + master.adjustment_x;
							double master_right = master_left + master.layer->producer.get()->pixel_constraints().width.get();

							if((slave_left >= master_left && slave_right <= master_right) || (slave_left <= master_left && slave_right >= master_right))
							{
								//change width of slave
								r.layer->position.x.bind(master.layer->position.x - r.adjustment_x + master.adjustment_x + (slave_left - master_left));
								r.layer->producer.get()->pixel_constraints().width.bind(master.layer->producer.get()->pixel_constraints().width + (slave_right - slave_left - master_right + master_left)); 
							}
							else if(slave_left >= master_right)
							{
								//push slave ahead of master
								r.layer->position.x.bind(master.layer->position.x - r.adjustment_x + master.adjustment_x + master.layer->producer.get()->pixel_constraints().width + (slave_left - master_right));
							}
							else if(slave_right <= master_left)
							{
								r.layer->position.x.bind(master.layer->position.x - r.adjustment_x + master.adjustment_x - (master_left - slave_left));
							}
						}

						{	//y-coords
							double slave_top = r.layer->position.y.get() + r.adjustment_y;
							double slave_bottom = slave_top + r.layer->producer.get()->pixel_constraints().height.get();

							double master_top = master.layer->position.y.get() + master.adjustment_y;
							double master_bottom = master_top + master.layer->producer.get()->pixel_constraints().height.get();

							if((slave_top >= master_top && slave_bottom <= master_bottom) || (slave_top <= master_top && slave_bottom >= master_bottom))
							{
								//change height of slave
								r.layer->position.y.bind(master.layer->position.y - r.adjustment_y + master.adjustment_y + (slave_top - master_top));
								r.layer->producer.get()->pixel_constraints().height.bind(master.layer->producer.get()->pixel_constraints().height + (slave_bottom - slave_top - master_bottom + master_top)); 
							}
							else if(slave_top >= master_bottom)
							{
								//push slave ahead of master
								r.layer->position.y.bind(master.layer->position.y - r.adjustment_y + master.adjustment_y + master.layer->producer.get()->pixel_constraints().height + (slave_top - master_bottom));
							}
							else if(slave_bottom <= master_top)
							{
								r.layer->position.y.bind(master.layer->position.y - r.adjustment_y + master.adjustment_y - (master_top - slave_top));
							}
						}

					}
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

	BOOST_FOREACH(auto& track, *tracklist)
	{
		auto track_id = track.second.get<std::wstring>(L"stdTrackID");

		if (track_id == L"sheetPositionTrack")
		{
			BOOST_FOREACH(auto& key, track.second.get_child(L"keyList"))
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

			BOOST_FOREACH(auto& key, track.second.get_child(L"keyList"))
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
	}
}

spl::shared_ptr<core::frame_producer> create_psd_scene_producer(const spl::shared_ptr<core::frame_factory>& frame_factory, const core::video_format_desc& format_desc, const std::vector<std::wstring>& params)
{
	std::wstring filename = env::media_folder() + L"\\" + params[0] + L".psd";
	if(!boost::filesystem::is_regular_file(boost::filesystem::path(filename)))
		return core::frame_producer::empty();

	psd_document doc;
	if(!doc.parse(filename))
		return core::frame_producer::empty();

	spl::shared_ptr<core::scene::scene_producer> root(spl::make_shared<core::scene::scene_producer>(doc.width(), doc.height()));

	layer_link_constructor link_constructor;

	std::vector<std::pair<std::wstring, spl::shared_ptr<core::text_producer>>> text_producers_by_layer_name;

	auto layers_end = doc.layers().end();
	for(auto it = doc.layers().begin(); it != layers_end; ++it)
	{
		if((*it)->is_visible())
		{
			if((*it)->is_text() && (*it)->sheet_color() == 4)
			{
				std::wstring str = (*it)->text_data().get(L"EngineDict.Editor.Text", L"");
			
				core::text::text_info text_info(std::move(get_text_info((*it)->text_data())));
				text_info.size *= (*it)->text_scale();

				auto text_producer = core::text_producer::create(frame_factory, 0, 0, str, text_info, doc.width(), doc.height());
			
				core::text::string_metrics metrics = text_producer->measure_string(str);
			
				auto adjustment_x = -2;	//the 2 offset is just a hack for now. don't know why our text is rendered 2 px to the right of that in photoshop
				auto adjustment_y = metrics.bearingY;
				auto& new_layer = root->create_layer(text_producer, (*it)->location().x + adjustment_x, (*it)->location().y + adjustment_y, (*it)->name());	
				new_layer.adjustments.opacity.set((*it)->opacity() / 255.0);
				new_layer.hidden.set(!(*it)->is_visible());

				if ((*it)->has_timeline())
					create_timelines(root, format_desc, new_layer, (*it), adjustment_x, adjustment_y);

				if((*it)->link_group_id() != 0)
					link_constructor.add(&new_layer, (*it)->link_group_id(), (*it)->is_position_protected(), -adjustment_x, -adjustment_y);

				text_producers_by_layer_name.push_back(std::make_pair((*it)->name(), text_producer));
			}
			else
			{
				std::wstring layer_name = (*it)->name();
				std::shared_ptr<core::frame_producer> layer_producer;
				if((*it)->is_solid())
				{
					layer_producer = core::create_const_producer(core::create_color_frame(it->get(), frame_factory, (*it)->solid_color().to_uint32()), (*it)->bitmap()->width(), (*it)->bitmap()->height());
				}
				else if((*it)->bitmap())
				{
					/*if (boost::algorithm::istarts_with(layer_name, L"[producer]"))
					{
						auto hotswap = std::make_shared<core::hotswap_producer>((*it)->rect().width(), (*it)->rect().height());
						hotswap->producer().set(core::create_producer(frame_factory, format_desc, layer_name.substr(10)));
						layer_producer = hotswap;
					}
					else*/
					{
						core::pixel_format_desc pfd(core::pixel_format::bgra);
						pfd.planes.push_back(core::pixel_format_desc::plane((*it)->bitmap()->width(), (*it)->bitmap()->height(), 4));

						auto frame = frame_factory->create_frame(it->get(), pfd);
						memcpy(frame.image_data().data(), (*it)->bitmap()->data(), frame.image_data().size());

						layer_producer = core::create_const_producer(core::draw_frame(std::move(frame)), (*it)->bitmap()->width(), (*it)->bitmap()->height());
					}
				}

				if(layer_producer)
				{
					auto& new_layer = root->create_layer(spl::make_shared_ptr(layer_producer), (*it)->location().x, (*it)->location().y, (*it)->name());
					new_layer.adjustments.opacity.set((*it)->opacity() / 255.0);
					new_layer.hidden.set(!(*it)->is_visible());

					if ((*it)->has_timeline())
						create_timelines(root, format_desc, new_layer, (*it), 0, 0);

					if((*it)->link_group_id() != 0)
						link_constructor.add(&new_layer, (*it)->link_group_id(), (*it)->is_position_protected(), 0, 0);
				}
			}
		}
	}

	link_constructor.calculate();

	// Reset all dynamic text fields to empty strings and expose them as a scene parameter.
	BOOST_FOREACH(auto& text_layer, text_producers_by_layer_name)
		text_layer.second->text().bind(root->create_variable<std::wstring>(boost::to_lower_copy(text_layer.first), true, L""));

	auto params2 = params;
	params2.erase(params2.cbegin());

	root->call(params2);

	return root;
}

}}