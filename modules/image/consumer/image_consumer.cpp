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

#include <core/consumer/frame_consumer.h>
#include <core/video_format.h>
#include <core/frame/frame.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>

#include <tbb/concurrent_queue.h>

#include <FreeImage.h>

#include <vector>
#include <algorithm>

#include "../util/image_view.h"
#include "image/util/image_algorithms.h"

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
#ifdef WIN32
	FreeImage_SaveU(FIF_PNG, bitmap.get(), output_file.wstring().c_str(), 0);
#else
	FreeImage_Save(FIF_PNG, bitmap.get(), u8(output_file.wstring()).c_str(), 0);
#endif
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

	void initialize(const core::video_format_desc&, int) override
	{
	}

	std::future<bool> send(core::const_frame frame) override
	{
		auto filename = filename_;

		std::thread async([frame, filename]
		{
			try
			{
				auto filename2 = filename;

				if (filename2.empty())
					filename2 = env::media_folder() +  boost::posix_time::to_iso_wstring(boost::posix_time::second_clock::local_time()) + L".png";
				else
					filename2 = env::media_folder() + filename2 + L".png";

				auto bitmap = std::shared_ptr<FIBITMAP>(FreeImage_Allocate(static_cast<int>(frame.width()), static_cast<int>(frame.height()), 32), FreeImage_Unload);
				std::memcpy(FreeImage_GetBits(bitmap.get()), frame.image_data().begin(), frame.image_data().size());

				image_view<bgra_pixel> original_view(FreeImage_GetBits(bitmap.get()), static_cast<int>(frame.width()), static_cast<int>(frame.height()));
				unmultiply(original_view);

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

spl::shared_ptr<core::frame_consumer> create_consumer(
		const std::vector<std::wstring>& params, core::interaction_sink*, std::vector<spl::shared_ptr<core::video_channel>> channels)
{
	if (params.size() < 1 || !boost::iequals(params.at(0), L"IMAGE"))
		return core::frame_consumer::empty();

	std::wstring filename;

	if (params.size() > 1)
		filename = params.at(1);

	return spl::make_shared<image_consumer>(filename);
}

}}
