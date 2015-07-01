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

#include "util.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <list>
#include <vector>
#include <sstream>

namespace caspar { namespace core {

void wordwrap_line(const std::wstring& line, int width, std::wostream& result)
{
	std::list<std::wstring> words;
	boost::split(words, line, boost::is_any_of(L" "), boost::algorithm::token_compress_on);

	size_t current_width = 0;
	bool first = true;

	while (!words.empty())
	{
		auto word = std::move(words.front());
		words.pop_front();

		if (first)
		{
			current_width = word.length();
			result << std::move(word);
			first = false;
		}
		else if (current_width + 1 + word.length() > width)
		{
			result << L"\n";
			current_width = word.length();
			result << std::move(word);
		}
		else
		{
			current_width += 1 + word.length();
			result << L" " << std::move(word);
		}
	}
}

std::wstring wordwrap(const std::wstring& text, int width)
{
	std::vector<std::wstring> lines;
	boost::split(lines, text, boost::is_any_of(L"\n"));

	std::wstringstream result;

	for (auto& line : lines)
	{
		wordwrap_line(line, width, result);
		result << L"\n";
	}

	return result.str();
}

std::wstring indent(std::wstring text, const std::wstring& indent)
{
	text.insert(0, indent);
	bool last_is_newline = text.at(text.length() - 1) == L'\n';

	if (last_is_newline)
		text.erase(text.length() - 1);

	boost::replace_all(text, L"\n", L"\n" + indent);

	if (last_is_newline)
		text += L'\n';

	return text;
}

}}
