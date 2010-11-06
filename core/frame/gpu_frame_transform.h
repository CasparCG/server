#pragma once

#include <memory>

enum pixel_format
{
    bgra = 1,
    rgba,
    argb,
    abgr,
	yuv,
	yuva,
	invalid_pixel_format,
};

namespace caspar { namespace core {
	
class gpu_frame_transform
{
public:
	gpu_frame_transform();
	void set_pixel_format(pixel_format format);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<gpu_frame_transform> gpu_frame_transform_ptr;

}}