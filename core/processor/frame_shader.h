#pragma once

#include "../video/video_format.h"
#include "../video/pixel_format.h"

#include <memory>
#include <array>

namespace caspar { namespace core {
	
class frame_shader
{
public:
	frame_shader(const video_format_desc& format_desc);
	void use(const pixel_format_desc& image);

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<frame_shader> frame_shader_ptr;

}}