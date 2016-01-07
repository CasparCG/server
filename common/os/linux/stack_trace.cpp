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

#include "../stack_trace.h"

#include <execinfo.h>
#include <cstdio>
#include <array>
#include <memory>
#include <sstream>
#include <cxxabi.h>

namespace caspar {

std::string demangle(const std::string& mangled)
{
	auto start_of_name = mangled.find_first_of('(');

	if (start_of_name == std::string::npos)
        return "";

	auto start_of_offset = mangled.find_first_of('+', start_of_name);

	if (start_of_offset == std::string::npos)
        return "";

	auto end_of_name = mangled.find_first_of(')', start_of_offset);

	if (end_of_name == std::string::npos)
        return "";

	auto file = mangled.substr(0, start_of_name);
	auto mangled_symbol_name = mangled.substr(start_of_name + 1, start_of_offset - start_of_name - 1);
	auto offset = mangled.substr(start_of_offset + 1, end_of_name - start_of_offset - 1);

	int status;
	auto demangled = abi::__cxa_demangle(mangled_symbol_name.c_str(), nullptr, nullptr, &status);
	bool demangled_success = status == 0;

	if (demangled_success)
	{
		auto demangled_guard = std::shared_ptr<char>(demangled, free);

		return file + " : " + demangled + " + " + offset;
	}
	else
	{
		return "";
	}
}

std::wstring get_call_stack()
{
	std::array<void*, 100> stackframes;
	auto size = backtrace(stackframes.data(), stackframes.size());
	auto strings = backtrace_symbols(stackframes.data(), size);

	if (strings == nullptr)
		return L"Out of memory while generating stack trace";

	try
	{
		auto entries = std::shared_ptr<char*>(strings, free);
		std::wostringstream stream;
		stream << std::endl;

		for (int i = 0; i != size; ++i)
		{
			auto demangled = demangle(strings[i]);

			if (!demangled.empty() && demangled.find("caspar::get_call_stack") == std::string::npos)
				stream << L"   " << demangled.c_str() << std::endl;
		}

		return stream.str();
	}
	catch (const std::bad_alloc&)
	{
		return L"Out of memory while generating stack trace";
	}
	catch (...)
	{
		return L"Error while generating stack trace";
	}
}

}
