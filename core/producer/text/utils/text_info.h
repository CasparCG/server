#pragma once

#include <string>
#include "color.h"

namespace caspar { namespace core { namespace text {

	struct text_info
	{
		std::wstring font;
		std::wstring font_file;

		float size						= 0.0f;
		color<float> color;
		//int shadow_distance;
		//int shadow_size;
		//float shadow_spread;
		//color<float> shadow_color;
		int baseline_shift				= 0;
		int tracking					= 0;
	};

}}}