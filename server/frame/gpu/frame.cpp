#include "../../StdAfx.h"

#include "frame.h"

namespace caspar { namespace gpu {
	
gpu_frame::gpu_frame(size_t width, size_t height) : buffer_(width, height), data_(nullptr)
{
	map();
}

void gpu_frame::unmap()
{
	if(data_ == nullptr)
		return;
	data_ = nullptr;
	buffer_.unmap();
}

void gpu_frame::map()
{
	if(data_ != nullptr)
		return;
	data_ = reinterpret_cast<unsigned char*>(buffer_.map());
	if(!data_)
		BOOST_THROW_EXCEPTION(std::bad_alloc());
}

void gpu_frame::reset()
{
	map();
	audio_data().clear();
}
	
void gpu_frame::write_to_texture(common::gpu::texture& texture)
{
	assert(data_ == nullptr); // is unmapped
	buffer_.write_to_texture(texture);
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