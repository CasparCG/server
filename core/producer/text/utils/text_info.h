#pragma once

#include <string>
#include "color.h"

namespace caspar { namespace core { namespace text {

	struct text_info
	{
		std::wstring font;
		std::wstring font_file;

		double					size				= 0.0;
		text::color<double>		color;
		//int					shadow_distance;
		//int					shadow_size;
		//float					shadow_spread;
		//text::color<float>	shadow_color;
		int						baseline_shift		= 0;
		int						tracking			= 0;
		double					scale_x				= 1.0;
		double					scale_y				= 1.0;
		double					shear				= 0;
	};

}}}
