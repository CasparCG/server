#include "../../StdAfx.h"

#include "image_producer.h"
#include "image_loader.h"

#include "../../processor/frame_processor_device.h"
#include "../../format/video_format.h"
#include "../../server.h"
#include "../../../common/utility/find_file.h"
#include "../../../common/utility/memory.h"

#include <boost/assign.hpp>

using namespace boost::assign;

namespace caspar { namespace core { namespace image{

struct image_producer : public frame_producer
{
	image_producer(const std::wstring& filename) : filename_(filename)	{}

	~image_producer()
	{
		if(frame_processor_)
			frame_processor_->release_tag(this);
	}

	frame_ptr render_frame(){return frame_;}

	void initialize(const frame_processor_device_ptr& frame_processor)
	{
		frame_processor_ = frame_processor;
		auto bitmap = load_image(filename_);
		FreeImage_FlipVertical(bitmap.get());
		frame_ = frame_processor->create_frame(FreeImage_GetWidth(bitmap.get()), FreeImage_GetHeight(bitmap.get()), this);
		common::aligned_parallel_memcpy(frame_->data(), FreeImage_GetBits(bitmap.get()), frame_->size());
	}
	
	frame_processor_device_ptr frame_processor_;
	std::wstring filename_;
	frame_ptr frame_;
};

frame_producer_ptr create_image_producer(const  std::vector<std::wstring>& params)
{
	std::wstring filename = params[0];
	std::wstring resultFilename = common::find_file(server::media_folder() + filename, list_of(L"png")(L"tga")(L"bmp")(L"jpg")(L"jpeg"));
	if(!resultFilename.empty())
		return std::make_shared<image_producer>(resultFilename);

	return nullptr;
}

}}}