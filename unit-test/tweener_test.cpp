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

#include <common/tweener.h>

namespace caspar {

class TweenerTest : public ::testing::TestWithParam<std::wstring>
{
};

TEST_P(TweenerTest, StartsAndEndsCloseToDesiredSourceAndTarget)
{
	static const double duration = 15.0;
	static const double start_value = 3.0;
	static const double value_delta = 5.0;
	static const double end_value = start_value + value_delta;
	static const double REQUIRED_CLOSENESS = 0.01;

	auto name = GetParam();
	tweener t(name);

	EXPECT_NEAR(
		start_value,
		t(0, start_value, value_delta, duration),
		REQUIRED_CLOSENESS);
	EXPECT_NEAR(
		end_value,
		t(duration, start_value, value_delta, duration),
		REQUIRED_CLOSENESS);
}

INSTANTIATE_TEST_CASE_P(
	AllTweeners,
	TweenerTest,
	::testing::ValuesIn(tweener::names()));

}
