#pragma once

#include <core/video_format.h>
#include <core/producer/frame/pixel_format.h>

#include <memory>

namespace caspar { namespace core {

class image_transform;

class image_kernel
{
public:
	image_kernel();
	void apply(const pixel_format_desc& pix_desc, const image_transform& mode);

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}