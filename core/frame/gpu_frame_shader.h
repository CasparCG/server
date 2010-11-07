#pragma once

#include "frame_format.h"
#include "gpu_frame_desc.h"

#include <memory>
#include <array>

namespace caspar { namespace core {
	
class gpu_frame_shader
{
public:
	gpu_frame_shader(const frame_format_desc& format_desc);
	void use(gpu_frame_desc& image);

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<gpu_frame_shader> gpu_frame_shader_ptr;

}}