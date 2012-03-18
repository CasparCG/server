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

#include "image_producer.h"

#include "../util/image_loader.h"

#include <core/video_format.h>

#include <core/producer/frame_producer.h>
#include <core/frame/frame.h>
#include <core/frame/draw_frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/pixel_format.h>
#include <core/monitor/monitor.h>

#include <common/env.h>
#include <common/log.h>
#include <common/array.h>

#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>

using namespace boost::assign;

namespace caspar { namespace image {

struct image_producer : public core::frame_producer_base
{	
	monitor::basic_subject	event_subject_;
	const std::wstring		filename_;
	core::draw_frame		frame_;
	
	explicit image_producer(const spl::shared_ptr<core::frame_factory>& frame_factory, const std::wstring& filename) 
		: filename_(filename)
		, frame_(core::draw_frame::empty())	
	{
		auto bitmap = load_image(filename_);
		FreeImage_FlipVertical(bitmap.get());
		
		core::pixel_format_desc desc = core::pixel_format::bgra;
		desc.planes.push_back(core::pixel_format_desc::plane(FreeImage_GetWidth(bitmap.get()), FreeImage_GetHeight(bitmap.get()), 4));
		auto frame = frame_factory->create_frame(this, desc);

		std::copy_n(FreeImage_GetBits(bitmap.get()), frame.image_data(0).size(), frame.image_data(0).begin());
		frame_ = core::draw_frame(std::move(frame));

		CASPAR_LOG(info) << print() << L" Initialized";
	}
	
	// frame_producer

	core::draw_frame receive_impl() override
	{
		event_subject_ << monitor::event("file/path") % filename_;

		return frame_;
	}
			
	std::wstring print() const override
	{
		return L"image_producer[" + filename_ + L"]";
	}

	std::wstring name() const override
	{
		return L"image";
	}

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"image");
		info.add(L"filename", filename_);
		return info;
	}

	void subscribe(const monitor::observable::observer_ptr& o) override															
	{
		return event_subject_.subscribe(o);
	}

	void unsubscribe(const monitor::observable::observer_ptr& o) override		
	{
		return event_subject_.unsubscribe(o);
	}
};

spl::shared_ptr<core::frame_producer> create_producer(const spl::shared_ptr<core::frame_factory>& frame_factory, const core::video_format_desc& format_desc, const std::vector<std::wstring>& params)
{
	static const std::vector<std::wstring> extensions = list_of(L".png")(L".tga")(L".bmp")(L".jpg")(L".jpeg")(L".gif")(L".tiff")(L".tif")(L".jp2")(L".jpx")(L".j2k")(L".j2c");
	std::wstring filename = env::media_folder() + L"\\" + params[0];
	
	auto ext = std::find_if(extensions.begin(), extensions.end(), [&](const std::wstring& ex) -> bool
		{			
			return boost::filesystem::is_regular_file(boost::filesystem::path(filename).replace_extension(ex));
		});

	if(ext == extensions.end())
		return core::frame_producer::empty();

	return spl::make_shared<image_producer>(frame_factory, filename + *ext);
}


}}