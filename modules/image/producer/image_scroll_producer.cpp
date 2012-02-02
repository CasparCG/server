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

#include "image_scroll_producer.h"

#include "../util/image_loader.h"

#include <core/video_format.h>

#include <core/frame/draw_frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/frame_transform.h>
#include <core/frame/pixel_format.h>
#include <core/mixer/gpu/write_frame.h>

#include <common/env.h>
#include <common/log.h>
#include <common/except.h>

#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <array>

using namespace boost::assign;

namespace caspar { namespace image {
		
struct image_scroll_producer : public core::frame_producer
{	
	const std::wstring							filename_;
	std::vector<safe_ptr<core::draw_frame>>	frames_;
	core::video_format_desc						format_desc_;
	int										width_;
	int										height_;

	int											delta_;
	int											speed_;

	std::array<double, 2>						start_offset_;

	safe_ptr<core::draw_frame>					last_frame_;
	
	explicit image_scroll_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filename, int speed) 
		: filename_(filename)
		, delta_(0)
		, format_desc_(frame_factory->get_video_format_desc())
		, speed_(speed)
		, last_frame_(core::draw_frame::empty())
	{
		start_offset_.assign(0.0);

		auto bitmap = load_image(filename_);
		FreeImage_FlipVertical(bitmap.get());

		width_  = FreeImage_GetWidth(bitmap.get());
		height_ = FreeImage_GetHeight(bitmap.get());

		auto bytes = FreeImage_GetBits(bitmap.get());
		int count = width_*height_*4;

		if(height_ > format_desc_.height)
		{
			while(count > 0)
			{
				core::pixel_format_desc desc = core::pixel_format::bgra;
				desc.planes.push_back(core::pixel_format_desc::plane(width_, format_desc_.height, 4));
				auto frame = frame_factory->create_frame(reinterpret_cast<void*>(rand()), desc);

				if(count >= frame->image_data(0).size())
				{	
					std::copy_n(bytes + count - frame->image_data(0).size(), frame->image_data(0).size(), frame->image_data(0).begin());
					count -= static_cast<int>(frame->image_data(0).size());
				}
				else
				{
					memset(frame->image_data(0).begin(), 0, frame->image_data(0).size());	
					std::copy_n(bytes, count, frame->image_data(0).begin() + format_desc_.size - count);
					count = 0;
				}
			
				frames_.push_back(frame);
			}
			
			if(speed_ < 0.0)
			{
				auto offset = format_desc_.height - (height_ % format_desc_.height);
				auto offset2 = offset * 0.5/static_cast<double>(format_desc_.height);
				start_offset_[1] = (std::ceil(static_cast<double>(height_) / static_cast<double>(format_desc_.height)) + 1.0) * 0.5 - offset2;// - 1.5;
			}
		}
		else
		{
			int i = 0;
			while(count > 0)
			{
				core::pixel_format_desc desc = core::pixel_format::bgra;
				desc.planes.push_back(core::pixel_format_desc::plane(format_desc_.width, height_, 4));
				auto frame = frame_factory->create_frame(reinterpret_cast<void*>(rand()), desc);
				if(count >= frame->image_data(0).size())
				{	
					for(int y = 0; y < height_; ++y)
						std::copy_n(bytes + i * format_desc_.width*4 + y * width_*4, format_desc_.width*4, frame->image_data(0).begin() + y * format_desc_.width*4);
					
					++i;
					count -= static_cast<int>(frame->image_data(0).size());
				}
				else
				{
					memset(frame->image_data(0).begin(), 0, frame->image_data(0).size());	
					int width2 = width_ % format_desc_.width;
					for(int y = 0; y < height_; ++y)
						std::copy_n(bytes + i * format_desc_.width*4 + y * width_*4, width2*4, frame->image_data(0).begin() + y * format_desc_.width*4);

					count = 0;
				}
			
				frames_.push_back(frame);
			}

			std::reverse(frames_.begin(), frames_.end());

			if(speed_ > 0.0)
			{
				auto offset = format_desc_.width - (width_ % format_desc_.width);
				start_offset_[0] = offset * 0.5/static_cast<double>(format_desc_.width);
			}
			else
			{
				start_offset_[0] = (std::ceil(static_cast<double>(width_) / static_cast<double>(format_desc_.width)) + 1.0) * 0.5;// - 1.5;
			}
		}

		CASPAR_LOG(info) << print() << L" Initialized";
	}
	
	// frame_producer

	virtual safe_ptr<core::draw_frame> receive(int) override
	{		
		delta_ += speed_;

		if(frames_.empty())
			return core::draw_frame::eof();
		
		if(height_ > format_desc_.height)
		{
			if(static_cast<int>(std::abs(delta_)) >= height_ - format_desc_.height)
				return core::draw_frame::eof();

			for(int n = 0; n < frames_.size(); ++n)
			{
				frames_[n]->get_frame_transform().fill_translation[0] = start_offset_[0];
				frames_[n]->get_frame_transform().fill_translation[1] =	start_offset_[1] - (n+1) + delta_ * 0.5/static_cast<double>(format_desc_.height);
			}
		}
		else
		{
			if(static_cast<int>(std::abs(delta_)) >= width_ - format_desc_.width)
				return core::draw_frame::eof();

			for(int n = 0; n < frames_.size(); ++n)
			{
				frames_[n]->get_frame_transform().fill_translation[0] = start_offset_[0] - (n+1) + delta_ * 0.5/static_cast<double>(format_desc_.width);				
				frames_[n]->get_frame_transform().fill_translation[1] = start_offset_[1];
			}
		}

		return last_frame_ = make_safe<core::draw_frame>(frames_);
	}

	virtual safe_ptr<core::draw_frame> last_frame() const override
	{
		return last_frame_;
	}
		
	virtual std::wstring print() const override
	{
		return L"image_scroll_producer[" + filename_ + L"]";
	}

	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"image-scroll-producer");
		info.add(L"filename", filename_);
		return info;
	}

	virtual uint32_t nb_frames() const override
	{
		if(height_ > format_desc_.height)
		{
			auto length = (height_ - format_desc_.height);
			return length/std::abs(speed_);// + length % std::abs(delta_));
		}
		else
		{
			auto length = (width_ - format_desc_.width);
			auto result = length/std::abs(speed_);// + length % std::abs(delta_));
			return result;
		}
	}
};

safe_ptr<core::frame_producer> create_scroll_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params)
{
	static const std::vector<std::wstring> extensions = list_of(L"png")(L"tga")(L"bmp")(L"jpg")(L"jpeg")(L"gif")(L"tiff")(L"tif")(L"jp2")(L"jpx")(L"j2k")(L"j2c");
	std::wstring filename = env::media_folder() + L"\\" + params[0];
	
	auto ext = std::find_if(extensions.begin(), extensions.end(), [&](const std::wstring& ex) -> bool
		{					
			return boost::filesystem::is_regular_file(boost::filesystem::wpath(filename).replace_extension(ex));
		});

	if(ext == extensions.end())
		return core::frame_producer::empty();
	
	int speed = 0;
	auto speed_it = std::find(params.begin(), params.end(), L"SPEED");
	if(speed_it != params.end())
	{
		if(++speed_it != params.end())
			speed = boost::lexical_cast<int>(*speed_it);
	}

	if(speed == 0)
		return core::frame_producer::empty();

	return create_producer_print_proxy(
			make_safe<image_scroll_producer>(frame_factory, filename + L"." + *ext, speed));
}

}}