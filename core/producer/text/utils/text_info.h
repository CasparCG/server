#pragma once

#include <string>
#include "color.h"

namespace caspar { namespace core { namespace text {

	struct text_info
	{
		std::wstring font;
		std::wstring font_file;

		float size;
		color<float> color;
		//int shadow_distance;
		//int shadow_size;
		//float shadow_spread;
		//color<float> shadow_color;
		int baseline_shift;
		int tracking;

		text_info()
			: size(0.0f)
			, baseline_shift(0)
			, tracking(0)
		{
		}
	};

}}}