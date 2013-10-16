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

#include "stdafx.h"

#include <gtest/gtest.h>

#include <core/mixer/image/image_mixer.h>
#include <core/frame/pixel_format.h>
#include <core/frame/frame_transform.h>
#include <core/frame/draw_frame.h>

#include <accelerator/ogl/image/image_mixer.h>
#include <accelerator/ogl/util/device.h>

#include <accelerator/cpu/image/image_mixer.h>

namespace caspar { namespace core {

spl::shared_ptr<accelerator::ogl::device> ogl_device()
{
	static auto device = spl::make_shared<accelerator::ogl::device>();

	return device;
}

template <typename T>
spl::shared_ptr<core::image_mixer> create_mixer();

template <>
spl::shared_ptr<core::image_mixer> create_mixer<accelerator::ogl::image_mixer>()
{
	return spl::make_shared<accelerator::ogl::image_mixer>(
			ogl_device(),
			false); // blend modes not wanted
}

struct dummy_ogl_with_blend_modes {};
template <>
spl::shared_ptr<core::image_mixer> create_mixer<dummy_ogl_with_blend_modes>()
{
	return spl::make_shared<accelerator::ogl::image_mixer>(
			ogl_device(),
			true); // blend modes wanted
}

template <>
spl::shared_ptr<core::image_mixer> create_mixer<accelerator::cpu::image_mixer>()
{
	return spl::make_shared<accelerator::cpu::image_mixer>();
}

void assert_pixel_eq(uint8_t r, uint8_t g, uint8_t b, uint8_t a, const uint8_t* pos, int tolerance = 1)
{
	ASSERT_NEAR(b, *pos, tolerance);
	ASSERT_NEAR(g, *(pos + 1), tolerance);
	ASSERT_NEAR(r, *(pos + 2), tolerance);
	ASSERT_NEAR(a, *(pos + 3), tolerance);
}

template<typename Range>
void assert_all_pixels_eq(uint8_t r, uint8_t g, uint8_t b, uint8_t a, const Range& range, int tolerance = 1)
{
	for (auto iter = range.begin(); iter != range.end(); iter += 4)
		assert_pixel_eq(r, g, b, a, iter, tolerance);
}

void set_pixel(core::mutable_frame& frame, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	auto pos = frame.image_data().data() + (y * frame.pixel_format_desc().planes[0].width + x) * 4;

	*pos = b;
	*++pos = g;
	*++pos = r;
	*++pos = a;
}

template <typename T>
class MixerTest : public ::testing::Test
{
protected:
	spl::shared_ptr<core::image_mixer> mixer;
public:
	MixerTest() : mixer(create_mixer<T>()) { }

	core::mutable_frame create_frame(int width, int height)
	{
		core::pixel_format_desc desc(core::pixel_format::bgra);
		desc.planes.push_back(core::pixel_format_desc::plane(width, height, 4));
		return mixer->create_frame(this, desc);
	}

	core::mutable_frame create_single_color_frame(
			uint8_t r, uint8_t g, uint8_t b, uint8_t a, int width, int height)
	{
		auto frame = create_frame(width, height);

		for (int y = 0; y < height; ++y)
			for (int x = 0; x < width; ++x)
				set_pixel(frame, x, y, r, g, b, a);

		return frame;
	}

	void add_layer(core::draw_frame frame, core::blend_mode blend_mode = core::blend_mode::normal)
	{
		mixer->begin_layer(core::blend_mode::normal);

		frame.accept(*mixer);

		mixer->end_layer();
	}

	array<const uint8_t> get_result(int width, int height)
	{
		core::video_format_desc desc(core::video_format::x576p2500);
		desc.width = width;
		desc.height = height;
		desc.size = width * height * 4;

		return (*mixer)(desc).get();
	}
};

template <typename T>
class MixerTestEveryImpl : public MixerTest<T>
{
};

template <typename T>
class MixerTestOgl : public MixerTest<T> // Only use-cases supported by ogl mixer
{
};

template <typename T>
class MixerTestOglWithBlendModes : public MixerTest<T> // Only use-cases supported by ogl mixer
{
};

typedef testing::Types<
		accelerator::ogl::image_mixer,
		accelerator::cpu::image_mixer,
		dummy_ogl_with_blend_modes> all_mixer_implementations;
typedef testing::Types<
		accelerator::ogl::image_mixer,
		dummy_ogl_with_blend_modes> gpu_mixer_implementations;

TYPED_TEST_CASE(MixerTestEveryImpl, all_mixer_implementations);
TYPED_TEST_CASE(MixerTestOgl, gpu_mixer_implementations);
TYPED_TEST_CASE(MixerTestOglWithBlendModes, testing::Types<dummy_ogl_with_blend_modes>);

// Tests for use cases that should work on both CPU and GPU mixer
// --------------------------------------------------------------

TYPED_TEST(MixerTestEveryImpl, PassthroughSingleLayer)
{
	core::draw_frame frame(create_single_color_frame(255, 255, 255, 255, 16, 16));
	add_layer(frame);
	assert_all_pixels_eq(255, 255, 255, 255, get_result(16, 16));
}

TYPED_TEST(MixerTestEveryImpl, OpaqueTopLayer)
{
	core::draw_frame red_under(create_single_color_frame(255, 0, 0, 255, 16, 16));
	core::draw_frame green_over(create_single_color_frame(0, 255, 0, 255, 16, 16));
	add_layer(red_under);
	add_layer(green_over);
	assert_all_pixels_eq(0, 255, 0, 255, get_result(16, 16));
}

TYPED_TEST(MixerTestEveryImpl, HalfAlpha)
{
	core::draw_frame red_under(create_single_color_frame(255, 0, 0, 255, 16, 16));
	core::draw_frame half_green_over(create_single_color_frame(0, 127, 0, 127, 16, 16));
	add_layer(red_under);
	add_layer(half_green_over);
	assert_all_pixels_eq(127, 127, 0, 255, get_result(16, 16));

	add_layer(half_green_over);
	assert_all_pixels_eq(0, 127, 0, 127, get_result(16, 16));
}

// Tests for use cases that currently *only* works on GPU mixer
// ------------------------------------------------------------

TYPED_TEST(MixerTestOgl, HalfBrightness)
{
	core::draw_frame frame(create_single_color_frame(255, 255, 255, 255, 1, 1));
	frame.transform().image_transform.brightness = 0.5;
	add_layer(frame);
	assert_all_pixels_eq(127, 127, 127, 255, get_result(1, 1));
}

TYPED_TEST(MixerTestOgl, HalfOpacity)
{
	core::draw_frame red_under(create_single_color_frame(255, 0, 0, 255, 1, 1));
	core::draw_frame green_over(create_single_color_frame(0, 255, 0, 255, 1, 1));
	green_over.transform().image_transform.opacity = 0.5;
	add_layer(red_under);
	add_layer(green_over);
	assert_all_pixels_eq(127, 127, 0, 255, get_result(1, 1));

	add_layer(green_over);
	assert_all_pixels_eq(0, 127, 0, 127, get_result(1, 1));
}

TYPED_TEST(MixerTestOgl, MakeGrayscaleWithSaturation)
{
	core::draw_frame frame(create_single_color_frame(255, 0, 0, 255, 1, 1));
	frame.transform().image_transform.saturation = 0.0;
	add_layer(frame);
	auto result = get_result(1, 1);
	
	ASSERT_EQ(result.data()[0], result.data()[1]);
	ASSERT_EQ(result.data()[1], result.data()[2]);
}

TYPED_TEST(MixerTestOgl, TransformFillScale)
{
	core::draw_frame frame(create_single_color_frame(255, 255, 255, 255, 1, 1));
	frame.transform().image_transform.fill_translation[0] = 0.5;
	frame.transform().image_transform.fill_translation[1] = 0.5;
	frame.transform().image_transform.fill_scale[0] = 0.5;
	frame.transform().image_transform.fill_scale[1] = 0.5;
	add_layer(frame);
	auto res = get_result(2, 2);
	std::vector<const uint8_t> result(res.begin(), res.end());

	// bottom right corner
	ASSERT_EQ(boost::assign::list_of<uint8_t>
			(0)(0)(0)(0) (0)(0)(0)(0)
			(0)(0)(0)(0) (255)(255)(255)(255), result);

	frame.transform().image_transform.fill_translation[0] = 0;
	add_layer(frame);
	res = get_result(2, 2);
	result = std::vector<const uint8_t>(res.begin(), res.end());

	// bottom left corner
	ASSERT_EQ(boost::assign::list_of<uint8_t>
			(0)(0)(0)(0)         (0)(0)(0)(0)
			(255)(255)(255)(255) (0)(0)(0)(0), result);
}

TYPED_TEST(MixerTestOgl, LevelsBugGammaMinLevelsMismatch)
{
	auto src_frame = create_frame(2, 1);
	set_pixel(src_frame, 0, 0, 16, 16, 16, 255);
	set_pixel(src_frame, 1, 0, 235, 235, 235, 255);

	core::draw_frame frame(std::move(src_frame));
	frame.transform().image_transform.levels.min_input = 16.0 / 255.0;
	frame.transform().image_transform.levels.max_input = 235.0 / 255.0;
	add_layer(frame);

	auto result = get_result(2, 1);
	assert_pixel_eq(0, 0, 0, 255, result.data(), 0);
	assert_pixel_eq(255, 255, 255, 255, result.data() + 4, 0);
}

// Tests for use cases that only works on GPU mixer with blend-modes enabled
// -------------------------------------------------------------------------

}}
