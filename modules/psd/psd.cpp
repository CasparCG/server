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

#include "psd.h"
#include "layer.h"
#include "doc.h"

#include <core/frame/pixel_format.h>
#include <core/frame/frame_factory.h>
#include <core/producer/frame_producer.h>
#include <core/producer/text/text_producer.h>
#include <core/producer/scene/scene_producer.h>
#include <core/producer/scene/const_producer.h>
#include <core/frame/draw_frame.h>

#include <common/env.h>
#include <common/memory.h>

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>

namespace caspar { namespace psd {

void init()
{
	core::register_producer_factory(create_producer);
}

core::text::text_info get_text_info(const boost::property_tree::wptree& ptree)
{
	core::text::text_info result;
	int font_index = ptree.get(L"EngineDict.StyleRun.RunArray..StyleSheet.StyleSheetData.Font", 0);
	result.size = ptree.get(L"EngineDict.StyleRun.RunArray..StyleSheet.StyleSheetData.FontSize", 30.0f);

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
		};		

		std::vector<linked_layer_record> layers;
		std::vector<linked_layer_record> masters;

	public:
		void add(caspar::core::scene::layer* layer, int link_group, bool master)
		{
			linked_layer_record rec;
			rec.layer = layer;
			rec.link_id = link_group;
			rec.is_master = master;
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
			auto masters_end = masters.end();
			for(auto it = masters.begin(); it != masters.end(); ++it)
			{
				auto& master = (*it);
				//std::for_each(layers.begin(), layers.end(), [&master](linked_layer_record &r) mutable { 
				BOOST_FOREACH(auto &r, layers) {
					if(r.link_id == master.link_id && r.layer != master.layer)
					{
						{	//x-coords
							double slave_left = r.layer->position.x.get();
							double slave_right = slave_left + r.layer->producer.get()->pixel_constraints().width.get();

							double master_left = master.layer->position.x.get();
							double master_right = master_left + master.layer->producer.get()->pixel_constraints().width.get();

							if((slave_left >= master_left && slave_right <= master_right) || (slave_left <= master_left && slave_right >= master_right))
							{
								//change width of slave
								r.layer->position.x = master.layer->position.x + (slave_left - master_left);
								r.layer->producer.get()->pixel_constraints().width = master.layer->producer.get()->pixel_constraints().width + (slave_right - slave_left - master_right + master_left); 
							}
							else if(slave_left >= master_right)
							{
								//push slave ahead of master
								r.layer->position.x = master.layer->position.x + master.layer->producer.get()->pixel_constraints().width + (slave_left - master_right);
							}
							else if(slave_right <= master_left)
							{
								r.layer->position.x = master.layer->position.x - (master_left - slave_left);
							}
						}

						{	//y-coords
							double slave_top = r.layer->position.y.get();
							double slave_bottom = slave_top + r.layer->producer.get()->pixel_constraints().height.get();

							double master_top = master.layer->position.y.get();
							double master_bottom = master_top + master.layer->producer.get()->pixel_constraints().height.get();

							if((slave_top >= master_top && slave_bottom <= master_bottom) || (slave_top <= master_top && slave_bottom >= master_bottom))
							{
								//change width of slave
								r.layer->position.y = master.layer->position.y + (slave_top - master_top);
								r.layer->producer.get()->pixel_constraints().height = master.layer->producer.get()->pixel_constraints().height + (slave_bottom - slave_top - master_bottom + master_top); 
							}
							else if(slave_top >= master_bottom)
							{
								//push slave ahead of master
								r.layer->position.y = master.layer->position.y + master.layer->producer.get()->pixel_constraints().height + (slave_top - master_bottom);
							}
							else if(slave_bottom <= master_top)
							{
								r.layer->position.y = master.layer->position.y - (master_top - slave_top);
							}
						}

					}
				}
			}
		}
	};

spl::shared_ptr<core::frame_producer> create_producer(const spl::shared_ptr<core::frame_factory>& frame_factory, const core::video_format_desc& format_desc, const std::vector<std::wstring>& params)
{
	std::wstring filename = env::media_folder() + L"\\" + params[0] + L".psd";
	if(!boost::filesystem::is_regular_file(boost::filesystem::path(filename)))
		return core::frame_producer::empty();

	psd_document doc;
	if(!doc.parse(filename))
		return core::frame_producer::empty();

	spl::shared_ptr<core::scene::scene_producer> root(spl::make_shared<core::scene::scene_producer>(doc.width(), doc.height()));

	layer_link_constructor link_constructor;

	auto layers_end = doc.layers().end();
	for(auto it = doc.layers().begin(); it != layers_end; ++it)
	{
		if((*it)->is_text() && (*it)->visible())
		{
			std::wstring str = (*it)->text_data().get(L"EngineDict.Editor.Text", L"");
			
			core::text::text_info text_info(std::move(get_text_info((*it)->text_data())));
			auto text_producer = core::text_producer::create(frame_factory, 0, 0, str, text_info, doc.width(), doc.height());
			
			core::text::string_metrics metrics = text_producer->measure_string(str);
			
			auto& new_layer = root->create_layer(text_producer, (*it)->rect().left - 2, (*it)->rect().top + metrics.bearingY, (*it)->name());	//the 2 offset is just a hack for now. don't know why our text is rendered 2 px to the right of that in photoshop
			new_layer.adjustments.opacity.set((*it)->opacity() / 255.0);
			new_layer.hidden.set(!(*it)->visible());

			if((*it)->link_group_id() != 0)
				link_constructor.add(&new_layer, (*it)->link_group_id(), true);
		}
		else if((*it)->image() && (*it)->visible())
		{
			core::pixel_format_desc pfd(core::pixel_format::bgra);
			pfd.planes.push_back(core::pixel_format_desc::plane((*it)->rect().width(), (*it)->rect().height(), 4));

			auto frame = frame_factory->create_frame(it->get(), pfd);
			memcpy(frame.image_data().data(), (*it)->image()->data(), frame.image_data().size());

			auto layer_producer = core::create_const_producer(core::draw_frame(std::move(frame)), (*it)->rect().width(), (*it)->rect().height());

			auto& new_layer = root->create_layer(layer_producer, (*it)->rect().left, (*it)->rect().top);
			new_layer.adjustments.opacity.set((*it)->opacity() / 255.0);
			new_layer.hidden.set(!(*it)->visible());

			if((*it)->link_group_id() != 0)
				link_constructor.add(&new_layer, (*it)->link_group_id(), false);
		}
	}

	link_constructor.calculate();

	return root;
}

}}