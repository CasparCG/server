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

#include "help_repository.h"
#include "help_sink.h"

#include <common/except.h>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <utility>

namespace caspar { namespace core {

typedef std::pair<std::wstring, help_item_describer> help_item;

struct help_repository::impl
{
	std::vector<std::pair<help_item, std::set<std::wstring>>> items;

	static void help(const help_repository& self, help_item item, help_sink& sink)
	{
		sink.begin_item(item.first);
		item.second(sink, self);
		sink.end_item();
	}
};


help_repository::help_repository()
	: impl_(new impl)
{
}

void help_repository::register_item(std::set<std::wstring> tags, std::wstring name, help_item_describer describer)
{
	impl_->items.push_back(std::make_pair(help_item(name, describer), tags));
}

void help_repository::help(std::set<std::wstring> tags, help_sink& sink) const
{
	for (auto& item : impl_->items)
	{
		if (std::includes(item.second.begin(), item.second.end(), tags.begin(), tags.end()))
			impl::help(*this, item.first, sink);
	}
}

void help_repository::help(std::set<std::wstring> tags, std::wstring name, help_sink& sink) const
{
	auto found = impl_->items | boost::adaptors::filtered([&](const std::pair<help_item, std::set<std::wstring>>& item)
	{
		return boost::iequals(item.first.first, name) && std::includes(
				item.second.begin(), item.second.end(),
				tags.begin(), tags.end());
	});

	if (found.empty())
		CASPAR_THROW_EXCEPTION(file_not_found() << msg_info(L"Could not find help item " + name));

	for (auto& item : found)
	{
		if (std::includes(item.second.begin(), item.second.end(), tags.begin(), tags.end()))
			impl::help(*this, item.first, sink);
	}
}

}}
