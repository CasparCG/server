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

#include <common/exception/exceptions.h>
#include <common/env.h>
#include <common/log/log.h>
#include <common/utility/string.h>
#include <common/concurrency/future_util.h>

#include <core/consumer/frame_consumer.h>
#include <core/video_format.h>
#include <core/mixer/read_frame.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>

#include <tbb/concurrent_queue.h>

#include <FreeImage.h>
#include <vector>

namespace caspar { namespace image {
	
struct image_consumer : public core::frame_consumer
{
	core::video_format_desc	format_desc_;
	std::wstring			filename_;
public:

	// frame_consumer

	image_consumer(const std::wstring& filename)
		: filename_(filename)
	{
	}

	virtual void initialize(const core::video_format_desc& format_desc, int) override
	{
		format_desc_ = format_desc;
	}
	
	virtual boost::unique_future<bool> send(const safe_ptr<core::read_frame>& frame) override
	{				
		auto format_desc = format_desc_;
		auto filename = filename_;

		boost::thread async([format_desc, frame, filename]
		{
			try
			{
				std::string filename2 = narrow(filename);

				if (filename2.empty())
					filename2 = narrow(env::media_folder()) +  boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time()) + ".png";
				else
					filename2 = narrow(env::media_folder()) + filename2 + ".png";

				auto bitmap = std::shared_ptr<FIBITMAP>(FreeImage_Allocate(format_desc.width, format_desc.height, 32), FreeImage_Unload);
				memcpy(FreeImage_GetBits(bitmap.get()), frame->image_data().begin(), frame->image_size());
				FreeImage_FlipVertical(bitmap.get());
				FreeImage_Save(FIF_PNG, bitmap.get(), filename2.c_str(), 0);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
			}
		});
		async.detach();

		return wrap_as_future(false);
	}

	virtual std::wstring print() const override
	{
		return L"image[]";
	}

	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"image-consumer");
		return info;
	}

	virtual size_t buffer_depth() const override
	{
		return 0;
	}

	virtual int index() const override
	{
		return 100;
	}
};

safe_ptr<core::frame_consumer> create_consumer(const std::vector<std::wstring>& params)
{
	if(params.size() < 1 || params[0] != L"IMAGE")
		return core::frame_consumer::empty();

	std::wstring filename;

	if (params.size() > 1)
		filename = params[1];

	return make_safe<image_consumer>(filename);
}

}}
