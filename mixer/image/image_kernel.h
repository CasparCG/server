#pragma once

#include <memory>

namespace caspar { namespace core {

struct pixel_format_desc;
class image_transform;
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