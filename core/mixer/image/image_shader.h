#pragma once

#include <common/memory/safe_ptr.h>

namespace caspar { namespace core {

class shader;
class ogl_device;

struct texture_id
{
	enum type
	{
		plane0 = 0,
		plane1,
		plane2,
		plane3,
		local_key,
		layer_key,
		background,
	};
};

safe_ptr<shader> get_image_shader(ogl_device& ogl, bool& blend_modes);


}}