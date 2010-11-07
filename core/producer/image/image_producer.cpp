#include "../../StdAfx.h"

#include "image_producer.h"
#include "image_loader.h"

#include "../../frame/frame_factory.h"
#include "../../frame/frame_format.h"
#include "../../server.h"
#include "../../../common/utility/find_file.h"
#include "../../../common/utility/memory.h"

#include <boost/assign.hpp>

using namespace boost::assign;

namespace caspar { namespace core { namespace image{

struct image_producer : public frame_producer
{
	image_producer(const std::wstring& filename, const frame_format_desc& format_desc) : format_desc_(format_desc), filename_(filename)	{}

	~image_producer()
	{
		if(factory_)
			factory_->release_frames(this);
	}

	gpu_frame_ptr render_frame(){return frame_;}

	void initialize(const frame_factory_ptr& factory)
	{
		factory_ = factory;
		auto bitmap = load_image(filename_);
		if(FreeImage_GetWidth(bitmap.get()) != format_desc_.width || FreeImage_GetHeight(bitmap.get()) == format_desc_.height)
		{
			bitmap = std::shared_ptr<FIBITMAP>(FreeImage_Rescale(bitmap.get(), format_desc_.width, format_desc_.width, FILTER_BICUBIC), FreeImage_Unload);
			if(!bitmap)
				BOOST_THROW_EXCEPTION(invalid_argument() << msg_info("Unsupported image format."));
		}

		FreeImage_FlipVertical(bitmap.get());
		frame_ = factory->create_frame(format_desc_, this);
		common::aligned_parallel_memcpy(frame_->data(), FreeImage_GetBits(bitmap.get()), format_desc_.size);
	}

	const frame_format_desc& get_frame_format_desc() const { return format_desc_; } 

	frame_factory_ptr factory_;
	std::wstring filename_;
	frame_format_desc format_desc_;
	gpu_frame_ptr frame_;
};

frame_producer_ptr create_image_producer(const  std::vector<std::wstring>& params, const frame_format_desc& format_desc)
{
	std::wstring filename = params[0];
	std::wstring resultFilename = common::find_file(server::media_folder() + filename, list_of(L"png")(L"tga")(L"bmp")(L"jpg")(L"jpeg"));
	if(!resultFilename.empty())
		return std::make_shared<image_producer>(resultFilename, format_desc);

	return nullptr;
}

}}}