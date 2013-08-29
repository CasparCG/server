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

#include "../../stdafx.h"

#include "text_producer.h"

#include <core/producer/frame_producer.h>
#include <core/producer/color/color_producer.h>
#include <core/producer/variable.h>
#include <core/frame/geometry.h>
#include <core/frame/frame.h>
#include <core/frame/draw_frame.h>

#include <core/frame/frame_factory.h>
#include <core/frame/pixel_format.h>
#include <core/monitor/monitor.h>

#include <core/consumer/frame_consumer.h>
#include <modules/image/consumer/image_consumer.h>

#include <common/except.h>
#include <common/array.h>
#include <common/env.h>
#include <common/future.h>
#include <common/param.h>
#include <common/os/windows/system_info.h>
#include <memory>

#include <asmlib.h>
#include <FreeImage.h>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/filesystem.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "utils\texture_atlas.h"
#include "utils\texture_font.h"

class font_comparer {
	const std::wstring& lhs;
public:
	explicit font_comparer(const std::wstring& p) : lhs(p) {}
	bool operator()(const std::pair<std::wstring, std::wstring>&rhs) { return boost::iequals(lhs, rhs.first); }
};


namespace caspar { namespace core {
	namespace text {

		using namespace boost::filesystem3;

		std::map<std::wstring, std::wstring> fonts;

		std::map<std::wstring, std::wstring> enumerate_fonts()
		{
			std::map<std::wstring, std::wstring> result;

			FT_Library lib;
			FT_Error err = FT_Init_FreeType(&lib);
			if(err) 
				return result;

			auto fonts = directory_iterator(env::system_font_folder());
			auto end = directory_iterator();
			for(; fonts != end; ++fonts)
			{
				auto file = (*fonts);
				if(is_regular_file(file.path()))
				{
					FT_Face face;
					err = FT_New_Face(lib, u8(file.path().native()).c_str(), 0, &face);
					if(err) 
						continue;

					const char* fontname = FT_Get_Postscript_Name(face);	//this doesn't work for .fon fonts. Ignoring those for now
					if(fontname != nullptr)
					{
						std::string fontname_str(fontname);
						result.insert(std::pair<std::wstring, std::wstring>(std::wstring(fontname_str.begin(), fontname_str.end()), file.path().native()));
					}

					FT_Done_Face(face);
				}
			}

			FT_Done_FreeType(lib);

			return result;
		}

		void init()
		{
			fonts.swap(std::move(enumerate_fonts()));
			if(fonts.size() > 0)
				register_producer_factory(&create_text_producer);
		}

		text_info& find_font_file(text_info& info)
		{
			auto& font_name = info.font;
			auto it = std::find_if(fonts.begin(), fonts.end(), font_comparer(font_name));
			info.font_file = (it != fonts.end()) ? (*it).second : L"";
			return info;
		}
	}
	

struct text_producer::impl
{
	spl::shared_ptr<core::frame_factory> frame_factory_;
	constraints constraints_;
	int x_, y_, parent_width_, parent_height_;
	bool standalone_;
	variable_impl<std::wstring> text_;
	std::shared_ptr<void> text_subscription_;
	variable_impl<double> tracking_;
	std::shared_ptr<void> tracking_subscription_;
	variable_impl<double> current_bearing_y_;
	variable_impl<double> current_protrude_under_y_;
	draw_frame frame_;
	text::texture_atlas atlas_;
	text::texture_font font_;
	bool dirty_;

public:
	explicit impl(const spl::shared_ptr<frame_factory>& frame_factory, int x, int y, const std::wstring& str, text::text_info& text_info, long parent_width, long parent_height, bool standalone) 
		: frame_factory_(frame_factory)
		, constraints_(parent_width, parent_height)
		, x_(x), y_(y), parent_width_(parent_width), parent_height_(parent_height)
		, standalone_(standalone)
		, atlas_(512,512,4)
		, font_(atlas_, text::find_font_file(text_info), !standalone)
		, dirty_(false)
	{
		//TODO: examine str to determine which unicode_blocks to load
		font_.load_glyphs(text::Basic_Latin, text_info.color);
		font_.load_glyphs(text::Latin_1_Supplement, text_info.color);
		font_.load_glyphs(text::Latin_Extended_A, text_info.color);

		tracking_.value().set(text_info.tracking);
		text_subscription_ = text_.value().on_change([this]()
		{
			dirty_ = true;
		});
		tracking_subscription_ = tracking_.value().on_change([this]()
		{
			dirty_ = true;
		});

		constraints_.height.depend_on(text());
		constraints_.width.depend_on(text());
		current_bearing_y_.as<double>().depend_on(text());
		current_protrude_under_y_.as<double>().depend_on(text());

		//generate frame
		text_.value().set(str);

		CASPAR_LOG(info) << print() << L" Initialized";
	}

	void generate_frame()
	{
		core::pixel_format_desc pfd(core::pixel_format::bgra);
		pfd.planes.push_back(core::pixel_format_desc::plane(static_cast<int>(atlas_.width()), static_cast<int>(atlas_.height()), static_cast<int>(atlas_.depth())));

		text::string_metrics metrics;
		font_.set_tracking(static_cast<int>(tracking_.value().get()));
		std::vector<float> vertex_stream(std::move(font_.create_vertex_stream(text_.value().get(), x_, y_, parent_width_, parent_height_, &metrics)));
		auto frame = frame_factory_->create_frame(vertex_stream.data(), pfd);
		memcpy(frame.image_data().data(), atlas_.data(), frame.image_data().size());
		frame.set_geometry(frame_geometry(frame_geometry::quad_list, std::move(vertex_stream)));

		this->constraints_.width.set(metrics.width);
		this->constraints_.height.set(metrics.height);
		current_bearing_y_.value().set(metrics.bearingY);
		current_protrude_under_y_.value().set(metrics.protrudeUnderY);
		frame_ = core::draw_frame(std::move(frame));
		frame_.transform().image_transform.fill_translation[1] = static_cast<double>(metrics.bearingY) / static_cast<double>(metrics.height);
	}

	text::string_metrics measure_string(const std::wstring& str)
	{
		return font_.measure_string(str);
	}

	// frame_producer
			
	draw_frame receive_impl()
	{
		if (dirty_)
			generate_frame();

		return frame_;
	}

	boost::unique_future<std::wstring> call(const std::vector<std::wstring>& param)
	{
		std::wstring result;
		text_.value().set(param.empty() ? L"" : param[0]);

		return async(launch::deferred, [=]{return result;});
	}

	variable& get_variable(const std::wstring& name)
	{
		if (name == L"text")
			return text_;
		else if (name == L"current_bearing_y")
			return current_bearing_y_;
		else if (name == L"current_protrude_under_y")
			return current_protrude_under_y_;
		else if (name == L"tracking")
			return tracking_;

		CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(L"text_producer does not have a variable called " + name));
	}

	const std::vector<std::wstring>& get_variables() const
	{
		static std::vector<std::wstring> vars =
				boost::assign::list_of<std::wstring>
						(L"text")
						(L"tracking")
						(L"current_bearing_y")
						(L"current_protrude_under_y");

		return vars;
	}

	constraints& pixel_constraints()
	{
		return constraints_;
	}

	binding<std::wstring>& text()
	{
		return text_.value();
	}

	binding<double>& tracking()
	{
		return tracking_.value();
	}

	const binding<double>& current_bearing_y() const
	{
		return current_bearing_y_.value();
	}

	const binding<double>& current_protrude_under_y() const
	{
		return current_protrude_under_y_.value();
	}
	
	std::wstring print() const
	{
		return L"text[" + text_.value().get() + L"]";
	}

	std::wstring name() const
	{
		return L"text";
	}
	
	boost::property_tree::wptree info() const
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"text");
		info.add(L"text", text_.value().get());
		return info;
	}
};

text_producer::text_producer(const spl::shared_ptr<frame_factory>& frame_factory, int x, int y, const std::wstring& str, text::text_info& text_info, long parent_width, long parent_height, bool standalone)
	: impl_(new impl(frame_factory, x, y, str, text_info, parent_width, parent_height, standalone))
{}

draw_frame text_producer::receive_impl() { return impl_->receive_impl(); }
boost::unique_future<std::wstring> text_producer::call(const std::vector<std::wstring>& param) { return impl_->call(param); }
variable& text_producer::get_variable(const std::wstring& name) { return impl_->get_variable(name); }
const std::vector<std::wstring>& text_producer::get_variables() const { return impl_->get_variables(); }
text::string_metrics text_producer::measure_string(const std::wstring& str) { return impl_->measure_string(str); }

constraints& text_producer::pixel_constraints() { return impl_->pixel_constraints(); }
std::wstring text_producer::print() const { return impl_->print(); }
std::wstring text_producer::name() const { return impl_->name(); }
boost::property_tree::wptree text_producer::info() const { return impl_->info(); }
void text_producer::subscribe(const monitor::observable::observer_ptr& o) {}
void text_producer::unsubscribe(const monitor::observable::observer_ptr& o) {}
binding<std::wstring>& text_producer::text() { return impl_->text(); }
binding<double>& text_producer::tracking() { return impl_->tracking(); }
const binding<double>& text_producer::current_bearing_y() const { return impl_->current_bearing_y(); }
const binding<double>& text_producer::current_protrude_under_y() const { return impl_->current_protrude_under_y(); }

spl::shared_ptr<text_producer> text_producer::create(const spl::shared_ptr<frame_factory>& frame_factory, int x, int y, const std::wstring& str, text::text_info& text_info, long parent_width, long parent_height, bool standalone)
{
	return spl::make_shared<text_producer>(frame_factory, x, y, str, text_info, parent_width, parent_height, standalone);
}

spl::shared_ptr<frame_producer> create_text_producer(const spl::shared_ptr<frame_factory>& frame_factory, const video_format_desc& format_desc, const std::vector<std::wstring>& params)
{
	if(params.size() < 2 || !boost::iequals(params.at(0), L"[text]"))
		return core::frame_producer::empty();

	int x = 0, y = 0;
	if(params.size() >= 4)
	{
		x = boost::lexical_cast<int>(params.at(2));
		y = boost::lexical_cast<int>(params.at(3));
	}

	text::text_info text_info;
	text_info.font = get_param(L"FONT", params, L"verdana");
	text_info.size = static_cast<float>(get_param(L"SIZE", params, 30.0)); // 30.0f does not seem to work to get as float directly
	
	std::wstring col_str = get_param(L"color", params, L"#ffffffff");
	uint32_t col_val = 0xffffffff;
	try_get_color(col_str, col_val);
	text_info.color = core::text::color<float>(col_val);

	bool standalone = get_param(L"STANDALONE", params, false);

	return text_producer::create(frame_factory, x, y, params.at(1), text_info, format_desc.width, format_desc.height, standalone);
}

}}
