/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
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

#include <string>
#include <vector>

#include <boost/array.hpp>
#include <boost/algorithm/string/split.hpp>

namespace caspar {

template<size_t NUM>
class software_version
{
	boost::array<int, NUM> numbers_;
	std::string version_;
public:
	software_version(std::string version)
		: version_(std::move(version))
	{
		std::fill(numbers_.begin(), numbers_.end(), 0);
		std::vector<std::string> tokens;
		boost::split(tokens, version_, boost::is_any_of("."));
		auto num = std::min(NUM, tokens.size());

		for (size_t i = 0; i < num; ++i)
		{
			try
			{
				numbers_[i] = boost::lexical_cast<int>(tokens[i]);
			}
			catch (const boost::bad_lexical_cast&)
			{
				return;
			}
		}
	};

	const std::string& to_string() const
	{
		return version_;
	}

	bool operator<(const software_version<NUM>& other) const
	{
		for (int i = 0; i < NUM; ++i)
		{
			int this_num = numbers_[i];
			int other_num = other.numbers_[i];

			if (this_num < other_num)
				return true;
			else if (this_num > other_num)
				return false;
		}

		return false;
	}
};

}
