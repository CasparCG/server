/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
#pragma once

namespace caspar{ namespace CLK {

class CLKCommand
{
public:
	enum CLKCommands 
	{
		CLKDuration,
		CLKNewDuration,
		CLKNextEvent,
		CLKStop,
		CLKUntil,
		CLKAdd,
		CLKSub,
		CLKReset,
		CLKInvalidCommand
	};

	CLKCommand();
	virtual ~CLKCommand();

	bool SetCommand();
	bool NeedsTime() const 
	{
		return !(command_ == CLKNextEvent || command_ == CLKStop);
	}

	void Clear();
	const std::wstring& GetData();

	std::wstring dataCache_;
	std::wstring commandString_;
	CLKCommands command_;
	int clockID_;
	std::wstring time_;
	std::vector<std::wstring> parameters_;
};

}}