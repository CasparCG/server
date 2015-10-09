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

#include <common/memory.h>

#include <core/frame/audio_channel_layout.h>
#include <core/frame/frame.h>

#include <boost/range/algorithm/equal.hpp>

namespace {

::caspar::core::audio_buffer get_buffer(::caspar::core::mutable_audio_buffer buffer)
{
	::caspar::spl::shared_ptr<::caspar::core::mutable_audio_buffer> buf(new ::caspar::core::mutable_audio_buffer(std::move(buffer)));
	return ::caspar::core::audio_buffer(buf->data(), buf->size(), true, std::move(buf));
}

}

namespace caspar {

bool operator==(const ::caspar::core::audio_buffer& lhs, const ::caspar::core::audio_buffer& rhs)
{
	return boost::range::equal(lhs, rhs);
}

namespace core {

TEST(AudioChannelLayoutTest, PassThrough)
{
	audio_channel_layout input_layout(2, L"stereo", L"L R");
	audio_channel_layout output_layout(2, L"stereo", L"L R" );
	audio_channel_remapper remapper(input_layout, output_layout);

	auto result = remapper.mix_and_rearrange(get_buffer({ 1, 2 }));

	EXPECT_EQ(get_buffer({ 1, 2 }), result);
}

TEST(AudioChannelLayoutTest, ReverseLeftAndRight)
{
	audio_channel_layout input_layout(2,L"stereo", L"L R");
	audio_channel_layout output_layout(2, L"stereo", L"R L");
	audio_channel_remapper remapper(input_layout, output_layout);

	auto result = remapper.mix_and_rearrange(get_buffer({ 1, 2, 3, 4 }));

	EXPECT_EQ(get_buffer({ 2, 1, 4, 3 }), result);
}

TEST(AudioChannelLayoutTest, StereoToMono)
{
	spl::shared_ptr<audio_mix_config_repository> mix_repo;
	mix_repo->register_config(L"stereo", { L"mono" }, L"C < L + R");
	audio_channel_layout input_layout(2, L"stereo", L"L R");
	audio_channel_layout output_layout(1, L"mono", L"C");
	audio_channel_remapper remapper(input_layout, output_layout, mix_repo);

	auto result = remapper.mix_and_rearrange(get_buffer({ 10, 30, 50, 30 }));

	EXPECT_EQ(get_buffer({ 20, 40 }), result);
}

TEST(AudioChannelLayoutTest, StereoToPassthru)
{
	audio_channel_layout input_layout(2, L"stereo", L"L R");
	audio_channel_layout output_layout(16, L"16ch", L"");
	audio_channel_remapper remapper(input_layout, output_layout);

	auto result = remapper.mix_and_rearrange(get_buffer({ 1, 2 }));

	EXPECT_EQ(get_buffer({ 1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }), result);
}

TEST(AudioChannelLayoutTest, 16ChTo8ChPassthruDropLast8)
{
	audio_channel_layout input_layout(16, L"16ch", L"");
	audio_channel_layout output_layout(8, L"8ch", L"");
	audio_channel_remapper remapper(input_layout, output_layout);

	auto result = remapper.mix_and_rearrange(get_buffer(
	{
		1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, // Sample 1
		17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, // Sample 2
	}));

	EXPECT_EQ(get_buffer(
	{
		1,  2,  3,  4,  5,  6,  7,  8, // Sample 1
		17, 18, 19, 20, 21, 22, 23, 24 // Sample 2
	}), result);
}

TEST(AudioChannelLayoutTest, MixMultipliers)
{
	spl::shared_ptr<audio_mix_config_repository> mix_repo;
	mix_repo->register_config(
			L"5.1",
			{ L"stereo" },
			L"FL < FL + 0.5 * FC + 0.5 * BL | FR < FR + 0.5 * FC + 0.5 * BR"); // = 1.0 / (1.0 + 0.5 + 0.5) = 0.5 scaled => 0.5 + 0.25 + 0.25
	audio_channel_layout input_layout(6, L"5.1", L"FL FR FC LFE BL BR");
	audio_channel_layout output_layout(2, L"stereo", L"FL FR");
	audio_channel_remapper remapper(input_layout, output_layout, mix_repo);
	//                                                    FL   FR  FC   LFE   BL  BR
	auto result = remapper.mix_and_rearrange(get_buffer({ 200, 50, 100, 1000, 40, 80 }));

	EXPECT_EQ(get_buffer({
		100 + 25 + 10,
		25  + 25 + 20
	}), result);
}

}}
