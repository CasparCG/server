#pragma once

namespace caspar { namespace core { namespace text {

	struct string_metrics
	{
		string_metrics()
				: width(0), bearingY(0), protrudeUnderY(0), height(0) {}
		int width;
		int bearingY;
		int protrudeUnderY;
		int height;
	};
}}}