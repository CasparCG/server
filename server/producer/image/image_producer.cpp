#include "../../StdAfx.h"

#include "image_producer.h"
#include "image_loader.h"

#include "../../frame/system_frame.h"
#include "../../frame/format.h"
#include "../../server.h"
#include "../../../common/utility/find_file.h"
#include "../../../common/image/image.h"

#include <boost/assign.hpp>

using namespace boost::assign;

namespace caspar{ namespace image{

struct image_producer : public frame_producer
{
	image_producer(const std::wstring& filename, const frame_format_desc& format_desc) : format_desc_(format_desc)
	{
		auto bitmap = load_image(filename);

		if(FreeImage_GetWidth(bitmap.get()) != format_desc.width || FreeImage_GetHeight(bitmap.get()) == format_desc.height)
		{
			bitmap = std::shared_ptr<FIBITMAP>(FreeImage_Rescale(bitmap.get(), format_desc.width, format_desc.width, FILTER_BICUBIC), FreeImage_Unload);
			if(!bitmap)
				BOOST_THROW_EXCEPTION(invalid_argument() << msg_info("Unsupported image format."));
		}

		FreeImage_FlipVertical(bitmap.get());

		frame_ = std::make_shared<system_frame>(format_desc.size);
		common::image::copy(frame_->data(), FreeImage_GetBits(bitmap.get()), frame_->size());
	}

	frame_ptr get_frame()
	{
		return frame_;
	}

	const frame_format_desc& get_frame_format_desc() const { return format_desc_; } 

	frame_format_desc format_desc_;
	frame_ptr frame_;
};

frame_producer_ptr create_image_producer(const  std::vector<std::wstring>& params, const frame_format_desc& format_desc)
{
	std::wstring filename = params[0];
	std::wstring resultFilename = common::find_file(server::media_folder() + filename, list_of(L"png")(L"tga")(L"bmp")(L"jpg")(L"jpeg"));
	if(!resultFilename.empty())
		return std::make_shared<image_producer>(resultFilename, format_desc);

	return nullptr;
}

}}