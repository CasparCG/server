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
#include <memory>

#include <asmlib.h>
#include <FreeImage.h>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/filesystem.hpp>

#include "utils\texture_atlas.h"
#include "utils\texture_font.h"

namespace caspar { namespace core {

std::wstring find_font_file(const std::wstring& font_name)
{
	std::wstring filename = L"c:\\windows\\fonts\\" + font_name;	//TODO: move font-folder setting to settings
	if(boost::filesystem::exists(filename + L".otf"))
		return filename + L".otf";
	if(boost::filesystem::exists(filename + L".ttf"))
		return filename + L".ttf";
	if(boost::filesystem::exists(filename + L".fon"))
		return filename + L".fon";

	//TODO: Searching by filename is not enough to identify some fonts. we need to extract the font-names somehow
	return L"";
}

struct text_producer::impl
{
	spl::shared_ptr<core::frame_factory>	frame_factory_;
	constraints constraints_;
	int x_, y_, parent_width_, parent_height_;
	bool standalone_;
	std::wstring text_;
	draw_frame frame_;
	text::texture_atlas atlas_;
	text::texture_font font_;

public:
	explicit impl(const spl::shared_ptr<frame_factory>& frame_factory, int x, int y, const std::wstring& str, const text::text_info& text_info, long parent_width, long parent_height, bool standalone) 
		: frame_factory_(frame_factory)
		, constraints_(parent_width, parent_height)
		, x_(x), y_(y), parent_width_(parent_width), parent_height_(parent_height)
		, standalone_(standalone)
		, atlas_(512,512,4)
		, font_(atlas_, find_font_file(text_info.font), text_info.size, !standalone)
	{
		font_.load_glyphs(text::Basic_Latin, text_info.color);
		font_.load_glyphs(text::Latin_1_Supplement, text_info.color);
		font_.load_glyphs(text::Latin_Extended_A, text_info.color);

		//generate frame
		generate_frame(str);

		CASPAR_LOG(info) << print() << L" Initialized";
	}

	void generate_frame(const std::wstring& str)
	{
		core::pixel_format_desc pfd(core::pixel_format::bgra);
		pfd.planes.push_back(core::pixel_format_desc::plane((int)atlas_.width(), (int)atlas_.height(), (int)atlas_.depth()));

		text::string_metrics metrics;
		std::vector<float> vertex_stream(std::move(font_.create_vertex_stream(str, x_, y_, parent_width_, parent_height_, &metrics)));
		auto frame = frame_factory_->create_frame(vertex_stream.data(), pfd);
		memcpy(frame.image_data().data(), atlas_.data(), frame.image_data().size());
		frame.set_geometry(frame_geometry(frame_geometry::quad_list, std::move(vertex_stream)));

		this->constraints_.width.set(metrics.width);
		this->constraints_.height.set(metrics.height);
		frame_ = core::draw_frame(std::move(frame));
	}

	text::string_metrics measure_string(const std::wstring& str)
	{
		return font_.measure_string(str);
	}

	// frame_producer
			
	draw_frame receive_impl()
	{
		return frame_;
	}

	boost::unique_future<std::wstring> call(const std::vector<std::wstring>& param)
	{
		std::wstring result;
		generate_frame(param.empty() ? L"" : param[0]);

		return async(launch::deferred, [=]{return result;});
	}

	constraints& pixel_constraints()
	{
		return constraints_;
	}
	
	std::wstring print() const
	{
		return L"text[" + text_ + L"]";
	}

	std::wstring name() const
	{
		return L"text";
	}
	
	boost::property_tree::wptree info() const
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"text");
		info.add(L"text", text_);
		return info;
	}
};

text_producer::text_producer(const spl::shared_ptr<frame_factory>& frame_factory, int x, int y, const std::wstring& str, const text::text_info& text_info, long parent_width, long parent_height, bool standalone)
	: impl_(new impl(frame_factory, x, y, str, text_info, parent_width, parent_height, standalone))
{}

draw_frame text_producer::receive_impl() { return impl_->receive_impl(); }
boost::unique_future<std::wstring> text_producer::call(const std::vector<std::wstring>& param) { return impl_->call(param); }
text::string_metrics text_producer::measure_string(const std::wstring& str) { return impl_->measure_string(str); }

constraints& text_producer::pixel_constraints() { return impl_->pixel_constraints(); }
std::wstring text_producer::print() const { return impl_->print(); }
std::wstring text_producer::name() const { return impl_->name(); }
boost::property_tree::wptree text_producer::info() const { return impl_->info(); }
void text_producer::subscribe(const monitor::observable::observer_ptr& o) {}
void text_producer::unsubscribe(const monitor::observable::observer_ptr& o) {}

spl::shared_ptr<text_producer> text_producer::create(const spl::shared_ptr<frame_factory>& frame_factory, int x, int y, const std::wstring& str, const text::text_info& text_info, long parent_width, long parent_height, bool standalone)
{
	return spl::make_shared<text_producer>(frame_factory, x, y, str, text_info, parent_width, parent_height, standalone);
}

spl::shared_ptr<frame_producer> create_text_producer(const spl::shared_ptr<frame_factory>& frame_factory, const video_format_desc& format_desc, const std::vector<std::wstring>& params)
{
	if(params.size() < 2 || params.at(0) != L"[TEXT]")
		return core::frame_producer::empty();

	int x = 0, y = 0;
	if(params.size() >= 4)
	{
		x = boost::lexical_cast<int>(params.at(2));
		y = boost::lexical_cast<int>(params.at(3));
	}

	text::text_info text_info;
	text_info.font = L"verdana";
	text_info.size = 30;
	text_info.color = text::color<float>(1.0f, 0, 0, 1.0f);
	return text_producer::create(frame_factory, x, y, params.at(1), text_info, format_desc.width, format_desc.height, true);
}

}}