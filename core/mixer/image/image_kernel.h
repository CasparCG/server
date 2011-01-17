#pragma once

#include "../../video_format.h"
#include "../frame/pixel_format.h"

#include <memory>

namespace caspar { namespace core {

struct image_transform;

class image_kernel
{
public:
	image_kernel();
	void apply(pixel_format::type pix_fmt, const image_transform& mode);

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}