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

#pragma once

#include "../fwd.h"

#include <common/memory.h>

#include <set>
#include <vector>
#include <functional>
#include <utility>

namespace caspar { namespace core {

typedef std::function<void(core::help_sink&, const core::help_repository&)> help_item_describer;

class help_repository
{
public:
	help_repository();
	void register_item(std::set<std::wstring> tags, std::wstring name, help_item_describer describer); // Not thread safe
	void help(std::set<std::wstring> tags, help_sink& sink) const;
	void help(std::set<std::wstring> tags, std::wstring name, help_sink& sink) const;
private:
	struct impl;
	spl::shared_ptr<impl> impl_;
};

}}
