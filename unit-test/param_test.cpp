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

#include <boost/assign.hpp>

#include <common/param.h>

namespace {
	static auto params = boost::assign::list_of<std::wstring>
			(L"param1")(L"1")
			(L"param2")(L"string value");
}

namespace caspar {

TEST(ParamTest, GetExisting)
{
	EXPECT_EQ(L"string value", get_param(L"param2", params));
	EXPECT_EQ(1, get_param<int>(L"param1", params));
}

TEST(ParamTest, GetDefaultValue)
{
	EXPECT_EQ(L"fail_val", get_param(L"param3", params, L"fail_val"));
	EXPECT_EQ(1, get_param(L"param3", params, 1));
}

TEST(ParamTest, InvalidLexicalCast)
{
	EXPECT_THROW(get_param<bool>(L"param2", params), invalid_argument);
}

}
