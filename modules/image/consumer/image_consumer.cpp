/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
#include "image_consumer.h"

#include <common/exception/exceptions.h>
#include <common/env.h>
#include <common/log/log.h>
#include <common/utility/string.h>

#include <core/consumer/frame_consumer.h>
#include <core/video_format.h>
#include <core/mixer/read_frame.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>

#include <tbb/concurrent_queue.h>

#include <FreeImage.h>

namespace caspar {
	
struct image_consumer : public core::frame_consumer
{
	core::video_format_desc	format_desc_;
	size_t counter_;
public:
	image_consumer() : counter_(0)
	{
	}

	virtual void initialize(const core::video_format_desc& format_desc)
	{
		format_desc_ = format_desc;
	}
	
	virtual bool send(const safe_ptr<core::read_frame>& frame)
	{				
		if(counter_ < core::consumer_buffer_depth())
			++counter_;
		else if(counter_ == core::consumer_buffer_depth())
		{
			boost::thread async([=]
			{
				try
				{
					auto filename = narrow(env::data_folder()) +  boost::posix_time::to_iso_string(boost::posix_time::second_clock::local_time()) + ".png";

					auto bitmap = std::shared_ptr<FIBITMAP>(FreeImage_Allocate(format_desc_.width, format_desc_.height, 32), FreeImage_Unload);
					memcpy(FreeImage_GetBits(bitmap.get()), frame->image_data().begin(), frame->image_size());
					FreeImage_FlipVertical(bitmap.get());
					FreeImage_Save(FIF_PNG, bitmap.get(), filename.c_str(), 0);
				}
				catch(...)
				{
					CASPAR_LOG_CURRENT_EXCEPTION();
				}
			});
			async.detach();

			return false;
		}

		return true;
	}

	virtual size_t buffer_depth() const{return 3;}

	virtual std::wstring print() const
	{
		return L"image[]";
	}

	virtual const core::video_format_desc& get_video_format_desc() const
	{
		return format_desc_;
	}
};

safe_ptr<core::frame_consumer> create_image_consumer(const std::vector<std::wstring>& params)
{
	if(params.size() < 1 || params[0] != L"IMAGE")
		return core::frame_consumer::empty();

	return make_safe<image_consumer>();
}


}
