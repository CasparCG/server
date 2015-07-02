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
* Author: Helge Norberg, helge.norberg@svt.se
*/

#include "image_scroll_producer.h"

#include "../util/image_loader.h"
#include "../util/image_view.h"
#include "../util/image_algorithms.h"

#include <core/video_format.h>

#include <core/frame/frame.h>
#include <core/frame/draw_frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/frame_transform.h>
#include <core/frame/pixel_format.h>
#include <core/monitor/monitor.h>
#include <core/help/help_sink.h>
#include <core/help/help_repository.h>

#include <common/env.h>
#include <common/log.h>
#include <common/except.h>
#include <common/array.h>
#include <common/tweener.h>
#include <common/param.h>
#include <common/os/filesystem.h>

#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/scoped_array.hpp>

#include <algorithm>
#include <array>

namespace caspar { namespace image {
		
struct image_scroll_producer : public core::frame_producer_base
{	
	core::monitor::subject			monitor_subject_;

	const std::wstring				filename_;
	std::vector<core::draw_frame>	frames_;
	core::video_format_desc			format_desc_;
	int								width_;
	int								height_;
	core::constraints				constraints_;

	double							delta_				= 0.0;
	double							speed_;

	int								start_offset_x_		= 0;
	int								start_offset_y_		= 0;
	bool							progressive_;
	
	explicit image_scroll_producer(
			const spl::shared_ptr<core::frame_factory>& frame_factory,
			const core::video_format_desc& format_desc,
			const std::wstring& filename,
			double speed,
			double duration,
			int motion_blur_px = 0,
			bool premultiply_with_alpha = false,
			bool progressive = false)
		: filename_(filename)
		, format_desc_(format_desc)
		, speed_(speed)
		, progressive_(progressive)
	{
		auto bitmap = load_image(filename_);
		FreeImage_FlipVertical(bitmap.get());

		width_  = FreeImage_GetWidth(bitmap.get());
		height_ = FreeImage_GetHeight(bitmap.get());
		constraints_.width.set(width_);
		constraints_.height.set(height_);

		bool vertical = width_ == format_desc_.width;
		bool horizontal = height_ == format_desc_.height;

		if (!vertical && !horizontal)
			BOOST_THROW_EXCEPTION(
				caspar::invalid_argument() << msg_info("Neither width nor height matched the video resolution"));

		if (vertical)
		{
			if (duration != 0.0)
			{
				double total_num_pixels = format_desc_.height * 2 + height_;

				speed_ = total_num_pixels / (duration * format_desc_.fps * static_cast<double>(format_desc_.field_count));

				if (std::abs(speed_) > 1.0)
					speed_ = std::ceil(speed_);
			}

			if (speed_ < 0.0)
			{
				start_offset_y_ = height_ + format_desc_.height;
			}
		}
		else
		{
			if (duration != 0.0)
			{
				double total_num_pixels = format_desc_.width * 2 + width_;

				speed_ = total_num_pixels / (duration * format_desc_.fps * static_cast<double>(format_desc_.field_count));

				if (std::abs(speed_) > 1.0)
					speed_ = std::ceil(speed_);
			}

			if (speed_ > 0.0)
				start_offset_x_ = format_desc_.width - (width_ % format_desc_.width);
			else
				start_offset_x_ = format_desc_.width - (width_ % format_desc_.width) + width_ + format_desc_.width;
		}

		auto bytes = FreeImage_GetBits(bitmap.get());
		auto count = width_*height_*4;
		image_view<bgra_pixel> original_view(bytes, width_, height_);

		if (premultiply_with_alpha)
			premultiply(original_view);

		boost::scoped_array<uint8_t> blurred_copy;

		if (motion_blur_px > 0)
		{
			double angle = 3.14159265 / 2; // Up

			if (horizontal && speed_ < 0)
				angle *= 2; // Left
			else if (vertical && speed > 0)
				angle *= 3; // Down
			else if (horizontal && speed  > 0)
				angle = 0.0; // Right

			blurred_copy.reset(new uint8_t[count]);
			image_view<bgra_pixel> blurred_view(blurred_copy.get(), width_, height_);
			caspar::tweener blur_tweener(L"easeInQuad");
			blur(original_view, blurred_view, angle, motion_blur_px, blur_tweener);
			bytes = blurred_copy.get();
			bitmap.reset();
		}

		if (vertical)
		{
			int n = 1;

			while(count > 0)
			{
				core::pixel_format_desc desc = core::pixel_format::bgra;
				desc.planes.push_back(core::pixel_format_desc::plane(width_, format_desc_.height, 4));
				auto frame = frame_factory->create_frame(reinterpret_cast<void*>(rand()), desc);

				if(count >= frame.image_data(0).size())
				{	
					std::copy_n(bytes + count - frame.image_data(0).size(), frame.image_data(0).size(), frame.image_data(0).begin());
					count -= static_cast<int>(frame.image_data(0).size());
				}
				else
				{
					memset(frame.image_data(0).begin(), 0, frame.image_data(0).size());	
					std::copy_n(bytes, count, frame.image_data(0).begin() + format_desc_.size - count);
					count = 0;
				}

				core::draw_frame draw_frame(std::move(frame));

				// Set the relative position to the other image fragments
				draw_frame.transform().image_transform.fill_translation[1] = - n++;

				frames_.push_back(draw_frame);
			}
		}
		else if (horizontal)
		{
			int i = 0;
			while(count > 0)
			{
				core::pixel_format_desc desc = core::pixel_format::bgra;
				desc.planes.push_back(core::pixel_format_desc::plane(format_desc_.width, height_, 4));
				auto frame = frame_factory->create_frame(reinterpret_cast<void*>(rand()), desc);
				if(count >= frame.image_data(0).size())
				{	
					for(int y = 0; y < height_; ++y)
						std::copy_n(bytes + i * format_desc_.width*4 + y * width_*4, format_desc_.width*4, frame.image_data(0).begin() + y * format_desc_.width*4);
					
					++i;
					count -= static_cast<int>(frame.image_data(0).size());
				}
				else
				{
					memset(frame.image_data(0).begin(), 0, frame.image_data(0).size());	
					auto width2 = width_ % format_desc_.width;
					for(int y = 0; y < height_; ++y)
						std::copy_n(bytes + i * format_desc_.width*4 + y * width_*4, width2*4, frame.image_data(0).begin() + y * format_desc_.width*4);

					count = 0;
				}
			
				frames_.push_back(core::draw_frame(std::move(frame)));
			}

			std::reverse(frames_.begin(), frames_.end());

			// Set the relative positions of the image fragments.
			for (size_t n = 0; n < frames_.size(); ++n)
			{
				double translation = - (static_cast<double>(n) + 1.0);
				frames_[n].transform().image_transform.fill_translation[0] = translation;
			}
		}

		CASPAR_LOG(info) << print() << L" Initialized";
	}

	std::vector<core::draw_frame> get_visible()
	{
		std::vector<core::draw_frame> result;
		result.reserve(frames_.size());

		for (auto& frame : frames_)
		{
			auto& fill_translation = frame.transform().image_transform.fill_translation;

			if (width_ == format_desc_.width)
			{
				auto motion_offset_in_screens = (static_cast<double>(start_offset_y_) + delta_) / static_cast<double>(format_desc_.height);
				auto vertical_offset = fill_translation[1] + motion_offset_in_screens;

				if (vertical_offset < -1.0 || vertical_offset > 1.0)
				{
					continue;
				}
			}
			else
			{
				auto motion_offset_in_screens = (static_cast<double>(start_offset_x_) + delta_) / static_cast<double>(format_desc_.width);
				auto horizontal_offset = fill_translation[0] + motion_offset_in_screens;

				if (horizontal_offset < -1.0 || horizontal_offset > 1.0)
				{
					continue;
				}
			}

			result.push_back(frame);
		}

		return std::move(result);
	}
	
	// frame_producer
	core::draw_frame render_frame(bool allow_eof)
	{
		if(frames_.empty())
			return core::draw_frame::empty();
		
		core::draw_frame result(get_visible());
		auto& fill_translation = result.transform().image_transform.fill_translation;

		if (width_ == format_desc_.width)
		{
			if (static_cast<size_t>(std::abs(delta_)) >= height_ + format_desc_.height && allow_eof)
				return core::draw_frame::empty();

			fill_translation[1] = 
				static_cast<double>(start_offset_y_) / static_cast<double>(format_desc_.height)
				+ delta_ / static_cast<double>(format_desc_.height);
		}
		else
		{
			if (static_cast<size_t>(std::abs(delta_)) >= width_ + format_desc_.width && allow_eof)
				return core::draw_frame::empty();

			fill_translation[0] = 
				static_cast<double>(start_offset_x_) / static_cast<double>(format_desc_.width)
				+ (delta_) / static_cast<double>(format_desc_.width);
		}

		return result;
	}

	core::draw_frame render_frame(bool allow_eof, bool advance_delta)
	{
		auto result = render_frame(allow_eof);

		if (advance_delta)
		{
			advance();
		}

		return result;
	}

	void advance()
	{
		delta_ += speed_;
	}

	core::draw_frame receive_impl() override
	{
		core::draw_frame result;

		if (format_desc_.field_mode == core::field_mode::progressive || progressive_)
		{
			result = render_frame(true, true);
		}
		else
		{
			auto field1 = render_frame(true, true);
			auto field2 = render_frame(true, false);

			if (field1 != core::draw_frame::empty() && field2 == core::draw_frame::empty())
			{
				field2 = render_frame(false, true);
			}
			else
			{
				advance();
			}

			result = core::draw_frame::interlace(field1, field2, format_desc_.field_mode);
		}
		
		monitor_subject_ << core::monitor::message("/file/path") % filename_
						 << core::monitor::message("/delta") % delta_ 
						 << core::monitor::message("/speed") % speed_;

		return result;
	}

	core::constraints& pixel_constraints() override
	{
		return constraints_;
	}
				
	std::wstring print() const override
	{
		return L"image_scroll_producer[" + filename_ + L"]";
	}

	std::wstring name() const override
	{
		return L"image-scroll";
	}

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"image-scroll");
		info.add(L"filename", filename_);
		return info;
	}

	uint32_t nb_frames() const override
	{
		if(width_ == format_desc_.width)
		{
			auto length = (height_ + format_desc_.height * 2);
			return static_cast<uint32_t>(length / std::abs(speed_));// + length % std::abs(delta_));
		}
		else
		{
			auto length = (width_ + format_desc_.width * 2);
			return static_cast<uint32_t>(length / std::abs(speed_));// + length % std::abs(delta_));
		}
	}

	core::monitor::subject& monitor_output()
	{
		return monitor_subject_;
	}
};

void describe_scroll_producer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Scrolls an image either horizontally or vertically.");
	sink.syntax(L"[image_file:string] SPEED [speed:float] {BLUR [blur_px:int]} {[premultiply:PREMULTIPLY]} {[progressive:PROGRESSIVE]}");
	sink.para()
		->text(L"Scrolls an image either horizontally or vertically. ")
		->text(L"It is the image dimensions that decide if it will be a vertical scroll or a horizontal scroll. ")
		->text(L"A horizontal scroll will be selected if the image height is exactly the same as the video format height. ")
		->text(L"A vertical scroll will be selected if the image width is exactly the same as the video format width.");
	sink.definitions()
		->item(L"image_file", L"The image without extension. The file has to have either the same width or the same height as the video format.")
		->item(L"speed", L"A positive or negative float defining how many pixels to move the image each frame.")
		->item(L"blur_px", L"If specified, will do a directional blur in the scrolling direction by the given number of pixels.")
		->item(L"premultiply", L"If the image is in straight alpha, use this option to make it display correctly in CasparCG.")
		->item(L"progressive", L"When an interlaced video format is used, by default the image is moved every field. This can be overridden by specifying this option, causing the image to only move on full frames.");
	sink.para()->text(L"If ")->code(L"SPEED [speed]")->text(L" is ommitted, the ordinary ")->see(L"Image Producer")->text(L" will be used instead.");
	sink.example(L">> PLAY 1-10 cred_1280 SPEED 8 BLUR 2", L"Given that cred_1280 is a as wide as the video mode, this will create a rolling end credits with a little bit of blur and a speed of 8 pixels per frame.");
}

spl::shared_ptr<core::frame_producer> create_scroll_producer(const core::frame_producer_dependencies& dependencies, const std::vector<std::wstring>& params)
{
	static const auto extensions = {
		L".png",
		L".tga",
		L".bmp",
		L".jpg",
		L".jpeg",
		L".gif",
		L".tiff",
		L".tif",
		L".jp2",
		L".jpx",
		L".j2k",
		L".j2c"
	};
	std::wstring filename = env::media_folder() + params.at(0);
	
	auto ext = std::find_if(extensions.begin(), extensions.end(), [&](const std::wstring& ex) -> bool
	{
		auto file = caspar::find_case_insensitive(boost::filesystem::path(filename).replace_extension(ex).wstring());

		return static_cast<bool>(file);
	});

	if(ext == extensions.end())
		return core::frame_producer::empty();
	
	double duration = 0.0;
	double speed = get_param(L"SPEED", params, 0.0);

	if (speed == 0)
		duration = get_param(L"DURATION", params, 0.0);

	if(speed == 0 && duration == 0)
		return core::frame_producer::empty();

	int motion_blur_px = get_param(L"BLUR", params, 0);

	bool premultiply_with_alpha = contains_param(L"PREMULTIPLY", params);
	bool progressive = contains_param(L"PROGRESSIVE", params);

	return core::create_destroy_proxy(spl::make_shared<image_scroll_producer>(
			dependencies.frame_factory,
			dependencies.format_desc,
			*caspar::find_case_insensitive(filename + *ext),
			-speed,
			-duration,
			motion_blur_px,
			premultiply_with_alpha,
			progressive));
}

}}
