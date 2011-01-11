#pragma once

#include <Windows.h>

#include <BlueVelvet4.h>
#include "../../format/video_format.h"

#include <common/memory/page_locked_allocator.h>

#include <tbb/mutex.h>
#include <tbb/recursive_mutex.h>

#include <unordered_map>

namespace caspar { namespace core { namespace bluefish {
	
static const size_t MAX_HANC_BUFFER_SIZE = 256*1024;
static const size_t MAX_VBI_BUFFER_SIZE = 36*1920*4;

struct blue_dma_buffer
{
public:
	blue_dma_buffer(int image_size, int id) 
		: id_(id)
		, image_size_(image_size)
		, hanc_size_(256*1024)
		, image_buffer_(image_size_)
		, hanc_buffer_(hanc_size_){}
			
	int id() const {return id_;}

	PBYTE image_data() { return image_buffer_.data(); }
	PBYTE hanc_data() { return hanc_buffer_.data(); }

	size_t image_size() const { return image_size_; }
	size_t hanc_size() const { return hanc_size_; }

private:	
	int id_;
	size_t image_size_;
	size_t hanc_size_;
	std::vector<BYTE, page_locked_allocator<BYTE>> image_buffer_;	
	std::vector<BYTE, page_locked_allocator<BYTE>> hanc_buffer_;
};
typedef std::shared_ptr<blue_dma_buffer> blue_dma_buffer_ptr;

}}}