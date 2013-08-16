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

#include "xml_scene_producer.h"

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <common/env.h>
#include <core/producer/frame_producer.h>

#include "scene_producer.h"

namespace caspar { namespace core { namespace scene {

spl::shared_ptr<core::frame_producer> create_xml_scene_producer(
		const spl::shared_ptr<core::frame_factory>& frame_factory,
		const core::video_format_desc& format_desc,
		const std::vector<std::wstring>& params)
{
	if (params.empty())
		return core::frame_producer::empty();

	std::wstring filename = env::media_folder() + L"\\" + params[0] + L".xml";
	
	if (!boost::filesystem::is_regular_file(boost::filesystem::path(filename)))
		return core::frame_producer::empty();

	boost::property_tree::wptree root;
	std::wifstream file(filename);
	boost::property_tree::read_xml(
			file,
			root,
			boost::property_tree::xml_parser::trim_whitespace
					| boost::property_tree::xml_parser::no_comments);

	int width = root.get<int>(L"scene.<xmlattr>.width");
	int height = root.get<int>(L"scene.<xmlattr>.height");

	auto scene = spl::make_shared<scene_producer>(width, height);

	BOOST_FOREACH(auto elem, root.get_child(L"scene.variables"))
	{
		auto type = elem.second.get<std::wstring>(L"<xmlattr>.type");

		if (type == L"double")
			scene->create_variable<double>(
					L"variable." + elem.second.get<std::wstring>(L"<xmlattr>.id"),
					false,
					elem.second.get_value<std::wstring>());
	}

	BOOST_FOREACH(auto elem, root.get_child(L"scene.layers"))
	{
		auto id = elem.second.get<std::wstring>(L"<xmlattr>.id");
		auto producer = create_producer(frame_factory, format_desc, elem.second.get<std::wstring>(L"producer"));
		auto& layer = scene->create_layer(producer, 0, 0, id);
		auto variable_prefix = L"layer." + id + L".";

		layer.hidden = scene->create_variable<bool>(variable_prefix + L"hidden", false, elem.second.get(L"hidden", L"false"));
		layer.position.x = scene->create_variable<double>(variable_prefix + L"x", false, elem.second.get<std::wstring>(L"x"));
		layer.position.y = scene->create_variable<double>(variable_prefix + L"y", false, elem.second.get<std::wstring>(L"y"));

		scene->create_variable<double>(variable_prefix + L"width", false) = layer.producer.get()->pixel_constraints().width;
		scene->create_variable<double>(variable_prefix + L"height", false) = layer.producer.get()->pixel_constraints().height;
	}

	BOOST_FOREACH(auto& elem, root.get_child(L"scene.timelines"))
	{
		auto& variable = scene->get_variable(elem.second.get<std::wstring>(L"<xmlattr>.variable"));

		BOOST_FOREACH(auto& k, elem.second)
		{
			if (k.first == L"<xmlattr>")
				continue;

			auto easing = k.second.get(L"<xmlattr>.easing", L"");
			auto at = k.second.get<int64_t>(L"<xmlattr>.at");

			if (variable.is<double>())
				scene->add_keyframe(variable.as<double>(), k.second.get_value<double>(), at, easing);
			else if (variable.is<int>())
				scene->add_keyframe(variable.as<int>(), k.second.get_value<int>(), at, easing);
		}
	}

	BOOST_FOREACH(auto& elem, root.get_child(L"scene.parameters"))
	{
		auto& variable = scene->get_variable(elem.second.get<std::wstring>(L"<xmlattr>.variable"));
		auto id = elem.second.get<std::wstring>(L"<xmlattr>.id");

		if (variable.is<double>())
			scene->create_variable<double>(id, true) = variable.as<double>();
		else if (variable.is<std::wstring>())
			scene->create_variable<std::wstring>(id, true) = variable.as<std::wstring>();
	}

	return scene;
}

}}}
