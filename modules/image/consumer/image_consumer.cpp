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

#include "image_consumer.h"

#include <common/except.h>
#include <common/env.h>
#include <common/log.h>
#include <common/utf.h>
#include <common/array.h>
#include <common/future.h>
#include <common/os/general_protection_fault.h>

#include <core/consumer/frame_consumer.h>
#include <core/video_format.h>
#include <core/frame/frame.h>
#include <core/help/help_sink.h>
#include <core/help/help_repository.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string.hpp>

#include <tbb/concurrent_queue.h>

#include <asmlib.h>

#include <FreeImage.h>

#include <vector>
#include <algorithm>

#include "../util/image_view.h"

namespace caspar { namespace image {

void write_cropped_png(
		const core::const_frame& frame,
		const core::video_format_desc& format_desc,
		const boost::filesystem::path& output_file,
		int width,
		int height)
{
	auto bitmap = std::shared_ptr<FIBITMAP>(FreeImage_Allocate(width, height, 32), FreeImage_Unload);
	image_view<bgra_pixel> destination_view(FreeImage_GetBits(bitmap.get()), width, height);
	image_view<bgra_pixel> complete_frame(const_cast<uint8_t*>(frame.image_data().begin()), format_desc.width, format_desc.height);
	auto thumbnail_view = complete_frame.subview(0, 0, width, height);

	std::copy(thumbnail_view.begin(), thumbnail_view.end(), destination_view.begin());
	FreeImage_FlipVertical(bitmap.get());
	FreeImage_SaveU(FIF_PNG, bitmap.get(), output_file.wstring().c_str(), 0);
}

struct image_consumer : public core::frame_consumer
{
	core::monitor::subject	monitor_subject_;
	std::wstring			filename_;
public:

	// frame_consumer

	image_consumer(const std::wstring& filename)
		: filename_(filename)
	{
	}

	void initialize(const core::video_format_desc&, const core::audio_channel_layout&, int) override
	{
	}

	int64_t presentation_frame_age_millis() const override
	{
		return 0;
	}

	std::future<bool> send(core::const_frame frame) override
	{
		auto filename = filename_;

		boost::thread async([frame, filename]
		{
			ensure_gpf_handler_installed_for_thread("image-consumer");

			try
			{
				auto filename2 = filename;
				
				if (filename2.empty())
					filename2 = env::media_folder() +  boost::posix_time::to_iso_wstring(boost::posix_time::second_clock::local_time()) + L".png";
				else
					filename2 = env::media_folder() + filename2 + L".png";

				auto bitmap = std::shared_ptr<FIBITMAP>(FreeImage_Allocate(static_cast<int>(frame.width()), static_cast<int>(frame.height()), 32), FreeImage_Unload);
				A_memcpy(FreeImage_GetBits(bitmap.get()), frame.image_data().begin(), frame.image_data().size());
				FreeImage_FlipVertical(bitmap.get());
#ifdef WIN32
				FreeImage_SaveU(FIF_PNG, bitmap.get(), filename2.c_str(), 0);
#else
				FreeImage_Save(FIF_PNG, bitmap.get(), u8(filename2).c_str(), 0);
#endif
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		});
		async.detach();

		return make_ready_future(false);
	}

	std::wstring print() const override
	{
		return L"image[]";
	}
	
	std::wstring name() const override
	{
		return L"image";
	}

	boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"image");
		return info;
	}

	int buffer_depth() const override
	{
		return -1;
	}

	int index() const override
	{
		return 100;
	}

	core::monitor::subject& monitor_output()
	{
		return monitor_subject_;
	}
};

void describe_consumer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Writes a single PNG snapshot of a video channel.");
	sink.syntax(L"IMAGE {[filename:string]|yyyyMMddTHHmmss}");
	sink.para()
		->text(L"Writes a single PNG snapshot of a video channel. ")->code(L".png")->text(L" will be appended to ")
		->code(L"filename")->text(L". The PNG image will be stored under the ")->code(L"media")->text(L" folder.");
	sink.para()->text(L"Examples:");
	sink.example(L">> ADD 1 IMAGE screenshot", L"creating media/screenshot.png");
	sink.example(L">> ADD 1 IMAGE", L"creating media/20130228T210946.png if the current time is 2013-02-28 21:09:46.");
}

spl::shared_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>& params, core::interaction_sink*)
{
	if (params.size() < 1 || !boost::iequals(params.at(0), L"IMAGE"))
		return core::frame_consumer::empty();

	std::wstring filename;

	if (params.size() > 1)
		filename = params.at(1);

	return spl::make_shared<image_consumer>(filename);
}

}}
