#include <vector>
#include <stdint.h>
#include "../util/image_algorithms.h"

namespace caspar { namespace image {

std::vector<std::pair<int, int>> get_line_points(int num_pixels, double angle_radians)
{
	std::vector<std::pair<int, int>> line_points;
	line_points.reserve(num_pixels);

	double delta_x = std::cos(angle_radians);
	double delta_y = -std::sin(angle_radians); // In memory is revered
	double max_delta = std::max(std::abs(delta_x), std::abs(delta_y));
	double amplification = 1.0 / max_delta;
	delta_x *= amplification;
	delta_y *= amplification;

	for (int i = 1; i <= num_pixels; ++i)
		line_points.push_back(std::make_pair(
			static_cast<int>(std::floor(delta_x * static_cast<double>(i) + 0.5)), 
			static_cast<int>(std::floor(delta_y * static_cast<double>(i) + 0.5))));

	return std::move(line_points);
}

}}	//namespace caspar::image