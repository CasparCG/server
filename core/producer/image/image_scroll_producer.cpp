#include "../../StdAfx.h"

#include "image_scroll_producer.h"

#include "image_loader.h"

#include "../../frame/frame_format.h"
#include "../../frame/frame_factory.h"
#include "../../server.h"
#include "../../../common/utility/find_file.h"
#include "../../../common/image/image.h"

#include <tbb/parallel_for.h>
#include <tbb/parallel_invoke.h>
#include <tbb/scalable_allocator.h>

#include <boost/assign.hpp>
#include <boost/algorithm/string/case_conv.hpp>

using namespace boost::assign;

namespace caspar{ namespace image{

enum direction
{
	Up, Down, Left, Right
};

struct image_scroll_producer : public frame_producer
{
	static const int DEFAULT_SCROLL_SPEED = 50;

	image_scroll_producer(const std::wstring& filename, const std::vector<std::wstring>& params, const frame_format_desc& format_desc) 
		: format_desc_(format_desc), speed_(image_scroll_producer::DEFAULT_SCROLL_SPEED), direction_(direction::Up), offset_(0)
	{
		load_and_pad_image(filename);

		auto pos = filename.find_last_of(L'_');
		if(pos != std::wstring::npos && pos + 1 < filename.size())
		{
			std::wstring speedStr = filename.substr(pos + 1);
			pos = speedStr.find_first_of(L'.');
			if(pos != std::wstring::npos)
			{
				speedStr = speedStr.substr(0, pos);		
				speed_ = common::lexical_cast_or_default<int>(speedStr, image_scroll_producer::DEFAULT_SCROLL_SPEED);
			}
		}

		loop_ = std::find(params.begin(), params.end(), L"LOOP") != params.end();
				
		if(image_width_ - format_desc.width > image_height_ - format_desc_.height)
			direction_ = speed_ < 0 ? direction::Right : direction::Left;
		else
			direction_ = speed_ < 0 ? direction::Down : direction::Up;

		if (direction_ == direction::Down)
			offset_ = image_height_ - format_desc_.height;
		else if (direction_ == direction::Right)
			offset_ = image_width_ - format_desc_.width;

		speed_ = static_cast<int>(abs(static_cast<double>(speed_) / format_desc.fps));
	}

	void load_and_pad_image(const std::wstring& filename)
	{
		auto pBitmap = load_image(filename);

		size_t width = FreeImage_GetWidth(pBitmap.get());
		size_t height = FreeImage_GetHeight(pBitmap.get());

		image_width_ = std::max(width, format_desc_.width);
		image_height_ = std::max(height, format_desc_.height);

		image_ = std::shared_ptr<unsigned char>(static_cast<unsigned char*>(scalable_aligned_malloc(image_width_*image_height_*4, 16)));
		common::image::clear(image_.get(), image_width_*image_height_*4);

		unsigned char* pBits = FreeImage_GetBits(pBitmap.get());
		
		for (size_t i = 0; i < height; ++i)
			common::image::copy(&image_.get()[i * image_width_ * 4], &pBits[i* width * 4], width * 4);
	}

	gpu_frame_ptr render_frame()
	{
		gpu_frame_ptr frame = factory_->create_frame(format_desc_);
		common::image::clear(frame->data(), frame->size());

		const int delta_x = direction_ == direction::Left ? speed_ : -speed_;
		const int delta_y = direction_ == direction::Up ? speed_ : -speed_;

		unsigned char* frame_data = frame->data();
		unsigned char* image_data = image_.get();
	
		if (direction_ == direction::Up || direction_ == direction::Down)
		{
			tbb::parallel_for(static_cast<size_t>(0), format_desc_.height, static_cast<size_t>(1), [&](size_t i)
			{
				int srcRow = i + offset_;
				int dstInxex = i * format_desc_.width * 4;
				int srcIndex = srcRow * format_desc_.width * 4;
				int size = format_desc_.width * 4;

				memcpy(&frame_data[dstInxex], &image_data[srcIndex], size);	
			});				
			
			offset_ += delta_y;
		}
		else
		{
			tbb::parallel_for(static_cast<size_t>(0), format_desc_.height, static_cast<size_t>(1), [&](size_t i)
			{
				int correctOffset = offset_;
				int dstIndex = i * format_desc_.width * 4;
				int srcIndex = (i * image_width_ + correctOffset) * 4;
			
				int stopOffset = std::min<int>(correctOffset + format_desc_ .width, image_width_);
				int size = (stopOffset - correctOffset) * 4;

				memcpy(&frame_data[dstIndex], &image_data[srcIndex], size);
			});

			offset_ += delta_x;
		}

		return frame;
	}

	gpu_frame_ptr render_interlaced_frame()
	{
		gpu_frame_ptr next_frame1;
		gpu_frame_ptr next_frame2;
		tbb::parallel_invoke([&]{ next_frame1 = render_frame(); }, [&]{ next_frame2 = render_frame(); });
		
		common::image::copy_field(next_frame1->data(), next_frame2->data(), format_desc_.mode == video_mode::upper ? 1 : 0, format_desc_.width, format_desc_.height);
		return next_frame1;
	}
	
	gpu_frame_ptr get_frame()
	{
		return format_desc_.mode == video_mode::progressive ? render_frame() : render_interlaced_frame();
	}

	
	void initialize(const frame_factory_ptr& factory)
	{
		factory_ = factory;
	}

	const frame_format_desc& get_frame_format_desc() const { return format_desc_; } 
	
	int image_width_;
	int image_height_;
	int speed_;
	int offset_;
	direction direction_;

	tbb::atomic<bool> loop_;
	std::shared_ptr<unsigned char> image_;
	frame_format_desc format_desc_;

	frame_factory_ptr factory_;
};

frame_producer_ptr create_image_scroll_producer(const std::vector<std::wstring>& params, const frame_format_desc& format_desc)
{
	std::wstring filename = params[0];
	auto result_filename = common::find_file(server::media_folder() + filename, list_of(L"spng")(L"stga")(L"sbmp")(L"sjpg")(L"sjpeg"));
	if(!result_filename.empty())
		return std::make_shared<image_scroll_producer>(result_filename, params, format_desc);

	return nullptr;
}

}}