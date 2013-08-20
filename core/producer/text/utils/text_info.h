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
		int baseline_shift;
		int tracking;
	};

}}}