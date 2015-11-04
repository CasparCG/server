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

#include "../system_info.h"

#include "../../memory.h"

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/algorithm/count.hpp>
#include <boost/iostreams/filter/grep.hpp>
#include <boost/iostreams/filter/line.hpp>
#include <boost/iostreams/filter/aggregate.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/pipeline.hpp>
#include <boost/iostreams/compose.hpp>
#include <boost/iostreams/device/file.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <functional>

namespace caspar {

namespace detail {

	class functor_line_filter : public boost::iostreams::wline_filter
	{
		std::function<std::wstring (const std::wstring& line)> func_;
	public:
		template<typename Func>
		functor_line_filter(const Func& func)
		: func_(func)
		{
		}

		std::wstring do_filter(const std::wstring& in) override
		{
			return func_(in);
		}
	};

	class truncate_duplicates : public boost::iostreams::wline_filter
	{
		std::set<std::wstring> seen_;
	public:
		std::wstring do_filter(const std::wstring& in) override
		{
			auto previously_not_seen = seen_.insert(in).second;

			if (previously_not_seen)
				return in;
			else
				return L"";
		}
	};

} // namespace detail

std::wstring consume(std::shared_ptr<std::wistream>&& in)
{
	return std::wstring(std::istreambuf_iterator<wchar_t>(*in), std::istreambuf_iterator<wchar_t>());
}

template<typename Filter>
std::shared_ptr<boost::iostreams::filtering_wistream> operator|(std::shared_ptr<std::wistream> in, const Filter& filter)
{
	spl::shared_ptr<boost::iostreams::filtering_wistream> filtered;
	filtered->push(filter);
	filtered->push(*in);

	return std::shared_ptr<boost::iostreams::filtering_wistream>(
		filtered.get(),
		[in, filtered](boost::iostreams::filtering_wistream* p) {});
}

// Commands

std::shared_ptr<std::wifstream> cat(const std::string& file)
{
	return std::make_shared<std::wifstream>(file);
}

boost::iostreams::wgrep_filter grep(const std::wstring& pattern)
{
	return boost::iostreams::wgrep_filter(boost::wregex(pattern));
}

detail::functor_line_filter cut(const std::wstring& delimiter, int column)
{
	return detail::functor_line_filter([=](const std::wstring& line)
	{
		std::vector<std::wstring> result;
		boost::algorithm::split(result, line, boost::is_any_of(delimiter), boost::algorithm::token_compress_on);
		return result.at(column);
	});
}

detail::functor_line_filter trim()
{
	return detail::functor_line_filter([](const std::wstring& line) { return boost::trim_copy(line); });
}

struct wc : boost::iostreams::aggregate_filter<wchar_t>
{
	void do_filter(const std::vector<wchar_t>& src, std::vector<wchar_t>& dest) override
	{
		auto line_count = boost::range::count(src, L'\n') + 4;
		auto out = boost::lexical_cast<std::wstring>(line_count);
		dest.insert(dest.begin(), out.begin(), out.end());
		dest.push_back('\n');
	}
};

struct remove_empty_lines : boost::iostreams::aggregate_filter<wchar_t>
{
	void do_filter(const std::vector<wchar_t>& src, std::vector<wchar_t>& dest) override
	{
		bool last_was_newline = false;

		for (auto iter = src.begin(), end = src.end(); iter != end; ++iter)
		{
			auto current    = *iter;
			auto is_newline = current == '\n';

			if (is_newline && (last_was_newline || iter == src.begin()))
				continue;

			dest.push_back(current);
			last_was_newline = is_newline;
		}

		if (!dest.empty() && dest.back() == '\n')
			dest.pop_back();
	}
};

boost::iostreams::composite<remove_empty_lines, detail::truncate_duplicates> uniq()
{
	return boost::iostreams::compose(remove_empty_lines(), detail::truncate_duplicates());
}

std::wstring cpu_info()
{
	try
	{
		auto num_physical_threads	= consume(cat("/proc/cpuinfo") | grep(L"model name") | wc() | remove_empty_lines());
		auto processor_names		= consume(cat("/proc/cpuinfo") | grep(L"model name") | cut(L":", 1) | trim() | uniq());

		return processor_names + L" Physical threads: " + num_physical_threads;
	}
	catch (...)
	{
		return L"Unknown CPU";
	}
}

std::wstring system_product_name()
{
	try
	{
		return consume(cat("/sys/devices/virtual/dmi/id/product_name") | remove_empty_lines());
	}
	catch (...)
	{
		return L"Unknown System";
	}
}

std::wstring os_description()
{
	try
	{
		return consume(cat("/etc/issue.net") | remove_empty_lines());
	}
	catch (...)
	{
		return L"Unknown OS";
	}
}

}

