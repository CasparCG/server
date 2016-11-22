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

#include "../../stdafx.h"

#include "../stack_trace.h"
#include "../../utf.h"

#include "../../compiler/vs/StackWalker.h"

#include <boost/algorithm/string/replace.hpp>

#include <utility>

#include <tbb/enumerable_thread_specific.h>
#include <boost/algorithm/string/find.hpp>

namespace caspar {

std::wstring get_call_stack()
{
	class log_call_stack_walker : public StackWalker
	{
		std::string str_ = "\n";
	public:
		log_call_stack_walker()
			: StackWalker()
		{
		}

		std::string flush()
		{
			auto result = std::move(str_);

			str_ = "\n";

			return result;
		}
	protected:
		virtual void OnSymInit(LPCSTR szSearchPath, DWORD symOptions, LPCSTR szUserName) override
		{
		}
		virtual void OnLoadModule(LPCSTR img, LPCSTR mod, DWORD64 baseAddr, DWORD size, DWORD result, LPCSTR symType, LPCSTR pdbName, ULONGLONG fileVersion) override
		{
		}
		virtual void OnDbgHelpErr(LPCSTR szFuncName, DWORD gle, DWORD64 addr) override
		{
		}
		virtual void OnOutput(LPCSTR szText) override
		{
			std::string str = szText;

			auto include_path = boost::find_first(str, "\\include\\");

			if (include_path)
			{
				str.erase(str.begin(), include_path.end());
			}

			boost::ireplace_all(str, get_source_prefix(), "");

			if (str.find("caspar::get_call_stack") == std::string::npos &&
				str.find("StackWalker::ShowCallstack") == std::string::npos)
			{
				str_ += "    " + std::move(str);
			}
		}
	};

	static tbb::enumerable_thread_specific<log_call_stack_walker> walkers;
	try
	{
		auto& walker = walkers.local();
		walker.ShowCallstack();
		return u16(walker.flush());
	}
	catch(...)
	{
		return L"Bug in stacktrace code!!!";
	}
}

const std::string& get_source_prefix()
{
	static const std::string SOURCE_PREFIX = []
	{
		std::string result = CASPAR_SOURCE_PREFIX;

		boost::replace_all(result, "/", "\\");
		result += "\\";

		return result;
	}();

	return SOURCE_PREFIX;
}

}
