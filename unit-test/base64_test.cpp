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

#include <set>

#include <boost/foreach.hpp>
#include <boost/assign.hpp>

#include <common/base64.h>
#include <common/except.h>

namespace {
	static const char DICTIONARY[] =
	{
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'/', '+'
	};

	const std::set<char>& get_dictionary()
	{
		static const std::set<char> dictionary(
				DICTIONARY, DICTIONARY + sizeof(DICTIONARY));

		return dictionary;
	}
}

namespace caspar {

TEST(Base64Test, InvalidInputLength)
{
	// 0 and 4 characters should be ok
	/*EXPECT_THROW(from_base64("1"), caspar_exception);
	EXPECT_THROW(from_base64("12"), caspar_exception);
	EXPECT_THROW(from_base64("123"), caspar_exception);
	EXPECT_THROW(from_base64("1234\n567"), caspar_exception);
	EXPECT_THROW(from_base64("12345"), caspar_exception);*/
}

TEST(Base64Test, InvalidInputCharacters)
{
	static std::string PREFIX("AAA");

	for (int i = 1; i < 256; ++i)
	{
		if (get_dictionary().find(static_cast<char>(i)) == get_dictionary().end()
				&& !std::isspace(i)
				&& i != '=')
		{
			auto invalid = PREFIX + static_cast<char>(i);

			//EXPECT_THROW(from_base64(invalid), std::exception);
		}
	}
}

TEST(Base64Test, WithoutPadding)
{
	EXPECT_EQ(std::vector<uint8_t>(), from_base64(""));
	EXPECT_EQ(boost::assign::list_of<uint8_t>(0)(0)(0),
			from_base64("AAAA"));
	EXPECT_EQ(boost::assign::list_of<uint8_t>(0)(0)(0)(0)(0)(0),
			from_base64("AAAAAAAA"));
	EXPECT_EQ(boost::assign::list_of<uint8_t>(255)(255)(255),
			from_base64("////"));
	EXPECT_EQ(boost::assign::list_of<uint8_t>(0)(0)(0)(255)(255)(255),
			from_base64("AAAA////"));
}

TEST(Base64Test, WithPadding)
{
	EXPECT_EQ(boost::assign::list_of<uint8_t>(0),
			from_base64("AA=="));
	EXPECT_EQ(boost::assign::list_of<uint8_t>(0)(0),
			from_base64("AAA="));
	EXPECT_EQ(boost::assign::list_of<uint8_t>(255),
			from_base64("//=="));
	EXPECT_EQ(boost::assign::list_of<uint8_t>(255)(255),
			from_base64("//8="));
	EXPECT_EQ(boost::assign::list_of<uint8_t>(255)(255),
			from_base64("//9="));
	EXPECT_EQ(boost::assign::list_of<uint8_t>(255)(255),
			from_base64("//+="));
	EXPECT_EQ(boost::assign::list_of<uint8_t>(255)(255),
			from_base64("///="));
}

TEST(Base64Test, ToBase64)
{
	static const char ALL_ZERO = 0;
	static const char ALL_ONE = static_cast<char>(static_cast<unsigned char>(255));
	std::vector<char> data;

	EXPECT_EQ("", to_base64(data.data(), data.size()));

	data.push_back(ALL_ZERO);
	EXPECT_EQ("AA==", to_base64(data.data(), data.size()));

	data.push_back(ALL_ONE);
	EXPECT_EQ("AP8=", to_base64(data.data(), data.size()));

	data.push_back(ALL_ZERO);
	EXPECT_EQ("AP8A", to_base64(data.data(), data.size()));

	data.push_back(ALL_ONE);
	EXPECT_EQ("AP8A/w==", to_base64(data.data(), data.size()));

	data.push_back(ALL_ZERO);
	EXPECT_EQ("AP8A/wA=", to_base64(data.data(), data.size()));
}

}
