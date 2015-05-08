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

#include "xml_scene_producer.h"
#include "expression_parser.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <common/env.h>
#include <core/producer/frame_producer.h>

#include <future>

#include "scene_producer.h"

namespace caspar { namespace core { namespace scene {

void deduce_expression(variable& var, const variable_repository& repo)
{
	auto expr_str = var.original_expr();
	auto trimmed = boost::trim_copy(expr_str);

	if (boost::starts_with(trimmed, L"${") && boost::ends_with(trimmed, L"}"))
	{
		expr_str = trimmed.substr(2, expr_str.length() - 3);

		if (var.is<double>())
			var.as<double>().bind(parse_expression<double>(expr_str, repo));
		else if (var.is<bool>())
			var.as<bool>().bind(parse_expression<bool>(expr_str, repo));
		else if (var.is<std::wstring>())
			var.as<std::wstring>().bind(parse_expression<std::wstring>(expr_str, repo));
	}
	else if (!expr_str.empty())
	{
		var.from_string(expr_str);
	}
}

spl::shared_ptr<core::frame_producer> create_xml_scene_producer(
		const spl::shared_ptr<core::frame_factory>& frame_factory,
		const core::video_format_desc& format_desc,
		const std::vector<std::wstring>& params)
{
	if (params.empty())
		return core::frame_producer::empty();

	std::wstring filename = env::media_folder() + L"/" + params[0] + L".xml";
	
	if (!boost::filesystem::is_regular_file(boost::filesystem::path(filename)))
		return core::frame_producer::empty();

	boost::property_tree::wptree root;
	boost::filesystem::wifstream file(filename);
	boost::property_tree::read_xml(
			file,
			root,
			boost::property_tree::xml_parser::trim_whitespace
					| boost::property_tree::xml_parser::no_comments);

	int width = root.get<int>(L"scene.<xmlattr>.width");
	int height = root.get<int>(L"scene.<xmlattr>.height");

	auto scene = spl::make_shared<scene_producer>(width, height);

	for (auto elem : root.get_child(L"scene.variables"))
	{
		auto type = elem.second.get<std::wstring>(L"<xmlattr>.type");
		auto id = elem.second.get<std::wstring>(L"<xmlattr>.id");
		auto is_public = elem.second.get(L"<xmlattr>.public", false);
		auto expr = elem.second.get_value<std::wstring>();

		if (!is_public)
			id = L"variable." + id;

		if (type == L"number")
			scene->create_variable<double>(id, is_public, expr);
		else if (type == L"string")
			scene->create_variable<std::wstring>(id, is_public, expr);
		else if (type == L"bool")
			scene->create_variable<bool>(id, is_public, expr);
	}

	for (auto& elem : root.get_child(L"scene.layers"))
	{
		auto id = elem.second.get<std::wstring>(L"<xmlattr>.id");
		auto producer = create_producer(frame_factory, format_desc, elem.second.get<std::wstring>(L"producer"));
		auto& layer = scene->create_layer(producer, 0, 0, id);
		auto variable_prefix = L"layer." + id + L".";

		layer.hidden = scene->create_variable<bool>(variable_prefix + L"hidden", false, elem.second.get(L"hidden", L"false"));
		layer.position.x = scene->create_variable<double>(variable_prefix + L"x", false, elem.second.get<std::wstring>(L"x"));
		layer.position.y = scene->create_variable<double>(variable_prefix + L"y", false, elem.second.get<std::wstring>(L"y"));

		layer.adjustments.opacity = scene->create_variable<double>(variable_prefix + L"adjustment.opacity", false, elem.second.get(L"adjustments.opacity", L"1.0"));

		scene->create_variable<double>(variable_prefix + L"width", false) = layer.producer.get()->pixel_constraints().width;
		scene->create_variable<double>(variable_prefix + L"height", false) = layer.producer.get()->pixel_constraints().height;

		for (auto& var_name : producer->get_variables())
		{
			auto& var = producer->get_variable(var_name);
			auto expr = elem.second.get<std::wstring>(L"parameters." + var_name, L"");

			if (var.is<double>())
				scene->create_variable<double>(variable_prefix + L"parameter." + var_name, false, expr) = var.as<double>();
			else if (var.is<std::wstring>())
				scene->create_variable<std::wstring>(variable_prefix + L"parameter." + var_name, false, expr) = var.as<std::wstring>();
			else if (var.is<bool>())
				scene->create_variable<bool>(variable_prefix + L"parameter." + var_name, false, expr) = var.as<bool>();
		}
	}

	for (auto& elem : root.get_child(L"scene.timelines"))
	{
		auto& variable = scene->get_variable(elem.second.get<std::wstring>(L"<xmlattr>.variable"));

		for (auto& k : elem.second)
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

	auto repo = [&scene](const std::wstring& name) -> variable&
	{
		return scene->get_variable(name); 
	};

	for (auto& var_name : scene->get_variables())
	{
		deduce_expression(scene->get_variable(var_name), repo);
	}

	auto params2 = params;
	params2.erase(params2.begin());

	scene->call(params2);

	return scene;
}

}}}
