/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Helge Norberg, helge.norberg@svt.se
*/

#pragma once

#include <common/tweener.h>

#include <cmath>
#include <boost/foreach.hpp>

namespace caspar { namespace image {

/**
 * Helper for calculating the color of a pixel given any number of of other
 * pixels (each with their own weight).
 */
class rgba_weighting
{
	int r, g, b, a;
	int total_weight;
public:
	rgba_weighting()
		: r(0), g(0), b(0), a(0), total_weight(0)
	{
	}

	template<class RGBAPixel>
	inline void add_pixel(const RGBAPixel& pixel, uint8_t weight)
	{
		r += pixel.r() * weight;
		g += pixel.g() * weight;
		b += pixel.b() * weight;
		a += pixel.a() * weight;

		total_weight += weight;
	}

	template<class RGBAPixel>
	inline void store_result(RGBAPixel& pixel)
	{
		pixel.r() = static_cast<uint8_t>(r / total_weight);
		pixel.g() = static_cast<uint8_t>(g / total_weight);
		pixel.b() = static_cast<uint8_t>(b / total_weight);
		pixel.a() = static_cast<uint8_t>(a / total_weight);
	}
};

template<class T>
std::vector<T> get_tweened_values(const core::tweener& tweener, size_t num_values, T from, T to)
{
	std::vector<T> result;
	result.reserve(num_values);

	double start = static_cast<double>(from);
	double delta = static_cast<double>(to - from);
	double duration = static_cast<double>(num_values);

	for (double t = 0; t < duration; ++t)
	{
		result.push_back(static_cast<T>(tweener(t, start, delta, duration - 1.0)));
	}

	return std::move(result);
}

/**
 * Blur a source image and store the blurred result in a destination image.
 * <p>
 * The blur is done by weighting each relative pixel from a destination pixel
 * position using a vector of relative x-y pairs. The further away a related
 * pixel is the less weight it gets. A tweener is used to calculate the actual
 * weights of each related pixel.
 *
 * @param src                      The source view. Has to model the ImageView
 *                                 concept and have a pixel type modelling the
 *                                 RGBAPixel concept.
 * @param dst                      The destination view. Has to model the
 *                                 ImageView concept and have a pixel type
 *                                 modelling the RGBAPixel concept.
 * @param motion_trail_coordinates The relative x-y positions to weight in for
 *                                 each pixel.
 * @param tweener                  The tweener to use for calculating the
 *                                 weights of each relative position in the
 *                                 motion trail.
 */
template<class SrcView, class DstView>
void blur(
	const SrcView& src,
	DstView& dst,
	const std::vector<std::pair<int, int>> motion_trail_coordinates, 
	const core::tweener& tweener)
{
	auto blur_px = motion_trail_coordinates.size();
	auto tweened_weights_y = get_tweened_values<uint8_t>(tweener, blur_px + 2, 255, 0);
	tweened_weights_y.pop_back();
	tweened_weights_y.erase(tweened_weights_y.begin());

	auto src_end = src.end();
	auto dst_iter = dst.begin();

	for (auto src_iter = src.begin(); src_iter != src_end; ++src_iter, ++dst_iter)
	{
		rgba_weighting w;

		for (int i = 0; i < blur_px; ++i)
		{
			auto& coordinate = motion_trail_coordinates[i];
			auto other_pixel = src.relative(src_iter, coordinate.first, coordinate.second);

			if (other_pixel == nullptr)
				break;

			w.add_pixel(*other_pixel, tweened_weights_y[i]);
		}

		w.add_pixel(*src_iter, 255);
		w.store_result(*dst_iter);
	}
}

/**
 * Calculate relative x-y coordinates of a straight line with a given angle and
 * a given number of points.
 *
 * @param num_pixels    The number of pixels/points to create.
 * @param angle_radians The angle of the line in radians.
 *
 * @return the x-y pairs.
 */
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

/**
 * Directionally blur a source image modelling the ImageView concept and store
 * the blurred image to a destination image also modelling the ImageView
 * concept.
 * <p>
 * The pixel type of the views must model the RGBAPixel concept.
 *
 * @param src           The source image view. Has to model the ImageView
 *                      concept and have a pixel type that models RGBAPixel.
 * @param dst           The destiation image view. Has to model the ImageView
 *                      concept and have a pixel type that models RGBAPixel.
 * @param angle_radians The angle in radians to directionally blur the image.
 * @param blur_px       The number of pixels of the blur.
 * @param tweener       The tweener to use to create a pixel weighting curve
 *                      with.
 */
template<class SrcView, class DstView>
void blur(
	const SrcView& src,
	DstView& dst,
	double angle_radians,
	int blur_px, 
	const core::tweener& tweener)
{
	auto motion_trail = get_line_points(blur_px, angle_radians);

	blur(src, dst, motion_trail, tweener);
}

/**
 * Premultiply with alpha for each pixel in an ImageView. The modifications is
 * done in place. The pixel type of the ImageView must model the RGBAPixel
 * concept.
 *
 * @param view_to_modify The image view to premultiply in place. Has to model
 *                       the ImageView concept and have a pixel type that
 *                       models RGBAPixel.
 */
template<class SrcDstView>
void premultiply(SrcDstView& view_to_modify)
{
	std::for_each(view_to_modify.begin(), view_to_modify.end(), [&](SrcDstView::pixel_type& pixel)
	{
		int alpha = static_cast<int>(pixel.a());

		if (alpha != 255) // Performance optimization
		{
			// We don't event try to premultiply 0 since it will be unaffected.
			if (pixel.r())
				pixel.r() = static_cast<uint8_t>(static_cast<int>(pixel.r()) * alpha / 255);

			if (pixel.g())
				pixel.g() = static_cast<uint8_t>(static_cast<int>(pixel.g()) * alpha / 255);

			if (pixel.b())
				pixel.b() = static_cast<uint8_t>(static_cast<int>(pixel.b()) * alpha / 255);
		}
	});
}

}}
