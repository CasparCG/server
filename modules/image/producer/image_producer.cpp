#include "image_producer.h"

#include "../util/image_loader.h"

#include <core/video_format.h>

#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/write_frame.h>

#include <common/env.h>

#include <boost/assign.hpp>
#include <boost/filesystem.hpp>

#include <algorithm>

using namespace boost::assign;

namespace caspar {

struct image_producer : public core::frame_producer
{	
	std::wstring filename_;
	safe_ptr<core::basic_frame> frame_;
	
	explicit image_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filename) 
		: filename_(filename)
		, frame_(core::basic_frame::empty())	
	{
		auto bitmap = load_image(filename_);
		FreeImage_FlipVertical(bitmap.get());
		auto frame = frame_factory->create_frame(FreeImage_GetWidth(bitmap.get()), FreeImage_GetHeight(bitmap.get()));
		std::copy_n(FreeImage_GetBits(bitmap.get()), frame->image_data().size(), frame->image_data().begin());
		frame_ = std::move(frame);
	}
	
	virtual safe_ptr<core::basic_frame> receive(){return frame_;}

	
	virtual std::wstring print() const
	{
		return L"image_producer[" + filename_ + L"]";
	}
};

safe_ptr<core::frame_producer> create_image_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params)
{
	static const std::vector<std::wstring> extensions = list_of(L"png")(L"tga")(L"bmp")(L"jpg")(L"jpeg");
	std::wstring filename = env::media_folder() + L"\\" + params[0];
	
	auto ext = std::find_if(extensions.begin(), extensions.end(), [&](const std::wstring& ex) -> bool
		{					
			return boost::filesystem::is_regular_file(boost::filesystem::wpath(filename).replace_extension(ex));
		});

	if(ext == extensions.end())
		return core::frame_producer::empty();

	return make_safe<image_producer>(frame_factory, filename + L"." + *ext);
}


}