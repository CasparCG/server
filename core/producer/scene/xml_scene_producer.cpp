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
#include <common/os/filesystem.h>
#include <common/filesystem.h>
#include <common/ptree.h>
#include <core/producer/frame_producer.h>
#include <core/help/help_repository.h>
#include <core/help/help_sink.h>

#include <future>
#include <vector>

#include "scene_producer.h"
#include "scene_cg_proxy.h"

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

void describe_xml_scene_producer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"A simple producer for dynamic graphics using .scene files.");
	sink.syntax(L"[.scene_filename:string] {[param1:string] [value1:string]} {[param2:string] [value2:string]} ...");
	sink.para()->text(L"A simple producer that looks in the ")->code(L"templates")->text(L" folder for .scene files.");
	sink.para()->text(L"The .scene file is a simple XML document containing variables, layers and timelines.");
	sink.example(L">> PLAY 1-10 scene_name_sign FIRSTNAME \"John\" LASTNAME \"Doe\"", L"loads and plays templates/scene_name_sign.scene and sets the variables FIRSTNAME and LASTNAME.");
	sink.para()->text(L"The scene producer also supports setting the variables while playing via the CALL command:");
	sink.example(L">> CALL 1-10 FIRSTNAME \"Jane\"", L"changes the variable FIRSTNAME on an already loaded scene.");
}

spl::shared_ptr<core::frame_producer> create_xml_scene_producer(
		const core::frame_producer_dependencies& dependencies,
		const std::vector<std::wstring>& params)
{
	if (params.empty())
		return core::frame_producer::empty();

	auto found = find_case_insensitive(env::template_folder() + L"/" + params.at(0) + L".scene");
	if (!found)
		return core::frame_producer::empty();

	auto filename		= *found;
	auto template_name	= get_relative(filename, env::template_folder()).wstring();

	CASPAR_SCOPED_CONTEXT_MSG(template_name + L": ");

	boost::property_tree::wptree root;
	boost::filesystem::wifstream file(filename);
	boost::property_tree::read_xml(
			file,
			root,
			boost::property_tree::xml_parser::trim_whitespace
					| boost::property_tree::xml_parser::no_comments);

	int width = ptree_get<int>(root, L"scene.<xmlattr>.width");
	int height = ptree_get<int>(root, L"scene.<xmlattr>.height");

	auto scene = spl::make_shared<scene_producer>(L"scene", template_name, width, height, dependencies.format_desc);

	for (auto elem : root | witerate_children(L"scene.variables") | welement_context_iteration)
	{
		ptree_verify_element_name(elem, L"variable");

		auto type		= ptree_get<std::wstring>(elem.second, L"<xmlattr>.type");
		auto id			= ptree_get<std::wstring>(elem.second, L"<xmlattr>.id");
		auto is_public	= elem.second.get(L"<xmlattr>.public", false);
		auto expr		= ptree_get_value<std::wstring>(elem.second);

		if (!is_public)
			id = L"variable." + id;

		if (type == L"number")
			scene->create_variable<double>(id, is_public, expr);
		else if (type == L"string")
			scene->create_variable<std::wstring>(id, is_public, expr);
		else if (type == L"bool")
			scene->create_variable<bool>(id, is_public, expr);
	}

	for (auto& elem : root | witerate_children(L"scene.layers") | welement_context_iteration)
	{
		ptree_verify_element_name(elem, L"layer");

		auto id							= ptree_get<std::wstring>(elem.second, L"<xmlattr>.id");
		auto producer_string			= ptree_get<std::wstring>(elem.second, L"producer");
		auto producer					= [&]
		{
			CASPAR_SCOPED_CONTEXT_MSG(" -> ");
			auto adjusted_dependencies = dependencies;
			auto& adjusted_format_desc = adjusted_dependencies.format_desc;

			adjusted_format_desc.field_mode		 = field_mode::progressive;
			adjusted_format_desc.fps			*= adjusted_format_desc.field_count;
			adjusted_format_desc.duration		/= adjusted_format_desc.field_count;
			adjusted_format_desc.field_count	 = 1;

			return dependencies.producer_registry->create_producer(adjusted_dependencies, producer_string);
		}();
		auto& layer						= scene->create_layer(producer, 0, 0, id);
		auto variable_prefix			= L"layer." + id + L".";

		auto overridden_width = elem.second.get<std::wstring>(L"width", L"");
		if (!overridden_width.empty())
			layer.producer.get()->pixel_constraints().width = scene->create_variable<double>(variable_prefix + L"width", false, overridden_width);
		else
			scene->create_variable<double>(variable_prefix + L"width", false) = layer.producer.get()->pixel_constraints().width;

		auto overridden_height = elem.second.get<std::wstring>(L"height", L"");
		if (!overridden_height.empty())
			layer.producer.get()->pixel_constraints().height = scene->create_variable<double>(variable_prefix + L"height", false, overridden_height);
		else
			scene->create_variable<double>(variable_prefix + L"height", false) = layer.producer.get()->pixel_constraints().height;

		layer.hidden								= scene->create_variable<bool>(variable_prefix + L"hidden", false, elem.second.get(L"hidden", L"false"));
		layer.position.x							= scene->create_variable<double>(variable_prefix + L"x", false, ptree_get<std::wstring>(elem.second, L"x"));
		layer.position.y							= scene->create_variable<double>(variable_prefix + L"y", false, ptree_get<std::wstring>(elem.second, L"y"));
		layer.anchor.x								= scene->create_variable<double>(variable_prefix + L"anchor_x", false, elem.second.get<std::wstring>(L"anchor_x", L"0.0"));
		layer.anchor.y								= scene->create_variable<double>(variable_prefix + L"anchor_y", false, elem.second.get<std::wstring>(L"anchor_y", L"0.0"));
		layer.clip.upper_left.x						= scene->create_variable<double>(variable_prefix + L"clip.upper_left_x", false, elem.second.get<std::wstring>(L"clip.upper_left_x", L"0.0"));
		layer.clip.upper_left.y						= scene->create_variable<double>(variable_prefix + L"clip.upper_left_y", false, elem.second.get<std::wstring>(L"clip.upper_left_y", L"0.0"));
		layer.clip.lower_right.x					= scene->create_variable<double>(variable_prefix + L"clip.lower_right_x", false, elem.second.get<std::wstring>(L"clip.lower_right_x", L"${scene_width}"));
		layer.clip.lower_right.y					= scene->create_variable<double>(variable_prefix + L"clip.lower_right_y", false, elem.second.get<std::wstring>(L"clip.lower_right_y", L"${scene_height}"));
		layer.rotation								= scene->create_variable<double>(variable_prefix + L"rotation", false, elem.second.get<std::wstring>(L"rotation", L"0.0"));
		layer.crop.upper_left.x						= scene->create_variable<double>(variable_prefix + L"crop_upper_left_x", false, elem.second.get<std::wstring>(L"crop_upper_left_x", L"0.0"));
		layer.crop.upper_left.y						= scene->create_variable<double>(variable_prefix + L"crop_upper_left_y", false, elem.second.get<std::wstring>(L"crop_upper_left_y", L"0.0"));
		layer.crop.lower_right.x					= scene->create_variable<double>(variable_prefix + L"crop_lower_right_x", false, elem.second.get<std::wstring>(L"crop_lower_right_x", L"${" + variable_prefix + L"width}"));
		layer.crop.lower_right.y					= scene->create_variable<double>(variable_prefix + L"crop_lower_right_y", false, elem.second.get<std::wstring>(L"crop_lower_right_y", L"${" + variable_prefix + L"height}"));
		layer.perspective.upper_left.x				= scene->create_variable<double>(variable_prefix + L"perspective_upper_left_x", false, elem.second.get<std::wstring>(L"perspective_upper_left_x", L"0.0"));
		layer.perspective.upper_left.y				= scene->create_variable<double>(variable_prefix + L"perspective_upper_left_y", false, elem.second.get<std::wstring>(L"perspective_upper_left_y", L"0.0"));
		layer.perspective.upper_right.x				= scene->create_variable<double>(variable_prefix + L"perspective_upper_right_x", false, elem.second.get<std::wstring>(L"perspective_upper_right_x", L"${" + variable_prefix + L"width}"));
		layer.perspective.upper_right.y				= scene->create_variable<double>(variable_prefix + L"perspective_upper_right_y", false, elem.second.get<std::wstring>(L"perspective_upper_right_y", L"0.0"));
		layer.perspective.lower_right.x				= scene->create_variable<double>(variable_prefix + L"perspective_lower_right_x", false, elem.second.get<std::wstring>(L"perspective_lower_right_x", L"${" + variable_prefix + L"width}"));
		layer.perspective.lower_right.y				= scene->create_variable<double>(variable_prefix + L"perspective_lower_right_y", false, elem.second.get<std::wstring>(L"perspective_lower_right_y", L"${" + variable_prefix + L"height}"));
		layer.perspective.lower_left.x				= scene->create_variable<double>(variable_prefix + L"perspective_lower_left_x", false, elem.second.get<std::wstring>(L"perspective_lower_left_x", L"0.0"));
		layer.perspective.lower_left.y				= scene->create_variable<double>(variable_prefix + L"perspective_lower_left_y", false, elem.second.get<std::wstring>(L"perspective_lower_left_y", L"${" + variable_prefix + L"height}"));

		layer.adjustments.opacity					= scene->create_variable<double>(variable_prefix + L"adjustment.opacity", false, elem.second.get(L"adjustments.opacity", L"1.0"));
		layer.adjustments.contrast					= scene->create_variable<double>(variable_prefix + L"adjustment.contrast", false, elem.second.get(L"adjustments.contrast", L"1.0"));
		layer.adjustments.saturation				= scene->create_variable<double>(variable_prefix + L"adjustment.saturation", false, elem.second.get(L"adjustments.saturation", L"1.0"));
		layer.adjustments.brightness				= scene->create_variable<double>(variable_prefix + L"adjustment.brightness", false, elem.second.get(L"adjustments.brightness", L"1.0"));
		layer.levels.min_input						= scene->create_variable<double>(variable_prefix + L"level.min_input", false, elem.second.get(L"levels.min_input", L"0.0"));
		layer.levels.max_input						= scene->create_variable<double>(variable_prefix + L"level.max_input", false, elem.second.get(L"levels.max_input", L"1.0"));
		layer.levels.gamma							= scene->create_variable<double>(variable_prefix + L"level.gamma", false, elem.second.get(L"levels.gamma", L"1.0"));
		layer.levels.min_output						= scene->create_variable<double>(variable_prefix + L"level.min_output", false, elem.second.get(L"levels.min_output", L"0.0"));
		layer.levels.max_output						= scene->create_variable<double>(variable_prefix + L"level.max_output", false, elem.second.get(L"levels.max_output", L"1.0"));
		layer.volume								= scene->create_variable<double>(variable_prefix + L"volume", false, elem.second.get(L"volume", L"1.0"));
		layer.is_key								= scene->create_variable<bool>(variable_prefix + L"is_key", false, elem.second.get(L"is_key", L"false"));
		layer.use_mipmap							= scene->create_variable<bool>(variable_prefix + L"use_mipmap", false, elem.second.get(L"use_mipmap", L"false"));
		layer.blend_mode							= scene->create_variable<std::wstring>(variable_prefix + L"blend_mode", false, elem.second.get(L"blend_mode", L"normal")).transformed([](const std::wstring& b) { return get_blend_mode(b); });
		layer.chroma_key.enable						= scene->create_variable<bool>(variable_prefix + L"chroma_key.enable", false, elem.second.get(L"chroma_key.enable", L"false"));
		layer.chroma_key.target_hue					= scene->create_variable<double>(variable_prefix + L"chroma_key.target_hue", false, elem.second.get(L"chroma_key.target_hue", L"120.0"));
		layer.chroma_key.hue_width					= scene->create_variable<double>(variable_prefix + L"chroma_key.hue_width", false, elem.second.get(L"chroma_key.hue_width", L"0.1"));
		layer.chroma_key.min_saturation				= scene->create_variable<double>(variable_prefix + L"chroma_key.min_saturation", false, elem.second.get(L"chroma_key.min_saturation", L"0.0"));
		layer.chroma_key.min_brightness				= scene->create_variable<double>(variable_prefix + L"chroma_key.min_brightness", false, elem.second.get(L"chroma_key.min_brightness", L"0.0"));
		layer.chroma_key.softness					= scene->create_variable<double>(variable_prefix + L"chroma_key.softness", false, elem.second.get(L"chroma_key.softness", L"0.0"));
		layer.chroma_key.spill_suppress				= scene->create_variable<double>(variable_prefix + L"chroma_key.spill_suppress", false, elem.second.get(L"chroma_key.spill_suppress", L"0.0"));
		layer.chroma_key.spill_suppress_saturation	= scene->create_variable<double>(variable_prefix + L"chroma_key.spill_suppress_saturation", false, elem.second.get(L"chroma_key.spill_suppress_saturation", L"1.0"));

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

	if (root.get_child_optional(L"scene.marks"))
	{
		for (auto& mark : root | witerate_children(L"scene.marks") | welement_context_iteration)
		{
			ptree_verify_element_name(mark, L"mark");

			auto at		= ptree_get<int64_t>(mark.second, L"<xmlattr>.at");
			auto type	= get_mark_action(ptree_get<std::wstring>(mark.second, L"<xmlattr>.type"));
			auto label	= mark.second.get(L"<xmlattr>.label", L"");

			scene->add_mark(at, type, label);
		}
	}

	if (root.get_child_optional(L"scene.timelines"))
	{
		for (auto& elem : root | witerate_children(L"scene.timelines") | welement_context_iteration)
		{
			ptree_verify_element_name(elem, L"timeline");

			auto variable_name	= ptree_get<std::wstring>(elem.second, L"<xmlattr>.variable");
			auto& variable		= scene->get_variable(variable_name);

			for (auto& k : elem.second)
			{
				if (k.first == L"<xmlattr>")
					continue;

				ptree_verify_element_name(k, L"keyframe");

				auto easing	= k.second.get(L"<xmlattr>.easing", L"");
				auto at		= ptree_get<int64_t>(k.second, L"<xmlattr>.at");

				auto keyframe_variable_name = L"timeline." + variable_name + L"." + boost::lexical_cast<std::wstring>(at);

				if (variable.is<double>())
					scene->add_keyframe(variable.as<double>(), scene->create_variable<double>(keyframe_variable_name, false, ptree_get_value<std::wstring>(k.second)), at, easing);
				else if (variable.is<int>())
					scene->add_keyframe(variable.as<int>(), scene->create_variable<int>(keyframe_variable_name, false, ptree_get_value<std::wstring>(k.second)), at, easing);
			}
		}
	}

	auto repo = [&scene](const std::wstring& name) -> variable&
	{
		return scene->get_variable(name);
	};

	int task_id = 0;
	if (root.get_child_optional(L"scene.tasks"))
	{
		for (auto& task : root | witerate_children(L"scene.tasks") | welement_context_iteration)
		{
			auto at_frame = task.second.get_optional<int64_t>(L"<xmlattr>.at");
			auto when = task.second.get_optional<std::wstring>(L"<xmlattr>.when");

			binding<bool> condition;

			if (at_frame)
				condition = scene->timeline_frame() == *at_frame;
			else if (when)
				condition = scene->create_variable<bool>(L"tasks.task_" + boost::lexical_cast<std::wstring>(task_id), false, *when);
			else
				CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Task elements must have either an at attribute or a when attribute"));

			auto& element_name = task.first;

			std::vector<std::wstring> call_params;

			if (element_name == L"call")
			{
				for (auto& arg : task.second)
				{
					if (arg.first == L"<xmlattr>")
						continue;

					ptree_verify_element_name(arg, L"arg");
					call_params.push_back(ptree_get_value<std::wstring>(arg.second));
				}
			}
			else if (element_name == L"cg_play")
				call_params.push_back(L"play()");
			else if (element_name == L"cg_stop")
				call_params.push_back(L"stop()");
			else if (element_name == L"cg_next")
				call_params.push_back(L"next()");
			else if (element_name == L"cg_invoke")
			{
				call_params.push_back(L"invoke()");
				call_params.push_back(ptree_get<std::wstring>(task.second, L"<xmlattr>.label"));
			}
			else if (element_name == L"goto_mark")
			{
				auto mark_label	= ptree_get<std::wstring>(task.second, L"<xmlattr>.label");
				auto weak_scene	= static_cast<std::weak_ptr<scene_producer>>(scene);

				scene->add_task(std::move(condition), [=]
				{
					auto strong = weak_scene.lock();

					if (strong)
						strong->call({ L"play()", mark_label });
				});
			}
			else
				CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Only valid element names in <tasks /> are call, cg_play, cg_stop, cg_next, cg_invoke and goto_mark"));

			if (!call_params.empty())
			{
				auto to_layer = ptree_get<std::wstring>(task.second, L"<xmlattr>.to_layer");
				auto producer = scene->get_layer(to_layer).producer.get();

				auto weak_producer = static_cast<std::weak_ptr<frame_producer>>(producer);

				scene->add_task(std::move(condition), [=]
				{
					auto strong = weak_producer.lock();

					if (strong)
						strong->call(call_params);
				});
			}

			++task_id;
		}
	}

	for (auto& var_name : scene->get_variables())
	{
		CASPAR_SCOPED_CONTEXT_MSG(L"/scene/variables/variable[@id=" + var_name + L"]/text()");
		deduce_expression(scene->get_variable(var_name), repo);
	}

	auto params2 = params;
	params2.erase(params2.begin());

	scene->call(params2);

	return scene;
}

void init(module_dependencies dependencies)
{
	dependencies.producer_registry->register_producer_factory(L"XML Scene Producer", create_xml_scene_producer, describe_xml_scene_producer);
	dependencies.cg_registry->register_cg_producer(
			L"scene",
			{ L".scene" },
			[](const std::wstring& filename) { return ""; },
			[](const spl::shared_ptr<core::frame_producer>& producer)
			{
				return spl::make_shared<core::scene::scene_cg_proxy>(producer);
			},
			[](
			const core::frame_producer_dependencies& dependencies,
			const std::wstring& filename)
			{
				return create_xml_scene_producer(dependencies, { filename });
			},
			false);
}

}}}
