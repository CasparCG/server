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
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "../../StdAfx.h"

#include "color_producer.h"

#include <core/producer/frame_producer.h>
#include <core/frame/frame.h>
#include <core/frame/draw_frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/pixel_format.h>
#include <core/frame/audio_channel_layout.h>
#include <core/monitor/monitor.h>
#include <core/help/help_repository.h>
#include <core/help/help_sink.h>

#include <common/array.h>

#include <boost/algorithm/string.hpp>

#include <sstream>
#include "core/producer/variable.h"
#include "common/future.h"

namespace caspar { namespace core {

draw_frame create_color_frame(void* tag, const spl::shared_ptr<frame_factory>& frame_factory, const std::vector<uint32_t>& values)
{
	if (values.size() == 0)
		return draw_frame::empty();

	core::pixel_format_desc desc(pixel_format::bgra);
	desc.planes.push_back(core::pixel_format_desc::plane(static_cast<int>(values.size()), 1, 4));
	auto frame = frame_factory->create_frame(tag, desc, core::audio_channel_layout::invalid());

	for (int i = 0; i < values.size(); ++i)
		*reinterpret_cast<uint32_t*>(frame.image_data(0).begin() + (i * 4)) = values.at(i);

	return core::draw_frame(std::move(frame));
}

draw_frame create_color_frame(void* tag, const spl::shared_ptr<frame_factory>& frame_factory, const uint32_t value)
{
	const std::vector<uint32_t> values = { value };

	return create_color_frame(tag, frame_factory, values);
}

std::vector<uint32_t> parse_colors(const std::wstring& raw_str)
{
	std::vector<std::wstring> split;
	boost::split(split, raw_str, boost::is_any_of(L" "));

	std::vector<uint32_t> values;

	for (int i = 0; i < split.size(); ++i)
	{
		uint32_t val = 0;
		if (try_get_color(split.at(i), val))
			values.push_back(val);
	}

	return std::move(values);
}

class color_producer : public frame_producer_base
{
	monitor::subject			monitor_subject_;

	constraints					constraints_;
	draw_frame					frame_;
	variable_impl<std::wstring>	colors_str_;
	std::shared_ptr<void>		colors_str_subscription_;

public:
	color_producer(const spl::shared_ptr<core::frame_factory>& frame_factory, const std::wstring& colors, const long parent_width, const long parent_height)
		: constraints_(parent_width, parent_height)
		, frame_(draw_frame::empty())
	{
		colors_str_subscription_ = colors_str_.value().on_change([this, frame_factory]()
		{
			auto colors = parse_colors(colors_str_.value().get());
			frame_ = create_color_frame(this, frame_factory, colors);

		});
		colors_str_.value().set(colors);

		CASPAR_LOG(info) << print() << L" Initialized";
	}

	std::future<std::wstring> call(const std::vector<std::wstring>& param) override
	{
		std::wstring result;
		colors_str_.value().set(param.empty() ? L"" : param[0]);

		return caspar::make_ready_future(std::move(result));
	}

	core::variable& get_variable(const std::wstring& name) override
	{
		if (name == L"color")
			return colors_str_;

		CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"color_producer does not have a variable called " + name));
	}

	const std::vector<std::wstring>& get_variables() const override
	{
		static const std::vector<std::wstring> vars = {
			L"color",
		};

		return vars;
	}

	// frame_producer

	draw_frame receive_impl() override
	{
		monitor_subject_ << monitor::message("color") % colors_str_.value().get();

		return frame_;
	}

	constraints& pixel_constraints() override
	{
		return constraints_;
	}

	std::wstring print() const override
	{
		return L"color[" + colors_str_.value().get() + L"]";
	}

	std::wstring name() const override
	{
		return L"color";
	}

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"color");
		info.add(L"color", colors_str_.value().get());
		return info;
	}

	monitor::subject& monitor_output() override {return monitor_subject_;}
};

std::wstring get_hex_color(const std::wstring& str)
{
	if(str.at(0) == '#')
		return str.length() == 7 ? L"#FF" + str.substr(1) : str;

	const std::wstring col_str = boost::to_upper_copy(str);

	if(col_str == L"EMPTY")
		return L"#00000000";

	if(col_str == L"BLACK")
		return L"#FF000000";

	if(col_str == L"WHITE")
		return L"#FFFFFFFF";

	if(col_str == L"RED")
		return L"#FFFF0000";

	if(col_str == L"GREEN")
		return L"#FF00FF00";

	if(col_str == L"BLUE")
		return L"#FF0000FF";

	if(col_str == L"ORANGE")
		return L"#FFFFA500";

	if(col_str == L"YELLOW")
		return L"#FFFFFF00";

	if(col_str == L"BROWN")
		return L"#FFA52A2A";

	if(col_str == L"GRAY")
		return L"#FF808080";

	if(col_str == L"TEAL")
		return L"#FF008080";

	return str;
}

bool try_get_color(const std::wstring& str, uint32_t& value)
{
	auto color_str = get_hex_color(str);
	if(color_str.length() != 9 || color_str[0] != '#')
		return false;

	std::wstringstream ss(color_str.substr(1));
	if(!(ss >> std::hex >> value) || !ss.eof())
		return false;

	return true;
}

void describe_color_producer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Fills a layer with a solid color or a horizontal gradient.");
	sink.syntax(
		L"[COLOR] "
		L"{#[argb_hex_value:string]}"
		L",{#[rgb_hex_value:string]}"
		L",{[named_color:EMPTY,BLACK,WHITE,RED,GREEN,BLUE,ORANGE,YELLOW,BROWN,GRAY,TEAL]} "
		L"{...more colors}");
	sink.para()->text(L"A producer that fills a layer with a solid color. If more than one color is specified it becomes a gradient between those colors.");
	sink.para()
		->text(L"If a ")->code(L"#")
		->text(L" sign followed by an 8 character hexadecimal number is given it is interpreted as an 8-bit per channel ARGB color. ")
		->text(L"If it is instead followed by a 6 character hexadecimal number it is interpreted as an 8-bit per channel RGB value with an opaque alpha channel.");
	sink.para()->text(L"There are also a number of predefined named colors, that are specified without a ")->code(L"#")->text(L" sign:");
	sink.definitions()
		->item(L"EMPTY",	L"Predefined ARGB value #00000000")
		->item(L"BLACK",	L"Predefined ARGB value #FF000000")
		->item(L"WHITE",	L"Predefined ARGB value #FFFFFFFF")
		->item(L"RED",		L"Predefined ARGB value #FFFF0000")
		->item(L"GREEN",	L"Predefined ARGB value #FF00FF00")
		->item(L"BLUE",		L"Predefined ARGB value #FF0000FF")
		->item(L"ORANGE",	L"Predefined ARGB value #FFFFA500")
		->item(L"YELLOW",	L"Predefined ARGB value #FFFFFF00")
		->item(L"BROWN",	L"Predefined ARGB value #FFA52A2A")
		->item(L"GRAY",		L"Predefined ARGB value #FF808080")
		->item(L"TEAL",		L"Predefined ARGB value #FF008080");
	sink.para()
		->strong(L"Note")->text(L" that it is important that all RGB values are multiplied with the alpha ")
		->text(L"at all times, otherwise the compositing in the mixer will be incorrect.");
	sink.para()->text(L"Solid color examples:");
	sink.example(L">> PLAY 1-10 [COLOR] EMPTY", L"For a completely transparent frame.");
	sink.example(L">> PLAY 1-10 [COLOR] #00000000", L"For an explicitly defined completely transparent frame.");
	sink.example(L">> PLAY 1-10 [COLOR] #000000", L"For an explicitly defined completely opaque black frame.");
	sink.example(L">> PLAY 1-10 [COLOR] RED", L"For a completely red frame.");
	sink.example(L">> PLAY 1-10 [COLOR] #7F007F00",
		L"For a completely green half transparent frame. "
		L"Since the RGB part has been premultiplied with the A part this is 100% green.");
	sink.para()->text(L"Horizontal gradient examples:");
	sink.example(L">> PLAY 1-10 [COLOR] WHITE BLACK", L"For a gradient between white and black.");
	sink.example(L">> PLAY 1-10 [COLOR] WHITE WHITE WHITE BLACK", L"For a gradient between white and black with the white part beings 3 times as long as the black part.");
	sink.example(
		L">> PLAY 1-10 [COLOR] RED EMPTY\n"
		L">> MIXER 1-10 ANCHOR 0.5 0.5\n"
		L">> MIXER 1-10 FILL 0.5 0.5 2 2\n"
		L">> MIXER 1-10 ROTATION 45",
		L"For a 45 degree gradient covering the screen.");
}

spl::shared_ptr<frame_producer> create_color_producer(const frame_producer_dependencies& dependencies, const std::vector<std::wstring>& params)
{
	if(params.size() == 0)
		return core::frame_producer::empty();

	int start_pos = 0;
	if (params.size() > 1 && boost::iequals(params.at(0), L"[color]"))
		start_pos = 1;

	uint32_t value = 0;
	if(!try_get_color(params.at(start_pos), value))
		return core::frame_producer::empty();

	std::vector<std::wstring> colors;

	for (auto& param : params)
	{
		if (try_get_color(param, value))
			colors.push_back(param);
	}

	auto colors_str = boost::join(colors, L" ");
	return spl::make_shared<color_producer>(dependencies.frame_factory, colors_str, dependencies.format_desc.width, dependencies.format_desc.height);
}

}}
