#include "../../StdAfx.h"

#include "frame.h"
#include "../algorithm.h"
#include "../../../common/image/copy.h"

namespace caspar { namespace gpu {
	
gpu_frame::gpu_frame(size_t width, size_t height, void* tag) : buffer_(width, height), texture_(width, height), data_(nullptr), tag_(tag)
{	
}

void gpu_frame::lock()
{
	if(data_ == nullptr)
		return;
	data_ = nullptr;
	buffer_.unmap();
	buffer_.write_to_texture(texture_);
}

void gpu_frame::unlock()
{
	if(data_ != nullptr)
		return;
	data_ = reinterpret_cast<unsigned char*>(buffer_.map());
	if(!data_)
		BOOST_THROW_EXCEPTION(std::bad_alloc());
}
	
void gpu_frame::draw()
{
	texture_.draw();
}

void* gpu_frame::tag() const
{
	return tag_;
}
	
unsigned char* gpu_frame::data()
{
	if(data_ == nullptr)
		BOOST_THROW_EXCEPTION(invalid_operation());
	return data_;
}
size_t gpu_frame::size() const { return buffer_.size(); }
size_t gpu_frame::width() const { return buffer_.width();}
size_t gpu_frame::height() const { return buffer_.height();}

}}