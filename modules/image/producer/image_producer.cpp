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

#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>
#include <core/mixer/write_frame.h>

#include <common/env.h>
#include <common/log/log.h>

#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>

namespace caspar { namespace image {

struct image_producer : public core::frame_producer
{	
	const std::string filename_;
	safe_ptr<core::basic_frame> frame_;
	
	explicit image_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::string& filename) 
		: filename_(filename)
		, frame_(core::basic_frame::empty())	
	{
		auto bitmap = load_image(filename_);
		FreeImage_FlipVertical(bitmap.get());
		
		core::pixel_format_desc desc;
		desc.pix_fmt = core::pixel_format::bgra;
		desc.planes.push_back(core::pixel_format_desc::plane(FreeImage_GetWidth(bitmap.get()), FreeImage_GetHeight(bitmap.get()), 4));
		auto frame = frame_factory->create_frame(this, desc);

		std::copy_n(FreeImage_GetBits(bitmap.get()), frame->image_data().size(), frame->image_data().begin());
		frame->commit();
		frame_ = std::move(frame);

		CASPAR_LOG(info) << print() << " Initialized";
	}
	
	// frame_producer

	virtual safe_ptr<core::basic_frame> receive(int) override
	{
		return frame_;
	}
		
	virtual safe_ptr<core::basic_frame> last_frame() const override
	{
		return frame_;
	}

	virtual std::string print() const override
	{
		return "image_producer[" + filename_ + "]";
	}

	virtual boost::property_tree::ptree info() const override
	{
		boost::property_tree::ptree info;
		info.add("type", "image-producer");
		info.add("filename", filename_);
		return info;
	}
};

safe_ptr<core::frame_producer> create_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::string>& params)
{
	static const std::vector<std::string> extensions = boost::assign::list_of("png")("tga")("bmp")("jpg")("jpeg")("gif")("tiff")("tif")("jp2")("jpx")("j2k")("j2c");
	std::string filename = env::media_folder() + "\\" + params[0];
	
	auto ext = std::find_if(extensions.begin(), extensions.end(), [&](const std::string& ex) -> bool
		{					
			return boost::filesystem::is_regular_file(boost::filesystem::path(filename).replace_extension(ex));
		});

	if(ext == extensions.end())
		return core::frame_producer::empty();

	return make_safe<image_producer>(frame_factory, filename + "." + *ext);
}


}}