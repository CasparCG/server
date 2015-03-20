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

#include "../../compiler/vs/stack_walker.h"

#include <utility>

#include <tbb/enumerable_thread_specific.h>

namespace caspar {

std::wstring get_call_stack()
{
	class log_call_stack_walker : public stack_walker
	{
		std::string str_;
	public:
		log_call_stack_walker() : stack_walker() {}

		std::string flush()
		{
			return std::move(str_);
		}
	protected:
		virtual void OnSymInit(LPCSTR szSearchPath, DWORD symOptions, LPCSTR szUserName)
		{
		}
		virtual void OnLoadModule(LPCSTR img, LPCSTR mod, DWORD64 baseAddr, DWORD size, DWORD result, LPCSTR symType, LPCSTR pdbName, ULONGLONG fileVersion)
		{
		}
		virtual void OnDbgHelpErr(LPCSTR szFuncName, DWORD gle, DWORD64 addr)
		{
		}
		virtual void OnOutput(LPCSTR szText)
		{
			std::string str = szText;

			if(str.find("internal::get_call_stack") == std::string::npos && str.find("stack_walker::ShowCallstack") == std::string::npos)
				str_ += std::move(str);
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
		return L"!!!";
	}
}

}
