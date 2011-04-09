#pragma once

#include <memory>

#include <core/producer/frame/pixel_format.h>
#include <core/producer/frame/image_transform.h>

namespace caspar { namespace mixer {
	
class image_kernel
{
public:
	image_kernel();
	void apply(const core::pixel_format_desc& pix_desc, const core::image_transform& mode);

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}